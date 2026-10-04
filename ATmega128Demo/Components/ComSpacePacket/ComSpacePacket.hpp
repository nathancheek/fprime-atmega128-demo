// ======================================================================
// \title  ComSpacePacket.hpp
// \brief  hpp file for ComSpacePacket component implementation class
// ======================================================================

#ifndef ATmega128Demo_ComSpacePacket_HPP
#define ATmega128Demo_ComSpacePacket_HPP

#include "ATmega128Demo/Components/ComSpacePacket/ComSpacePacketComponentAc.hpp"
#include "config/FppConstantsAc.hpp"

namespace ATmega128Demo {

class ComSpacePacket final : public ComSpacePacketComponentBase {
  public:
    //! Sync word before each packet (ComCfg.SpacePacketSyncWord), or 0 for none
    static constexpr U16 SYNC_WORD = ComCfg::SpacePacketSyncWord;
    static constexpr U8 SYNC_SIZE = (SYNC_WORD != 0) ? 2 : 0;
    static constexpr U8 HEADER_SIZE = 6;
    //! CRC-16 after each packet (ComCfg.SpacePacketCrc), or none
    static constexpr U8 CRC_SIZE = (ComCfg::SpacePacketCrc != 0) ? 2 : 0;
    //! Largest packet on the link, sync word, header and CRC included
    static constexpr U8 MAX_PACKET_SIZE = SYNC_SIZE + HEADER_SIZE + FW_COM_BUFFER_MAX_SIZE + CRC_SIZE;

    static_assert(SYNC_SIZE + HEADER_SIZE + FW_COM_BUFFER_MAX_SIZE + CRC_SIZE <= 255,
                  "Link packet sizes are kept in a U8");

    ComSpacePacket(const char* const compName);
    ~ComSpacePacket();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    void drvReceiveIn_handler(FwIndexType portNum, Fw::Buffer& buffer, const Drv::ByteStreamStatus& status) override;
    Fw::Buffer drvAllocateIn_handler(FwIndexType portNum, FwSizeType size) override;
    void drvDeallocateIn_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) override;
    void comIn_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) override;

    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------

    //! Dispatch every complete packet in the receive buffer, dropping bytes that are not part of one
    void processRx();
    //! Write bytes to the driver
    void sendPiece(U8* data, U8 size);

    U8 m_rx[MAX_PACKET_SIZE];  //!< Received bytes not yet consumed
    U8 m_rxLen;                //!< Number of bytes in m_rx
    U16 m_seqCounts[10];       //!< Space Packet sequence counts, one per F Prime APID (FW_APIDS in the .cpp)
    U32 m_bytesReceived;       //!< Count for the BytesReceived channel
    U32 m_bytesSent;           //!< Count for the BytesSent channel
    U32 m_bytesDiscarded;      //!< Count for the BytesDiscarded channel
};

}  // namespace ATmega128Demo

#endif
