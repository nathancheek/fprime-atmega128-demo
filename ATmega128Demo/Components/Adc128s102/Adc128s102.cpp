// ======================================================================
// \title  Adc128s102.cpp
// \brief  cpp file for Adc128s102 component implementation class
// ======================================================================

#include "ATmega128Demo/Components/Adc128s102/Adc128s102.hpp"

namespace ATmega128Demo {

// Frame format (MSB first). DIN: 2 don't-care bits, the 3-bit address of the next channel, then 11
// don't-care bits. DOUT: 4 zero bits, then the 12-bit result for the channel addressed in the previous
// frame. The first frame after chip select falls converts whatever channel was addressed last.

Adc128s102 ::Adc128s102(const char* const compName) : Adc128s102ComponentBase(compName), m_readErrors(0) {}

Adc128s102 ::~Adc128s102() {}

void Adc128s102 ::run_handler(FwIndexType portNum, U32 context) {
    // Each channel is converted twice in a row and the second result kept. A single conversion straight
    // after another channel reads up to about 20 counts off on the thermistor inputs, because the sampling
    // capacitor doesn't fully settle through the source impedance in one track period.
    // Frame f addresses IN(f / 2) and returns the channel addressed in frame f - 1, so the kept result for
    // IN(ch) is in frame 2 * ch + 2. The last frame addresses IN0 again; its result is not needed, and it
    // leaves IN0 selected for the start of the next read.
    for (U8 f = 0; f < NUM_FRAMES; f++) {
        this->m_transfer[2 * f] = static_cast<U8>(((f / 2) % NUM_CHANNELS) << 3);
        this->m_transfer[2 * f + 1] = 0;
    }
    Fw::Buffer buffer(this->m_transfer, sizeof(this->m_transfer));
    const Drv::SpiStatus status = this->spiWriteRead_out(0, buffer, buffer);
    if (status != Drv::SpiStatus::SPI_OK) {
        this->tlmWrite_ReadErrors(++this->m_readErrors);
        return;
    }

    // Every frame after the first must start with 4 zero bits
    bool formatOk = true;
    for (U8 f = 1; f < NUM_FRAMES; f++) {
        formatOk = formatOk && ((this->m_transfer[2 * f] & 0xF0) == 0);
    }
    Adc128s102Counts counts;
    for (U8 ch = 0; ch < NUM_CHANNELS; ch++) {
        const U8* frame = &this->m_transfer[2 * (2 * ch + 2)];
        counts[ch] = static_cast<U16>(((frame[0] & 0x0F) << 8) | frame[1]);
    }
    if (!formatOk) {
        this->tlmWrite_ReadErrors(++this->m_readErrors);
    }
    this->tlmWrite_Counts(counts);
    if (this->isConnected_countsOut_OutputPort(0)) {
        this->countsOut_out(0, counts);
    }
}

}  // namespace ATmega128Demo
