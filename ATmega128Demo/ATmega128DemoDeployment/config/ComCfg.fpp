# ======================================================================
# FPP file for configuration of the communications stack
# ======================================================================

@ The width of packet descriptors when they are serialized by the framework
dictionary type FwPacketDescriptorType = U16

module ComCfg {

    @ Sync word written before each Space Packet on the ATmega128Demo.ComSpacePacket link (big endian), or 0
    @ for none. F´ GDS reads it from the dictionary (gds/space_packet_crc.py).
    dictionary constant SpacePacketSyncWord = 0xC1F5

    @ 1 to follow each Space Packet on the ATmega128Demo.ComSpacePacket link with a CRC-16/CCITT-FALSE over the
    @ packet, 0 for none. An integer, since FPP bool constants aren't usable in C++ constant expressions. F´ GDS
    @ reads it from the dictionary (gds/space_packet_crc.py).
    dictionary constant SpacePacketCrc = 1

    @ APIDs are 11 bits in the Space Packet protocol, so we use U16. Max value 7FF
    dictionary enum Apid : FwPacketDescriptorType {
        # APIDs prefixed with FW are reserved for F Prime and need to be present
        # in the enumeration. Their values can be changed
        FW_PACKET_COMMAND        = 0x0000  @< Command packet type - incoming
        FW_PACKET_TELEM          = 0x0001  @< Telemetry packet type - outgoing
        FW_PACKET_LOG            = 0x0002  @< Log type - outgoing
        FW_PACKET_FILE           = 0x0003  @< File type - incoming and outgoing
        FW_PACKET_PACKETIZED_TLM = 0x0004  @< Packetized telemetry packet type
        FW_PACKET_DP             = 0x0005  @< Data Product packet type
        FW_PACKET_IDLE           = 0x0006  @< F Prime idle
        FW_PACKET_PARAM          = 0x0007  @< Parameter value type - outgoing
        FW_PACKET_HAND           = 0x00FE  @< F Prime handshake
        FW_PACKET_UNKNOWN        = 0x00FF  @< F Prime unknown packet
        SPP_IDLE_PACKET          = 0x07FF  @< Per Space Packet Standard, all 1s (11bits) is reserved for Idle Packets
        INVALID_UNINITIALIZED    = 0x0800  @< Anything equal or higher value is invalid and should not be used
    } default INVALID_UNINITIALIZED

    @ Not used by this deployment, which has no TM frames. It must still be defined: Svc/Ccsds/Types.fpp has
    @ dictionary enums, so FPP pulls it into every deployment.
    dictionary constant TmFrameFixedSize = 128

}
