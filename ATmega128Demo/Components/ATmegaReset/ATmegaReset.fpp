module ATmega128Demo {

    @ Why the MCU last reset
    enum ResetReason : U8 {
        UNKNOWN = 0 @< No reset flag was set
        POWER_ON = 1
        @ External reset, including uploads and the serial adapter's DTR line. Named RESET_PIN
        @ because Arduino.h defines EXTERNAL as a macro.
        RESET_PIN = 2
        BROWN_OUT = 3
        @ Watchdog timeout: the main loop stopped petting the watchdog. Through urboot this relies
        @ on the rate driver's tick count, so a hang with interrupts disabled reads as RESET_PIN.
        WATCHDOG = 4
        JTAG = 5
        ASSERT = 6 @< FW_ASSERT, which resets through the watchdog
        COMMANDED = 7 @< The REBOOT command
    }

    @ Arguments of the last assert
    array AssertArgs = [4] I32

    @ The assert that caused the last reset
    struct AssertInfo {
        fileId: U32 format "0x{x}" @< Hash of the file's path (look up with fprime-util hash-to-file)
        lineNo: U32
        numArgs: U8 @< Arguments passed to FW_ASSERT; only the first 4 are kept
        args: AssertArgs
    }

    @ Reset handling for ATmega MCUs. Reports why the MCU last reset, resets through the
    @ watchdog on FW_ASSERT (printing the assert on the console and keeping its details in RAM
    @ that survives the reset), and runs the watchdog, which the run port pets.
    passive component ATmegaReset {

        @ Pets the watchdog. Call it well within the 2 s timeout.
        sync input port run: Svc.Sched

        @ Whether the watchdog runs (2 s timeout). Takes effect immediately on PRM_SET.
        param WATCHDOG_ENABLED: Fw.Enabled default Fw.Enabled.ENABLED

        @ Reboot the MCU through the watchdog. ResetReason reads COMMANDED afterwards.
        sync command REBOOT

        @ Cause an assert with the given arguments, to test assert handling
        sync command TEST_ASSERT(
            arg1: I32
            arg2: I32
        )

        @ Stop the main loop to test the watchdog. Fails if the watchdog is disabled.
        sync command TEST_HANG

        @ Why the MCU last reset
        telemetry ResetReason: ResetReason id 0

        @ Reset flags at boot, in MCUCSR bit order: JTRF 0x10, WDRF 0x08, BORF 0x04, EXTRF 0x02, PORF 0x01
        telemetry ResetFlags: U8 id 1 format "0x{x}"

        @ The assert that caused the last reset, or all zeros if the last reset was not an assert
        telemetry LastAssert: AssertInfo id 2

        @ Whether the watchdog is running
        telemetry WatchdogEnabled: Fw.Enabled id 3

        @ Port for requesting the current time
        time get port timeCaller

        @ Port for sending command registrations
        command reg port cmdRegOut

        @ Port for receiving commands
        command recv port cmdIn

        @ Port for sending command responses
        command resp port cmdResponseOut

        @ Port for sending telemetry channels to downlink
        telemetry port tlmOut

        @ Port to return the value of a parameter
        param get port prmGetOut

        @ Port to set the value of a parameter
        param set port prmSetOut
    }
}
