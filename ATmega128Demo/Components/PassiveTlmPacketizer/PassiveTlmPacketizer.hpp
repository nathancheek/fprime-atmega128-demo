// ======================================================================
// \title  PassiveTlmPacketizer.hpp
// \brief  hpp file for PassiveTlmPacketizer component implementation class
// ======================================================================

#ifndef ATmega128Demo_PassiveTlmPacketizer_HPP
#define ATmega128Demo_PassiveTlmPacketizer_HPP

#include <Svc/TlmPacketizer/TlmPacketizerTypes.hpp>
#include "ATmega128Demo/Components/PassiveTlmPacketizer/PassiveTlmPacketizerComponentAc.hpp"

namespace ATmega128Demo {

class PassiveTlmPacketizer final : public PassiveTlmPacketizerComponentBase {
  public:
    //! Bytes in front of the channel values: descriptor, packet ID and time
    static constexpr FwSizeType HEADER_SIZE =
        sizeof(FwPacketDescriptorType) + sizeof(FwTlmPacketizeIdType) + Fw::Time::SERIALIZED_SIZE;
    //! Space for the channel values
    static constexpr FwSizeType MAX_VALUES_SIZE = FW_COM_BUFFER_MAX_SIZE - HEADER_SIZE;

    PassiveTlmPacketizer(const char* const compName);
    ~PassiveTlmPacketizer();

    //! Use the packets in packetList, which must stay valid. Asserts if there are more than
    //! MAX_PACKETIZER_PACKETS or one does not fit.
    void setPacketList(const Svc::TlmPacketizerPacketList& packetList);

  private:
    void TlmRecv_handler(FwIndexType portNum, FwChanIdType id, Fw::Time& timeTag, Fw::TlmBuffer& val) override;
    void Run_handler(FwIndexType portNum, U32 context) override;

    const Svc::TlmPacketizerPacketList* m_packetList;
    FwSizeType m_valuesSize[Svc::MAX_PACKETIZER_PACKETS];       //!< Bytes of channel values in each packet
    U8 m_values[Svc::MAX_PACKETIZER_PACKETS][MAX_VALUES_SIZE];  //!< Latest channel values, in packet order
};

}  // namespace ATmega128Demo

#endif
