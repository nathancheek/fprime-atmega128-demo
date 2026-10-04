// ======================================================================
// \title  LiteCmdDispatcher.cpp
// \brief  cpp file for LiteCmdDispatcher component implementation class
// ======================================================================

#include "ATmega128Demo/Components/LiteCmdDispatcher/LiteCmdDispatcher.hpp"
#include "Fw/Com/ComPacket.hpp"
#include "Fw/Types/Assert.hpp"

namespace ATmega128Demo {

static_assert(CMD_DISPATCHER_DISPATCH_TABLE_SIZE <= 255, "Dispatch table count is kept in a U8");

LiteCmdDispatcher ::LiteCmdDispatcher(const char* const compName)
    : LiteCmdDispatcherComponentBase(compName),
      m_numEntries(0),
      m_seq(0),
      m_commandsDispatched(0),
      m_commandErrors(0) {}

LiteCmdDispatcher ::~LiteCmdDispatcher() {}

void LiteCmdDispatcher ::compCmdReg_handler(FwIndexType portNum, FwOpcodeType opCode) {
    for (U8 i = 0; i < this->m_numEntries; i++) {
        FW_ASSERT(this->m_entries[i].opcode != opCode, static_cast<FwAssertArgType>(opCode));
    }
    FW_ASSERT(this->m_numEntries < CMD_DISPATCHER_DISPATCH_TABLE_SIZE, static_cast<FwAssertArgType>(opCode));
    this->m_entries[this->m_numEntries].opcode = opCode;
    this->m_entries[this->m_numEntries].port = static_cast<U8>(portNum);
    this->m_numEntries++;
}

void LiteCmdDispatcher ::seqCmdBuff_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) {
    // Command packet: descriptor, opcode, then the serialized arguments
    constexpr FwSizeType HEADER_SIZE = sizeof(FwPacketDescriptorType) + sizeof(FwOpcodeType);
    const U8* const bytes = data.getBuffAddr();
    const FwSizeType size = data.getBuffLength();
    if (size < HEADER_SIZE) {
        this->commandError();
        return;
    }
    FwPacketDescriptorType descriptor = 0;
    FwOpcodeType opcode = 0;
    FwSizeType offset = 0;
    for (; offset < sizeof(FwPacketDescriptorType); offset++) {
        descriptor = static_cast<FwPacketDescriptorType>((descriptor << 8) | bytes[offset]);
    }
    for (; offset < HEADER_SIZE; offset++) {
        opcode = static_cast<FwOpcodeType>((opcode << 8) | bytes[offset]);
    }
    if (descriptor != Fw::ComPacketType::FW_PACKET_COMMAND) {
        this->commandError();
        return;
    }

    for (U8 i = 0; i < this->m_numEntries; i++) {
        if (this->m_entries[i].opcode == opcode) {
            Fw::CmdArgBuffer args(&bytes[HEADER_SIZE], size - HEADER_SIZE);
            this->m_commandsDispatched++;
            this->tlmWrite_CommandsDispatched(this->m_commandsDispatched);
            this->compCmdSend_out(this->m_entries[i].port, opcode, this->m_seq++, args);
            return;
        }
    }
    this->commandError();  // Unknown opcode
}

void LiteCmdDispatcher ::compCmdStat_handler(FwIndexType portNum,
                                             FwOpcodeType opCode,
                                             U32 cmdSeq,
                                             const Fw::CmdResponse& response) {
    if (response != Fw::CmdResponse::OK) {
        this->commandError();
    }
}

void LiteCmdDispatcher ::CMD_NO_OP_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void LiteCmdDispatcher ::commandError() {
    this->m_commandErrors++;
    this->tlmWrite_CommandErrors(this->m_commandErrors);
}

}  // namespace ATmega128Demo
