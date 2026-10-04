module ATmega128Demo {
    @ Passive telemetry packetizer for small targets. Holds the latest value of each channel in the
    @ packets laid out by the topology's telemetry packet definition, and sends every packet
    @ (FW_PACKET_PACKETIZED_TLM, one timestamp per packet) each time Run is called.
    @ Channels that are not in any packet are ignored.
    passive component PassiveTlmPacketizer {
        @ Telemetry values from components
        sync input port TlmRecv: Fw.Tlm

        @ Sends the packets
        sync input port Run: Svc.Sched

        @ Telemetry packet output
        output port PktSend: Fw.Com

        @ Port for requesting the current time
        time get port timeCaller
    }
}
