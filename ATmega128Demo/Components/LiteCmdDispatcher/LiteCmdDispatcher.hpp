// ======================================================================
// \title  LiteCmdDispatcher.hpp
// \brief  hpp file for LiteCmdDispatcher component implementation class
// ======================================================================

#ifndef ATmega128Demo_LiteCmdDispatcher_HPP
#define ATmega128Demo_LiteCmdDispatcher_HPP

#include <config/CommandDispatcherImplCfg.hpp>
#include "ATmega128Demo/Components/LiteCmdDispatcher/LiteCmdDispatcherComponentAc.hpp"

namespace ATmega128Demo {

class LiteCmdDispatcher final : public LiteCmdDispatcherComponentBase {
  public:
    LiteCmdDispatcher(const char* const compName);
    ~LiteCmdDispatcher();

  private:
    void compCmdReg_handler(FwIndexType portNum, FwOpcodeType opCode) override;
    void compCmdStat_handler(FwIndexType portNum,
                             FwOpcodeType opCode,
                             U32 cmdSeq,
                             const Fw::CmdResponse& response) override;
    void seqCmdBuff_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) override;
    void CMD_NO_OP_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    //! Count a rejected or failed command
    void commandError();

    static_assert(NUM_COMPCMDSEND_OUTPUT_PORTS <= 255, "Ports are kept in a U8");

    struct DispatchEntry {
        FwOpcodeType opcode;
        U8 port;
    };
    DispatchEntry m_entries[CMD_DISPATCHER_DISPATCH_TABLE_SIZE];
    U8 m_numEntries;
    U32 m_seq;  //!< Sequence number given to each dispatched command
    U32 m_commandsDispatched;
    U32 m_commandErrors;
};

}  // namespace ATmega128Demo

#endif
