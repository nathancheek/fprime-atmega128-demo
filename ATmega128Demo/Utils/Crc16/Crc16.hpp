// ======================================================================
// \title  Crc16.hpp
// \brief  CRC-16/CCITT-FALSE, shared by ComSpacePacket and EepromPrmDb
// ======================================================================

#ifndef ATmega128Demo_Crc16_HPP
#define ATmega128Demo_Crc16_HPP

#include "Fw/Types/BasicTypes.hpp"

namespace ATmega128Demo {
namespace Crc16 {

//! Starting value of every CRC
constexpr U16 INITIAL = 0xFFFF;

//! Fold one byte into a CRC-16/CCITT-FALSE (polynomial 0x1021, initial value 0xFFFF, no final XOR), the CCSDS
//! and ECSS PUS packet error control CRC. A nibble table keeps it to 32 bytes instead of the 512-byte table in
//! Utils/Hash/libcrc.
U16 update(U16 crc, U8 byte);

}  // namespace Crc16
}  // namespace ATmega128Demo

#endif
