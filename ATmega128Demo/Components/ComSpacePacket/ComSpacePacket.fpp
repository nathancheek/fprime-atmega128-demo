module ATmega128Demo {
    @ Minimal Space Packet link for small targets, meant for a short on-board link rather than
    @ space-to-ground: there are no TC/TM transfer frames.
    @
    @ Each packet on the link is a CCSDS Space Packet whose data field is one F Prime packet.
    @ ComCfg.SpacePacketSyncWord and ComCfg.SpacePacketCrc add a 2-byte sync word before it and a
    @ 2-byte CRC-16 after it:
    @
    @     [sync word] [Space Packet header] [F Prime packet] [CRC]
    @
    @ Both are outside the Space Packet, so the Space Packet's data length counts only the
    @ F Prime packet. The CRC covers the Space Packet but not the sync word.
    @
    @ Receive: finds Space Packets in the received byte stream, checks the sync word and CRC
    @ when enabled, and sends each command packet to the command dispatcher.
    @ Send: wraps each packet in a Space Packet and writes it to the driver.
    @
    @ Passive and allocation-free. It lends the byte stream driver its own receive buffer,
    @ standing in for a buffer manager.
    passive component ComSpacePacket {

        # ----------------------------------------------------------------------
        # Byte stream driver
        # ----------------------------------------------------------------------

        @ Writes packet pieces to the driver
        output port drvSendOut: Drv.ByteStreamSend

        @ Bytes received by the driver
        sync input port drvReceiveIn: Drv.ByteStreamData

        @ Returns received buffers to the driver
        output port drvReceiveReturnOut: Fw.BufferSend

        @ Lends the driver space in the receive buffer
        sync input port drvAllocateIn: Fw.BufferGet

        @ Takes back buffers lent to the driver
        sync input port drvDeallocateIn: Fw.BufferSend

        # ----------------------------------------------------------------------
        # Packets
        # ----------------------------------------------------------------------

        @ Command packets to the command dispatcher
        output port comCmdOut: Fw.Com

        @ Packets to send
        sync input port comIn: Fw.Com

        # ----------------------------------------------------------------------
        # Telemetry
        # ----------------------------------------------------------------------

        @ Bytes received from the driver
        telemetry BytesReceived: U32

        @ Bytes written to the driver, sync words, headers and CRCs included
        telemetry BytesSent: U32

        @ Received bytes discarded because they were not part of a valid packet
        telemetry BytesDiscarded: U32

        @ Port for requesting the current time
        time get port timeCaller

        @ Port for sending telemetry channels to downlink
        telemetry port tlmOut
    }
}
