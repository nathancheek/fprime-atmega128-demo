// ======================================================================
// \title  PassiveTlmPacketizer.cpp
// \brief  cpp file for PassiveTlmPacketizer component implementation class
// ======================================================================

#include "ATmega128Demo/Components/PassiveTlmPacketizer/PassiveTlmPacketizer.hpp"
#include <cstring>
#include "Fw/Com/ComPacket.hpp"
#include "Fw/Types/Assert.hpp"

namespace ATmega128Demo {

PassiveTlmPacketizer ::PassiveTlmPacketizer(const char* const compName)
    : PassiveTlmPacketizerComponentBase(compName), m_packetList(nullptr) {
    ::memset(this->m_valuesSize, 0, sizeof(this->m_valuesSize));
    ::memset(this->m_values, 0, sizeof(this->m_values));
}

PassiveTlmPacketizer ::~PassiveTlmPacketizer() {}

void PassiveTlmPacketizer ::setPacketList(const Svc::TlmPacketizerPacketList& packetList) {
    FW_ASSERT(packetList.numEntries <= Svc::MAX_PACKETIZER_PACKETS, static_cast<FwAssertArgType>(packetList.numEntries));
    for (FwChanIdType p = 0; p < packetList.numEntries; p++) {
        const Svc::TlmPacketizerPacket* packet = packetList.list[p];
        FW_ASSERT(packet != nullptr);
        FwSizeType size = 0;
        for (FwChanIdType i = 0; i < packet->numEntries; i++) {
            size += packet->list[i].size;
        }
        FW_ASSERT(size <= MAX_VALUES_SIZE, static_cast<FwAssertArgType>(packet->id), static_cast<FwAssertArgType>(size));
        this->m_valuesSize[p] = size;
    }
    this->m_packetList = &packetList;
}

void PassiveTlmPacketizer ::TlmRecv_handler(FwIndexType portNum,
                                            FwChanIdType id,
                                            Fw::Time& timeTag,
                                            Fw::TlmBuffer& val) {
    FW_ASSERT(this->m_packetList != nullptr);
    // A channel may appear in more than one packet
    for (FwChanIdType p = 0; p < this->m_packetList->numEntries; p++) {
        const Svc::TlmPacketizerPacket* packet = this->m_packetList->list[p];
        FwSizeType offset = 0;
        for (FwChanIdType i = 0; i < packet->numEntries; i++) {
            const Svc::TlmPacketizerChannelEntry& entry = packet->list[i];
            if (entry.id == id) {
                FW_ASSERT(val.getBuffLength() <= entry.size, static_cast<FwAssertArgType>(id),
                          static_cast<FwAssertArgType>(val.getBuffLength()));
                ::memcpy(&this->m_values[p][offset], val.getBuffAddr(), val.getBuffLength());
                break;
            }
            offset += entry.size;
        }
    }
}

void PassiveTlmPacketizer ::Run_handler(FwIndexType portNum, U32 context) {
    FW_ASSERT(this->m_packetList != nullptr);
    for (FwChanIdType p = 0; p < this->m_packetList->numEntries; p++) {
        Fw::ComBuffer buffer;
        Fw::SerializeStatus stat =
            buffer.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_PACKETIZED_TLM));
        FW_ASSERT(stat == Fw::FW_SERIALIZE_OK, stat);
        stat = buffer.serialize(this->m_packetList->list[p]->id);
        FW_ASSERT(stat == Fw::FW_SERIALIZE_OK, stat);
        stat = buffer.serialize(this->getTime());
        FW_ASSERT(stat == Fw::FW_SERIALIZE_OK, stat);
        stat = buffer.serialize(this->m_values[p], this->m_valuesSize[p], Fw::Serialization::OMIT_LENGTH);
        FW_ASSERT(stat == Fw::FW_SERIALIZE_OK, stat);
        this->PktSend_out(0, buffer, 0);
    }
}

}  // namespace ATmega128Demo
