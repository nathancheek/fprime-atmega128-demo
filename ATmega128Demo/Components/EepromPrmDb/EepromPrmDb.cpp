// ======================================================================
// \title  EepromPrmDb.cpp
// \brief  cpp file for EepromPrmDb component implementation class
// ======================================================================

#include "ATmega128Demo/Components/EepromPrmDb/EepromPrmDb.hpp"
#include <avr/eeprom.h>
#include "ATmega128Demo/Utils/Crc16/Crc16.hpp"
#include "Fw/Types/Assert.hpp"

namespace ATmega128Demo {

// Each record stores its value size in one byte
static_assert(FW_PARAM_BUFFER_MAX_SIZE <= 0xFF, "Parameter values must fit in 255 bytes");

namespace {
constexpr U8 LAYOUT_VERSION = 1;
const U8 HEADER[] = {'P', 'D', LAYOUT_VERSION};
constexpr U8 HEADER_SIZE = sizeof(HEADER);
constexpr U8 ERASED = 0xFF;

constexpr U8 STATE_VALID = 0x5A;
constexpr U8 STATE_END = ERASED;
const U8 END_MARKER = STATE_END;

U8 readByte(U16 address) {
    return eeprom_read_byte(reinterpret_cast<const uint8_t*>(address));
}
}  // namespace

EepromPrmDb ::EepromPrmDb(const char* const compName)
    : EepromPrmDbComponentBase(compName),
      m_regionStart(0),
      m_regionEnd(0),
      m_status(PrmDbStatus::EMPTY),
      m_segmentCount(0),
      m_segment(0),
      m_offset(0),
      m_writing(false),
      m_byteStarted(false),
      m_refused(false),
      m_appendAt(0),
      m_doneStatus(PrmDbStatus::OK) {}

EepromPrmDb ::~EepromPrmDb() {}

void EepromPrmDb ::configure(U16 regionStart, U16 regionSize) {
    // Room for the header and an end marker, within the EEPROM
    FW_ASSERT(regionSize > HEADER_SIZE, regionSize);
    FW_ASSERT(static_cast<U32>(regionStart) + regionSize <= E2END + 1, regionStart, regionSize);
    this->m_regionStart = regionStart;
    this->m_regionEnd = static_cast<U16>(regionStart + regionSize);
    U16 found;
    U16 end;
    this->setStatus(this->walk(0, true, found, end));
}

void EepromPrmDb ::setStatus(PrmDbStatus status) {
    this->m_status = status;
    this->tlmWrite_Status(status);
}

// ----------------------------------------------------------------------
// Ports and commands
// ----------------------------------------------------------------------

Fw::ParamValid EepromPrmDb ::getPrm_handler(FwIndexType portNum, FwPrmIdType id, Fw::ParamBuffer& val) {
    // Anything wrong with the database means all defaults, never a mix of saved values and defaults
    if (this->m_status == PrmDbStatus::CORRUPT) {
        return Fw::ParamValid::INVALID;
    }
    U16 found;
    U16 end;
    if (this->walk(id, true, found, end) == PrmDbStatus::CORRUPT) {
        this->setStatus(PrmDbStatus::CORRUPT);
        return Fw::ParamValid::INVALID;
    }
    if (found == NOT_FOUND) {
        return Fw::ParamValid::INVALID;
    }
    // walk() already checked the CRC and the size (it fits a parameter buffer)
    const U8 size = readByte(static_cast<U16>(found + SIZE_OFFSET));
    eeprom_read_block(val.getBuffAddr(), reinterpret_cast<const void*>(found + VALUE_OFFSET), size);
    const Fw::SerializeStatus status = val.setBuffLen(size);
    FW_ASSERT(status == Fw::FW_SERIALIZE_OK, status);
    return Fw::ParamValid::VALID;
}

void EepromPrmDb ::setPrm_handler(FwIndexType portNum, FwPrmIdType id, Fw::ParamBuffer& val) {
    // Refused until CLEAR_DB, so a repair can't bring back older saved values alongside defaults
    if (this->m_status == PrmDbStatus::CORRUPT) {
        return;
    }
    // One write at a time: the record buffer holds one save
    if (this->m_writing) {
        this->m_refused = true;
        return;
    }
    // Only a blank region is formatted automatically
    U16 found = NOT_FOUND;
    U16 end = static_cast<U16>(this->m_regionStart + HEADER_SIZE);
    this->m_segmentCount = 0;
    const Header header = this->readHeader();
    if (header == Header::INVALID) {
        this->setStatus(PrmDbStatus::CORRUPT);
        return;
    }
    if (header == Header::BLANK) {
        this->queueFormat();
    } else if (this->walk(id, false, found, end) == PrmDbStatus::CORRUPT) {
        // CRCs were checked at boot; this walk only finds the record and the end
        this->setStatus(PrmDbStatus::CORRUPT);
        return;
    }

    // Build the record in RAM: state, id, size, value, CRC, then the end marker that follows it
    const U8 size = static_cast<U8>(val.getBuffLength());
    U8* const record = this->m_record;
    record[0] = STATE_VALID;
    for (U8 i = 0; i < sizeof(FwPrmIdType); i++) {
        record[ID_OFFSET + i] = static_cast<U8>(id >> (8 * (sizeof(FwPrmIdType) - 1 - i)));
    }
    record[SIZE_OFFSET] = size;
    for (U8 i = 0; i < size; i++) {
        record[VALUE_OFFSET + i] = val.getBuffAddr()[i];
    }
    U16 crc = Crc16::INITIAL;
    for (U16 i = 0; i < VALUE_OFFSET + size; i++) {
        crc = Crc16::update(crc, record[i]);
    }
    record[VALUE_OFFSET + size] = static_cast<U8>(crc >> 8);
    record[VALUE_OFFSET + size + 1] = static_cast<U8>(crc);
    record[RECORD_OVERHEAD + size] = STATE_END;

    if ((found != NOT_FOUND) && (readByte(static_cast<U16>(found + SIZE_OFFSET)) == size)) {
        // Same size: rewrite the value and CRC in place. If that fails or is cut short, the CRC no longer matches.
        this->queue(static_cast<U16>(found + VALUE_OFFSET), &record[VALUE_OFFSET], static_cast<U8>(size + CRC_SIZE),
                    PrmDbStatus::CORRUPT);
    } else {
        // Append, with the end marker after it. The newest record for an ID wins.
        if (static_cast<U32>(end) + RECORD_OVERHEAD + size + 1 > this->m_regionEnd) {
            this->setStatus(PrmDbStatus::FULL);
            return;
        }
        this->m_appendAt = end;
        this->queue(static_cast<U16>(end + ID_OFFSET), &record[ID_OFFSET], static_cast<U8>(RECORD_OVERHEAD + size),
                    PrmDbStatus::SAVE_FAILED);
        // The state byte goes last: until it reads VALID, the record isn't part of the database
        this->queue(end, &record[0], 1, PrmDbStatus::SAVE_FAILED);
    }
    this->startWriting(PrmDbStatus::OK);
}

void EepromPrmDb ::run_handler(FwIndexType portNum, U32 context) {
    if (this->m_writing) {
        this->writeStep();
    }
}

void EepromPrmDb ::CLEAR_DB_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (this->m_writing) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    this->m_segmentCount = 0;
    this->queueFormat();
    this->startWriting(PrmDbStatus::EMPTY);
    // Accepted; Status reports the result
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Database Writing
// ----------------------------------------------------------------------

void EepromPrmDb ::queueFormat() {
    // End marker first, then the header that makes the database valid
    this->queue(static_cast<U16>(this->m_regionStart + HEADER_SIZE), &END_MARKER, 1, PrmDbStatus::CORRUPT);
    this->queue(this->m_regionStart, HEADER, HEADER_SIZE, PrmDbStatus::CORRUPT);
}

void EepromPrmDb ::queue(U16 address, const U8* data, U8 size, PrmDbStatus::T failStatus) {
    FW_ASSERT(this->m_segmentCount < FW_NUM_ARRAY_ELEMENTS(this->m_segments), this->m_segmentCount);
    this->m_segments[this->m_segmentCount] = {address, data, size, failStatus};
    this->m_segmentCount++;
}

void EepromPrmDb ::startWriting(PrmDbStatus::T doneStatus) {
    this->m_segment = 0;
    this->m_offset = 0;
    this->m_byteStarted = false;
    this->m_refused = false;
    this->m_doneStatus = doneStatus;
    this->m_writing = true;
    // Every byte, the first included, is started by run, so nothing is written if run isn't connected
    this->setStatus(PrmDbStatus::WRITING);
}

// An EEPROM byte takes about 8.5 ms to write on the ATmega128, so writing a record back to back would stall the
// rate group. Instead each call starts at most one byte: eeprom_write_byte starts the write and returns, and the
// next call reads the byte back and starts the next one.
void EepromPrmDb ::writeStep() {
    // Never wait for a write in progress
    if (!eeprom_is_ready()) {
        return;
    }
    while (this->m_segment < this->m_segmentCount) {
        const Segment& segment = this->m_segments[this->m_segment];
        const U16 address = static_cast<U16>(segment.address + this->m_offset);
        const U8 byte = segment.data[this->m_offset];
        if (this->m_byteStarted || (readByte(address) == byte)) {
            // Read back the byte started on the previous call once, with no retry. A byte that already matches
            // needs no write.
            if (readByte(address) != byte) {
                PrmDbStatus::T status = segment.failStatus;
                // A failed append leaves the database unchanged as long as its state byte still marks the end
                if ((status == PrmDbStatus::SAVE_FAILED) && (readByte(this->m_appendAt) != STATE_END)) {
                    status = PrmDbStatus::CORRUPT;
                }
                this->m_writing = false;
                this->setStatus(status);
                return;
            }
            this->m_byteStarted = false;
            this->m_offset++;
            if (this->m_offset == segment.size) {
                this->m_offset = 0;
                this->m_segment++;
            }
            continue;
        }
        FW_ASSERT((address >= this->m_regionStart) && (address < this->m_regionEnd), address);
        eeprom_write_byte(reinterpret_cast<uint8_t*>(address), byte);
        this->m_byteStarted = true;
        return;
    }
    this->m_writing = false;
    this->setStatus(this->m_refused ? PrmDbStatus::SAVE_FAILED : this->m_doneStatus);
}

// ----------------------------------------------------------------------
// Database Reading
// ----------------------------------------------------------------------

PrmDbStatus EepromPrmDb ::walk(FwPrmIdType id, bool checkCrc, U16& found, U16& end) const {
    found = NOT_FOUND;
    end = static_cast<U16>(this->m_regionStart + HEADER_SIZE);
    const Header header = this->readHeader();
    if (header != Header::VALID) {
        return (header == Header::BLANK) ? PrmDbStatus::EMPTY : PrmDbStatus::CORRUPT;
    }
    U16 address = end;
    bool anyRecords = false;
    for (;;) {
        if (address >= this->m_regionEnd) {
            return PrmDbStatus::CORRUPT;  // No end marker
        }
        const U8 state = readByte(address);
        if (state == STATE_END) {
            break;
        }
        if ((state != STATE_VALID) || (address + RECORD_OVERHEAD > this->m_regionEnd)) {
            return PrmDbStatus::CORRUPT;
        }
        const U8 size = readByte(static_cast<U16>(address + SIZE_OFFSET));
        const U16 next = static_cast<U16>(address + RECORD_OVERHEAD + size);
        if ((size > FW_PARAM_BUFFER_MAX_SIZE) || (next > this->m_regionEnd)) {
            return PrmDbStatus::CORRUPT;
        }
        FwPrmIdType recordId = 0;
        for (U8 i = 0; i < sizeof(FwPrmIdType); i++) {
            recordId = static_cast<FwPrmIdType>((recordId << 8) | readByte(static_cast<U16>(address + ID_OFFSET + i)));
        }
        if (checkCrc) {
            // CRC of state, id, size and value as stored
            U16 crc = Crc16::INITIAL;
            for (U16 i = 0; i < VALUE_OFFSET + size; i++) {
                crc = Crc16::update(crc, readByte(static_cast<U16>(address + i)));
            }
            const U16 stored = static_cast<U16>((readByte(static_cast<U16>(address + VALUE_OFFSET + size)) << 8) |
                                                readByte(static_cast<U16>(address + VALUE_OFFSET + size + 1)));
            if (crc != stored) {
                return PrmDbStatus::CORRUPT;
            }
        }
        if (recordId == id) {
            found = address;  // Keep going: a later record for the same ID is newer
        }
        anyRecords = true;
        address = next;
    }
    end = address;
    return anyRecords ? PrmDbStatus::OK : PrmDbStatus::EMPTY;
}

EepromPrmDb::Header EepromPrmDb ::readHeader() const {
    FW_ASSERT(this->m_regionEnd != 0);  // configure() must be called first
    bool valid = true;
    bool blank = true;
    for (U8 i = 0; i < HEADER_SIZE; i++) {
        const U8 byte = readByte(static_cast<U16>(this->m_regionStart + i));
        valid = valid && (byte == HEADER[i]);
        blank = blank && (byte == ERASED);
    }
    return valid ? Header::VALID : (blank ? Header::BLANK : Header::INVALID);
}

}  // namespace ATmega128Demo
