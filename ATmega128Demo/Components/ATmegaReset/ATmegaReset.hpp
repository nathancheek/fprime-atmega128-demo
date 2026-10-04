// ======================================================================
// \title  ATmegaReset.hpp
// \brief  hpp file for ATmegaReset component implementation class
// ======================================================================

#ifndef ATmega128Demo_ATmegaReset_HPP
#define ATmega128Demo_ATmegaReset_HPP

#include "ATmega128Demo/Components/ATmegaReset/ATmegaResetComponentAc.hpp"
#include "Fw/Types/Assert.hpp"

namespace ATmega128Demo {

class ATmegaReset final : public ATmegaResetComponentBase {
  public:
    //! Registers the assert hook, so asserts reset through the watchdog from construction on
    ATmegaReset(const char* const compName);
    ~ATmegaReset();

  private:
    //! Saves the assert in .noinit RAM and prints it on the console, then resets through the watchdog
    class Hook : public Fw::AssertHook {
      public:
        void reportAssert(FILE_NAME_ARG file,
                          FwSizeType lineNo,
                          FwSizeType numArgs,
                          FwAssertArgType arg1,
                          FwAssertArgType arg2,
                          FwAssertArgType arg3,
                          FwAssertArgType arg4,
                          FwAssertArgType arg5,
                          FwAssertArgType arg6) override;
        void doAssert() override;
    };

    void run_handler(FwIndexType portNum, U32 context) override;
    void REBOOT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
    void TEST_ASSERT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, I32 arg1, I32 arg2) override;
    void TEST_HANG_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    //! Report the reset and start or stop the watchdog once parameters are available
    void parametersLoaded() override;
    void parameterUpdated(FwPrmIdType id) override;

    //! Start or stop the watchdog to match WATCHDOG_ENABLED
    void applyWatchdogParam();

    Hook m_hook;
    ResetReason m_resetReason;
    U8 m_resetFlags;
    AssertInfo m_lastAssert;
    bool m_watchdogEnabled;
};

}  // namespace ATmega128Demo

#endif
