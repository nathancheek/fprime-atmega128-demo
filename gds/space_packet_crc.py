"""fprime-gds framing plugin: CCSDS Space Packets with an optional sync word and CRC, no transfer frames.

This is the ground side of ATmega128Demo/Components/ComSpacePacket. Every packet on the link is a CCSDS Space
Packet (no secondary header) whose data field is one F Prime packet. A 2-byte sync word can come before it and
a 2-byte CRC-16/CCITT-FALSE after it:

    [sync word] [Space Packet header] [F Prime packet] [CRC]

Both are outside the Space Packet, so the Space Packet's data length counts only the F Prime packet. The CRC
covers the Space Packet but not the sync word. Commands are telecommands (type 1), telemetry is type 0, and the
APID is the F Prime packet descriptor.

The sync word (ComCfg.SpacePacketSyncWord, 0 for none), whether there is a CRC (ComCfg.SpacePacketCrc) and
the largest F Prime packet (FW_COM_BUFFER_MAX_SIZE) come from the dictionary, so the ground always matches the
deployment it was built with.

To use it, put this directory on the Python path and name the class in FPRIME_GDS_EXTRA_PLUGINS:

    export PYTHONPATH="$PWD/gds"
    export FPRIME_GDS_EXTRA_PLUGINS="space_packet_crc:SpacePacketCrcFramerDeframer"
    fprime-gds ... --framing-selection space-packet-crc
"""

import copy
import struct

from fprime_gds.common.communication.framing import FramerDeframer
from fprime_gds.common.utils.config_manager import ConfigManager
from fprime_gds.plugin.definitions import gds_plugin_implementation


def crc16_ccitt_false(data):
    """CRC-16/CCITT-FALSE: polynomial 0x1021, initial value 0xFFFF, no reflection, no final XOR"""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def packet_descriptor(data):
    """Read the F Prime packet descriptor (the APID) at the start of an F Prime packet"""
    # fprime-gds 4.0 returns a type instance here, 4.1 and later return the type class
    descriptor = ConfigManager().get_type("FwPacketDescriptorType")
    if isinstance(descriptor, type):
        descriptor = descriptor()
    descriptor.deserialize(data, offset=0)
    return descriptor.val


class SpacePacketCrcFramerDeframer(FramerDeframer):
    """Frames commands and deframes telemetry as Space Packets with an optional sync word and CRC-16"""

    HEADER_SIZE = 6
    CRC_SIZE = 2
    SEQUENCE_COUNT_MAXIMUM = 1 << 14
    IDLE_APID = 0x7FF
    TYPE_TC = 0x1000
    SEQ_FLAGS_UNSEGMENTED = 0xC000

    def __init__(self):
        config = ConfigManager()
        sync_word = config.get_constant("ComCfg.SpacePacketSyncWord")
        self.sync = struct.pack(">H", sync_word) if sync_word else b""
        self.crc_size = self.CRC_SIZE if config.get_constant("ComCfg.SpacePacketCrc") else 0
        # Largest F Prime packet in a Space Packet
        self.max_data_size = config.get_constant("FW_COM_BUFFER_MAX_SIZE")
        self.sequence_counts = {}

    def frame(self, data):
        """Wrap an F Prime packet in a telecommand Space Packet, adding the sync word and CRC if enabled"""
        assert 0 < len(data) <= self.max_data_size, f"Packet of {len(data)} bytes exceeds {self.max_data_size}"
        apid = packet_descriptor(data)
        sequence_count = self.sequence_counts.get(apid, 0)
        self.sequence_counts[apid] = (sequence_count + 1) % self.SEQUENCE_COUNT_MAXIMUM
        # Data length is the number of data field bytes minus 1
        header = struct.pack(
            ">HHH",
            self.TYPE_TC | apid,
            self.SEQ_FLAGS_UNSEGMENTED | sequence_count,
            len(data) - 1,
        )
        packet = header + data
        crc = struct.pack(">H", crc16_ccitt_false(packet)) if self.crc_size else b""
        return self.sync + packet + crc

    def deframe(self, data, no_copy=False):
        """Find the next valid telemetry Space Packet, returning (packet, remaining, discarded)"""
        discarded = b""
        if not no_copy:
            data = copy.copy(data)
        sync_size = len(self.sync)
        while len(data) >= sync_size + self.HEADER_SIZE:
            identification, sequence_control, length_token = struct.unpack_from(">HHH", data, sync_size)
            packet_size = self.HEADER_SIZE + length_token + 1
            link_size = sync_size + packet_size + self.crc_size
            # Sync word, then version 0, telemetry, no secondary header, unsegmented, within size
            header_valid = (
                data[:sync_size] == self.sync
                and (identification & 0xF800) == 0
                and (sequence_control & 0xC000) == self.SEQ_FLAGS_UNSEGMENTED
                and length_token + 1 <= self.max_data_size
            )
            if not header_valid:
                discarded += data[:1]
                data = data[1:]
                continue
            if len(data) < link_size:
                break  # Wait for the rest of the packet
            packet_end = sync_size + packet_size
            if self.crc_size and (
                crc16_ccitt_false(data[sync_size:packet_end]) != struct.unpack_from(">H", data, packet_end)[0]
            ):
                discarded += data[:1]
                data = data[1:]
                continue
            packet = data[sync_size + self.HEADER_SIZE : packet_end]
            data = data[link_size:]
            if (identification & 0x7FF) == self.IDLE_APID:
                continue
            return packet, data, discarded
        return None, data, discarded

    @classmethod
    def get_name(cls):
        """Name of this implementation provided to CLI"""
        return "space-packet-crc"

    @classmethod
    @gds_plugin_implementation
    def register_framing_plugin(cls):
        """Register this framer/deframer as a framing plugin"""
        return cls
