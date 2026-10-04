// ======================================================================
// \title  ATmegaReset.cpp
// \brief  cpp file for ATmegaReset component implementation class
// ======================================================================

#include "ATmega128Demo/Components/ATmegaReset/ATmegaReset.hpp"
#include <Arduino/Os/Console.hpp>
#include <Fw/Logger/Logger.hpp>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <stddef.h>

// ----------------------------------------------------------------------
// Reset flags, captured before C++ static initialization
// ----------------------------------------------------------------------

// The bootloader (urboot) passes the reset flags in R2. Both R2 and MCUCSR are read in .init5,
// after XMEM is enabled (.init3) and .bss is cleared (.init4), neither of which touches R2.
extern "C" {
volatile U8 atmegaReset_r2;
volatile U8 atmegaReset_mcucsr;
void atmegaReset_captureFlags() __attribute__((naked, used, section(".init5")));
void atmegaReset_captureFlags() {
    U8 r2;
    asm volatile("mov %0, r2" : "=r"(r2));  // An operand, not the symbol name, so LTO can see the store
    atmegaReset_r2 = r2;
    atmegaReset_mcucsr = MCUCSR;
    MCUCSR = 0;  // So the next boot sees only the flags of its own reset
}
}

namespace Arduino {
extern volatile U8 s_pendingTicks;  // HardwareRateDriverAvr.cpp
}

namespace ATmega128Demo {

namespace {
// ----------------------------------------------------------------------
// Assert record, kept in .noinit RAM (external SRAM) across the watchdog reset
// ----------------------------------------------------------------------

constexpr U32 ASSERT_MAGIC = 0xA55E27ED;

struct AssertRecord {
    U32 magic;
    U32 fileId;
    U32 lineNo;
    U8 numArgs;
    I32 args[AssertArgs::SIZE];
    U16 check;  //!< Fletcher-16 of everything above, so leftover RAM contents are not mistaken for a record
};
AssertRecord s_assertRecord __attribute__((section(".noinit")));

// Set by the REBOOT command just before its watchdog reset
constexpr U32 REBOOT_MAGIC = 0x2EB0071D;
U32 s_rebootMarker __attribute__((section(".noinit")));

// Stall detection. urboot clears MCUCSR, and after a reset-pin reset (or an upload) it runs for 1 s
// and then exits through its own watchdog reset, so the application sees only WDRF either way.
// To tell a real watchdog timeout from that, read the rate driver's pending tick count from before
// the reset (it is in .noinit). The Timer1 interrupt keeps counting while the main loop is stuck,
// so 5 or more ticks (0.5 s at the 100 ms base tick, well before the 2 s timeout) means a stall.
constexpr U8 STALL_TICKS = 5;

// The tick count can only be trusted if this image wrote it. An upload that changes the size of .data or
// .bss moves .noinit, and the new image would read whatever the old one kept there. This marker sits next
// to the count and mixes in both addresses, so it only matches when the last image to run had the same
// .noinit layout.
constexpr U32 LAYOUT_MAGIC = 0x7C1C4B5D;
U32 s_layoutMarker __attribute__((section(".noinit")));

U32 layoutMarker() {
    const U32 tickAddress = static_cast<U16>(reinterpret_cast<uintptr_t>(&Arduino::s_pendingTicks));
    const U32 markerAddress = static_cast<U16>(reinterpret_cast<uintptr_t>(&s_layoutMarker));
    return LAYOUT_MAGIC ^ (tickAddress << 16) ^ markerAddress;
}

U16 recordCheck(const AssertRecord& record) {
    const U8* bytes = reinterpret_cast<const U8*>(&record);
    U8 sum1 = 0;
    U8 sum2 = 0;
    for (U8 i = 0; i < offsetof(AssertRecord, check); i++) {
        sum1 = static_cast<U8>(sum1 + bytes[i]);
        sum2 = static_cast<U8>(sum2 + sum1);
    }
    return static_cast<U16>((sum2 << 8) | sum1);
}

//! Reset as quickly as possible through the watchdog
void watchdogReset() {
    cli();
    wdt_enable(WDTO_15MS);
    for (;;) {
    }
}
}  // namespace

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

ATmegaReset ::ATmegaReset(const char* const compName)
    : ATmegaResetComponentBase(compName), m_resetReason(ResetReason::UNKNOWN), m_resetFlags(0), m_watchdogEnabled(false) {
    this->m_hook.registerHook();

    // Without a bootloader, MCUCSR holds the flags. Urboot clears MCUCSR and passes them in R2.
    const bool viaBootloader = (atmegaReset_mcucsr == 0);
    this->m_resetFlags = viaBootloader ? atmegaReset_r2 : atmegaReset_mcucsr;
    const U8 flags = this->m_resetFlags;
    // This runs during static initialization, before interrupts are enabled and before rateDriver.start(),
    // so the Timer1 interrupt can't change the count or race the marker here. The count is cleared once
    // read, so a reset before the rate driver starts can't report the same stall twice.
    const bool ticksValid = (s_layoutMarker == layoutMarker());
    const bool stalled = ticksValid && (Arduino::s_pendingTicks >= STALL_TICKS);
    Arduino::s_pendingTicks = 0;
    s_layoutMarker = layoutMarker();

    const bool asserted =
        (s_assertRecord.magic == ASSERT_MAGIC) && (s_assertRecord.check == recordCheck(s_assertRecord));
    const bool commanded = (s_rebootMarker == REBOOT_MAGIC);
    // Several flags can be set at once (e.g. PORF and BORF at power-up), so the most informative cause
    // wins: power-on makes RAM meaningless, an assert is the most specific (it also sets WDRF), a
    // brown-out can itself cause a hang, and pin and JTAG resets are deliberate. Asserts and REBOOT both
    // reset through the watchdog, so they're told apart by their .noinit markers.
    if (flags & _BV(PORF)) {
        this->m_resetReason = ResetReason::POWER_ON;
    } else if (asserted) {
        this->m_resetReason = ResetReason::ASSERT;
    } else if (commanded && (flags & _BV(WDRF))) {
        this->m_resetReason = ResetReason::COMMANDED;
    } else if (flags & _BV(BORF)) {
        this->m_resetReason = ResetReason::BROWN_OUT;
    } else if (flags & _BV(WDRF)) {
        // Through urboot, WDRF without a stall is the bootloader's own exit after a reset-pin reset
        this->m_resetReason = (stalled || !viaBootloader) ? ResetReason::WATCHDOG : ResetReason::RESET_PIN;
    } else if (flags & _BV(EXTRF)) {
        this->m_resetReason = ResetReason::RESET_PIN;
    } else if (flags & _BV(JTRF)) {
        this->m_resetReason = ResetReason::JTAG;
    }

    if (this->m_resetReason == ResetReason::ASSERT) {
        AssertArgs args;
        for (U8 i = 0; i < AssertArgs::SIZE; i++) {
            args[i] = s_assertRecord.args[i];
        }
        this->m_lastAssert.set(s_assertRecord.fileId, s_assertRecord.lineNo, s_assertRecord.numArgs, args);
    }
    s_assertRecord.magic = 0;  // Report each assert and reboot once
    s_rebootMarker = 0;
}

ATmegaReset ::~ATmegaReset() {}

// ----------------------------------------------------------------------
// Parameters
// ----------------------------------------------------------------------

void ATmegaReset ::parametersLoaded() {
    this->tlmWrite_ResetReason(this->m_resetReason);
    this->tlmWrite_ResetFlags(this->m_resetFlags);
    this->tlmWrite_LastAssert(this->m_lastAssert);
    this->applyWatchdogParam();
}

void ATmegaReset ::parameterUpdated(FwPrmIdType id) {
    if (id == PARAMID_WATCHDOG_ENABLED) {
        this->applyWatchdogParam();
    }
}

void ATmegaReset ::applyWatchdogParam() {
    Fw::ParamValid valid;
    this->m_watchdogEnabled = (this->paramGet_WATCHDOG_ENABLED(valid) == Fw::Enabled::ENABLED);
    if (this->m_watchdogEnabled) {
        wdt_enable(WDTO_2S);
    } else {
        wdt_disable();
    }
    this->tlmWrite_WatchdogEnabled(this->m_watchdogEnabled ? Fw::Enabled::ENABLED : Fw::Enabled::DISABLED);
}

// ----------------------------------------------------------------------
// Ports and commands
// ----------------------------------------------------------------------

void ATmegaReset ::run_handler(FwIndexType portNum, U32 context) {
    wdt_reset();
}

void ATmegaReset ::REBOOT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    // The response can't reach the ground before the reset; ResetReason reports the reboot instead
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    s_rebootMarker = REBOOT_MAGIC;
    watchdogReset();
}

void ATmegaReset ::TEST_ASSERT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, I32 arg1, I32 arg2) {
    FW_ASSERT(0, arg1, arg2);
}

void ATmegaReset ::TEST_HANG_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!this->m_watchdogEnabled) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    for (;;) {
    }
}

// ----------------------------------------------------------------------
// Assert hook
// ----------------------------------------------------------------------

void ATmegaReset::Hook ::reportAssert(FILE_NAME_ARG file,
                                      FwSizeType lineNo,
                                      FwSizeType numArgs,
                                      FwAssertArgType arg1,
                                      FwAssertArgType arg2,
                                      FwAssertArgType arg3,
                                      FwAssertArgType arg4,
                                      FwAssertArgType arg5,
                                      FwAssertArgType arg6) {
    static_assert(sizeof(FwAssertArgType) == sizeof(I32), "Assert args are stored as I32");
#if FW_ASSERT_LEVEL == FW_FILEID_ASSERT
    s_assertRecord.fileId = file;
#else
    s_assertRecord.fileId = 0;  // Only file ID asserts have a number to keep
#endif
    s_assertRecord.lineNo = static_cast<U32>(lineNo);
    s_assertRecord.numArgs = static_cast<U8>(numArgs);
    s_assertRecord.args[0] = arg1;
    s_assertRecord.args[1] = arg2;
    s_assertRecord.args[2] = arg3;
    s_assertRecord.args[3] = arg4;
    s_assertRecord.magic = ASSERT_MAGIC;
    s_assertRecord.check = recordCheck(s_assertRecord);

    // Print it on the console (the stream Main.cpp gives it), unless printing is what asserted
    static bool s_printing = false;
    if (!s_printing) {
        s_printing = true;
        const AssertRecord& r = s_assertRecord;
#if FW_ASSERT_LEVEL == FW_FILEID_ASSERT
        Fw::Logger::log("ASSERT file 0x%08" PRIx32 " line %" PRIu32 " args %" PRId32 " %" PRId32 " %" PRId32 " %" PRId32
                        " (%u)\n",
                        r.fileId, r.lineNo, r.args[0], r.args[1], r.args[2], r.args[3], static_cast<unsigned>(r.numArgs));
#else
        Fw::Logger::log("ASSERT %s line %" PRIu32 " args %" PRId32 " %" PRId32 " %" PRId32 " %" PRId32 " (%u)\n", file,
                        r.lineNo, r.args[0], r.args[1], r.args[2], r.args[3], static_cast<unsigned>(r.numArgs));
#endif
        // Wait for it to go out before the reset. HardwareSerial::flush() also works with interrupts disabled.
        Stream* const console =
            static_cast<Os::Arduino::StreamConsoleHandle*>(Os::Console::getSingleton().getHandle())->m_stream;
        if (console != nullptr) {
            console->flush();
        }
    }
}

void ATmegaReset::Hook ::doAssert() {
    watchdogReset();
}

}  // namespace ATmega128Demo
