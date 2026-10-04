module ATmega128Demo {

    @ State of the EEPROM parameter database
    enum PrmDbStatus : U8 {
        EMPTY = 0 @< No saved parameters
        OK = 1 @< Saved parameters loaded, and the last save (if any) was successful
        WRITING = 2 @< A save or CLEAR_DB is being written
        SAVE_FAILED = 3 @< One or more saves weren't stored
        FULL = 4 @< The last attempted save didn't fit in the EEPROM region
        CORRUPT = 5 @< The database is invalid
    }

    @ Parameter database stored in AVR on-chip EEPROM
    passive component EepromPrmDb {

        @ Parameter get requests
        sync input port getPrm: Fw.PrmGet

        @ Parameter save requests
        sync input port setPrm: Fw.PrmSet

        @ Writes queued EEPROM bytes, one per call
        sync input port run: Svc.Sched

        @ Forget all stored parameters. Components keep their current values until the next boot.
        sync command CLEAR_DB opcode 0

        @ State of the database
        telemetry Status: PrmDbStatus id 0

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
    }
}
