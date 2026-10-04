// ======================================================================
// \title  EepromPrmDb.hpp
// \brief  hpp file for EepromPrmDb component implementation class
// ======================================================================

#ifndef ATmega128Demo_EepromPrmDb_HPP
#define ATmega128Demo_EepromPrmDb_HPP

#include "ATmega128Demo/Components/EepromPrmDb/EepromPrmDbComponentAc.hpp"

namespace ATmega128Demo {

class EepromPrmDb final : public EepromPrmDbComponentBase {
  public:
    EepromPrmDb(const char* const compName);
    ~EepromPrmDb();

    //! Set the EEPROM bytes used for parameters (regionSize bytes from regionStart), and check the database there. Call
    //! before loading parameters. The rest of the EEPROM is left alone, so it can hold other data.
    void configure(U16 regionStart, U16 regionSize);

  private:
    //! The EEPROM layout's record fields: state | id | size | value | crc16
    static constexpr U16 ID_OFFSET = 1;
    static constexpr U16 SIZE_OFFSET = ID_OFFSET + sizeof(FwPrmIdType);
    static constexpr U16 VALUE_OFFSET = SIZE_OFFSET + 1;
    static constexpr U8 CRC_SIZE = 2;
    static constexpr U16 RECORD_OVERHEAD = VALUE_OFFSET + CRC_SIZE;
    static constexpr U16 NOT_FOUND = 0xFFFF;

    //! The region's header: BLANK (all 0xFF, as on a new chip), VALID, or INVALID
    enum class Header : U8 { BLANK, VALID, INVALID };

    //! Bytes to write to consecutive EEPROM addresses
    struct Segment {
        U16 address;
        const U8* data;
        U8 size;
        PrmDbStatus::T failStatus;  //!< Status if a byte doesn't read back
    };

    Fw::ParamValid getPrm_handler(FwIndexType portNum, FwPrmIdType id, Fw::ParamBuffer& val) override;
    void setPrm_handler(FwIndexType portNum, FwPrmIdType id, Fw::ParamBuffer& val) override;
    void run_handler(FwIndexType portNum, U32 context) override;
    void CLEAR_DB_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    //! Check every record (with CRCs if checkCrc), and find the newest one for id. Returns EMPTY (also for a blank
    //! region), OK or CORRUPT. found is the record's address (or NOT_FOUND) and end is where the next record would
    //! start.
    PrmDbStatus walk(FwPrmIdType id, bool checkCrc, U16& found, U16& end) const;
    Header readHeader() const;
    //! Queue a new header and an empty record list
    void queueFormat();
    void queue(U16 address, const U8* data, U8 size, PrmDbStatus::T failStatus);
    //! Start writing the queued segments on the next run call. Status is WRITING until they're written, then
    //! doneStatus.
    void startWriting(PrmDbStatus::T doneStatus);
    //! Check the byte written on the previous call, then start the next one that differs from EEPROM
    void writeStep();
    void setStatus(PrmDbStatus status);

    U16 m_regionStart;
    U16 m_regionEnd;  //!< Just past the region's last byte; 0 until configured
    PrmDbStatus m_status;

    // Background writing
    Segment m_segments[4];
    U8 m_segmentCount;
    U8 m_segment;          //!< Segment being written
    U8 m_offset;           //!< Byte within that segment
    bool m_writing;        //!< Segments are queued or being written
    bool m_byteStarted;    //!< The current byte's write was started and needs checking
    bool m_refused;        //!< A save was dropped while writing
    U16 m_appendAt;        //!< Where an appended record starts; its state byte is written last
    PrmDbStatus::T m_doneStatus;
    U8 m_record[RECORD_OVERHEAD + FW_PARAM_BUFFER_MAX_SIZE + 1];  //!< Record being saved, plus the end marker after it
};

}  // namespace ATmega128Demo

#endif
