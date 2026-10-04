// ======================================================================
// \title  ComSpacePacket.cpp
// \brief  cpp file for ComSpacePacket component implementation class
// ======================================================================

#include "ATmega128Demo/Components/ComSpacePacket/ComSpacePacket.hpp"
#include "ATmega128Demo/Utils/Crc16/Crc16.hpp"
#include <cstring>
#include "Fw/Types/Assert.hpp"
#include "config/APIDEnumAc.hpp"

namespace ATmega128Demo {

namespace {
constexpr U16 IDLE_APID = 0x7FF;
// Space Packet header fields
constexpr U8 TYPE_TC = 0x10;                  // packet type bit in the first byte
constexpr U8 VERSION_TYPE_SECHDR_MASK = 0xF8;  // version (3b), type (1b), secondary header flag (1b)
constexpr U8 SEQ_FLAGS_MASK = 0xC0;
constexpr U16 SEQ_FLAGS_UNSEGMENTED = 0xC000;
// The F Prime APIDs, which ComCfg.Apid lets the deployment renumber. Each has its own sequence count.
constexpr ComCfg::Apid::T FW_APIDS[] = {
    ComCfg::Apid::FW_PACKET_COMMAND, ComCfg::Apid::FW_PACKET_TELEM,          ComCfg::Apid::FW_PACKET_LOG,
    ComCfg::Apid::FW_PACKET_FILE,    ComCfg::Apid::FW_PACKET_PACKETIZED_TLM, ComCfg::Apid::FW_PACKET_DP,
    ComCfg::Apid::FW_PACKET_IDLE,    ComCfg::Apid::FW_PACKET_PARAM,          ComCfg::Apid::FW_PACKET_HAND,
    ComCfg::Apid::FW_PACKET_UNKNOWN};

void putU16(U8* dest, U16 value) {
    dest[0] = static_cast<U8>(value >> 8);
    dest[1] = static_cast<U8>(value);
}

U16 getU16(const U8* src) {
    return static_cast<U16>((src[0] << 8) | src[1]);
}

U16 crcUpdate(U16 crc, const U8* data, FwSizeType size) {
    for (FwSizeType i = 0; i < size; i++) {
        crc = Crc16::update(crc, data[i]);
    }
    return crc;
}
}  // namespace

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

ComSpacePacket ::ComSpacePacket(const char* const compName)
    : ComSpacePacketComponentBase(compName),
      m_rxLen(0),
      m_bytesReceived(0),
      m_bytesSent(0),
      m_bytesDiscarded(0) {
    static_assert(FW_NUM_ARRAY_ELEMENTS(FW_APIDS) == FW_NUM_ARRAY_ELEMENTS(m_seqCounts),
                  "One sequence count per F Prime APID");
    ::memset(this->m_seqCounts, 0, sizeof(this->m_seqCounts));
}

ComSpacePacket ::~ComSpacePacket() {}

// ----------------------------------------------------------------------
// Receive
// ----------------------------------------------------------------------

Fw::Buffer ComSpacePacket ::drvAllocateIn_handler(FwIndexType portNum, FwSizeType size) {
    // processRx() always leaves at least one free byte, since a full buffer holds a whole packet
    FW_ASSERT(this->m_rxLen < MAX_PACKET_SIZE, static_cast<FwAssertArgType>(this->m_rxLen));
    const U8 free = static_cast<U8>(MAX_PACKET_SIZE - this->m_rxLen);
    return Fw::Buffer(&this->m_rx[this->m_rxLen], static_cast<Fw::Buffer::SizeType>((size < free) ? size : free));
}

void ComSpacePacket ::drvDeallocateIn_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) {
    // Buffers lent by drvAllocateIn point into m_rx, so there is nothing to free
}

void ComSpacePacket ::drvReceiveIn_handler(FwIndexType portNum,
                                         Fw::Buffer& buffer,
                                         const Drv::ByteStreamStatus& status) {
    if (status == Drv::ByteStreamStatus::OP_OK) {
        // The driver must have filled the space lent by drvAllocateIn
        FW_ASSERT(buffer.getData() == &this->m_rx[this->m_rxLen]);
        FW_ASSERT(buffer.getSize() <= static_cast<FwSizeType>(MAX_PACKET_SIZE - this->m_rxLen),
                  static_cast<FwAssertArgType>(buffer.getSize()));
        this->m_rxLen = static_cast<U8>(this->m_rxLen + buffer.getSize());
        this->m_bytesReceived += static_cast<U32>(buffer.getSize());
        this->tlmWrite_BytesReceived(this->m_bytesReceived);
        this->processRx();
    }
    this->drvReceiveReturnOut_out(0, buffer);
}

void ComSpacePacket ::processRx() {
    U8 start = 0;
    U8 discarded = 0;
    while (this->m_rxLen - start >= SYNC_SIZE + HEADER_SIZE) {
        // Optional sync word, then the Space Packet primary header: version (3b) | type (1b) | secondary header
        // flag (1b) | APID (11b), sequence flags (2b) | sequence count (14b), data length - 1 (16b)
        const U8* const packet = &this->m_rx[start + SYNC_SIZE];
        const U16 dataLen = static_cast<U16>(getU16(&packet[4]) + 1);
        const bool headerValid = (SYNC_SIZE == 0 || getU16(&this->m_rx[start]) == SYNC_WORD) &&
                                 ((packet[0] & VERSION_TYPE_SECHDR_MASK) == TYPE_TC) &&
                                 ((packet[2] & SEQ_FLAGS_MASK) == SEQ_FLAGS_MASK) &&
                                 (dataLen <= FW_COM_BUFFER_MAX_SIZE);
        if (!headerValid) {
            start++;  // Not a packet start: drop a byte and keep looking
            discarded++;
            continue;
        }
        const U8 packetLen = static_cast<U8>(HEADER_SIZE + dataLen);
        const U8 linkLen = static_cast<U8>(SYNC_SIZE + packetLen + CRC_SIZE);
        if (this->m_rxLen - start < linkLen) {
            break;  // Wait for the rest of the packet
        }
        // The CRC follows the Space Packet and covers it, but not the sync word
        if (CRC_SIZE != 0 && crcUpdate(Crc16::INITIAL, packet, packetLen) != getU16(&packet[packetLen])) {
            start++;
            discarded++;
            continue;
        }
        // Only command packets are accepted; other valid packets are skipped
        const U16 apid = static_cast<U16>(getU16(&packet[0]) & 0x07FF);
        if (apid == ComCfg::Apid::FW_PACKET_COMMAND) {
            Fw::ComBuffer com;
            if (com.setBuff(&packet[HEADER_SIZE], static_cast<FwSizeType>(dataLen)) == Fw::FW_SERIALIZE_OK) {
                this->comCmdOut_out(0, com, 0);
            }
        }
        start = static_cast<U8>(start + linkLen);
    }
    if (discarded > 0) {
        this->m_bytesDiscarded += discarded;
        this->tlmWrite_BytesDiscarded(this->m_bytesDiscarded);
    }
    if (start > 0) {
        this->m_rxLen = static_cast<U8>(this->m_rxLen - start);
        ::memmove(this->m_rx, &this->m_rx[start], this->m_rxLen);
    }
}

// ----------------------------------------------------------------------
// Send
// ----------------------------------------------------------------------

void ComSpacePacket ::comIn_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) {
    U8* const payload = data.getBuffAddr();
    const FwSizeType payloadLen = data.getBuffLength();
    FW_ASSERT(payloadLen > 0 && payloadLen <= FW_COM_BUFFER_MAX_SIZE, static_cast<FwAssertArgType>(payloadLen));

    // The APID is the F Prime packet descriptor, which starts the packet (big endian)
    FwPacketDescriptorType descriptor = 0;
    for (FwSizeType i = 0; i < sizeof(FwPacketDescriptorType) && i < payloadLen; i++) {
        descriptor = static_cast<FwPacketDescriptorType>((descriptor << 8) | payload[i]);
    }
    const U16 apid = (descriptor < IDLE_APID) ? static_cast<U16>(descriptor)
                                              : static_cast<U16>(ComCfg::Apid::FW_PACKET_UNKNOWN);
    U16 seqCount = 0;
    for (FwSizeType i = 0; i < FW_NUM_ARRAY_ELEMENTS(FW_APIDS); i++) {
        if (FW_APIDS[i] == apid) {
            seqCount = this->m_seqCounts[i];
            this->m_seqCounts[i] = static_cast<U16>((seqCount + 1) & 0x3FFF);
            break;
        }
    }

    // Version 0, type 0 (telemetry), no secondary header
    U8 header[HEADER_SIZE];
    putU16(&header[0], apid);
    putU16(&header[2], static_cast<U16>(SEQ_FLAGS_UNSEGMENTED | seqCount));
    putU16(&header[4], static_cast<U16>(payloadLen - 1));

    if (SYNC_SIZE != 0) {
        U8 sync[2];
        putU16(sync, SYNC_WORD);
        this->sendPiece(sync, sizeof(sync));
    }
    this->sendPiece(header, sizeof(header));
    this->sendPiece(payload, static_cast<U8>(payloadLen));
    if (CRC_SIZE != 0) {
        U8 trailer[2];
        putU16(trailer, crcUpdate(crcUpdate(Crc16::INITIAL, header, sizeof(header)), payload, payloadLen));
        this->sendPiece(trailer, sizeof(trailer));
    }
    // Once per packet; sendPiece() only counts
    this->tlmWrite_BytesSent(this->m_bytesSent);
}

void ComSpacePacket ::sendPiece(U8* data, U8 size) {
    Fw::Buffer buffer(data, size);
    (void)this->drvSendOut_out(0, buffer);
    this->m_bytesSent += size;
}

}  // namespace ATmega128Demo
