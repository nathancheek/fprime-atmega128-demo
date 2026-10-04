// ======================================================================
// \title  Crc16.cpp
// \brief  CRC-16/CCITT-FALSE, shared by ComSpacePacket and EepromPrmDb
// ======================================================================

#include "ATmega128Demo/Utils/Crc16/Crc16.hpp"

namespace ATmega128Demo {
namespace Crc16 {

namespace {
const U16 NIBBLE_TABLE[16] = {0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
                              0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF};
}  // namespace

U16 update(U16 crc, U8 byte) {
    crc = static_cast<U16>((crc << 4) ^ NIBBLE_TABLE[(crc >> 12) ^ (byte >> 4)]);
    crc = static_cast<U16>((crc << 4) ^ NIBBLE_TABLE[(crc >> 12) ^ (byte & 0x0F)]);
    return crc;
}

}  // namespace Crc16
}  // namespace ATmega128Demo
