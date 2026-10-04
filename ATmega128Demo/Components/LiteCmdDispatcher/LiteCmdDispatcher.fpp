module ATmega128Demo {
    @ Minimal passive command dispatcher for small targets. Routes command packets to the
    @ component that registered each opcode. No events and no sequence tracking: command
    @ results are reported as telemetry counts instead.
    passive component LiteCmdDispatcher {

        @ Command registration input ports. Size must match the dispatch output ports.
        sync input port compCmdReg: [CmdDispatcherComponentCommandPorts] Fw.CmdReg

        @ Command dispatch output ports. Size must match the registration input ports.
        output port compCmdSend: [CmdDispatcherComponentCommandPorts] Fw.Cmd

        @ Command responses from components
        sync input port compCmdStat: Fw.CmdResponse

        @ Command packets to dispatch
        sync input port seqCmdBuff: Fw.Com

        match compCmdSend with compCmdReg

        @ No-op command
        sync command CMD_NO_OP opcode 0

        @ Commands sent to a component
        telemetry CommandsDispatched: U32 id 0

        @ Commands rejected (bad packet or unknown opcode) or completed with an error
        telemetry CommandErrors: U32 id 1

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
