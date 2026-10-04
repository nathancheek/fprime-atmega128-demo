// ======================================================================
// \title  Adc128s102.hpp
// \brief  hpp file for Adc128s102 component implementation class
// ======================================================================

#ifndef ATmega128Demo_Adc128s102_HPP
#define ATmega128Demo_Adc128s102_HPP

#include "ATmega128Demo/Components/Adc128s102/Adc128s102ComponentAc.hpp"

namespace ATmega128Demo {

class Adc128s102 final : public Adc128s102ComponentBase {
  public:
    static constexpr U8 NUM_CHANNELS = 8;

    Adc128s102(const char* const compName);
    ~Adc128s102();

  private:
    void run_handler(FwIndexType portNum, U32 context) override;

    //! Two 16-bit frames per channel (see run_handler), plus one more because each frame returns
    //! the channel addressed in the frame before it
    static constexpr U8 NUM_FRAMES = 2 * NUM_CHANNELS + 1;

    U8 m_transfer[2 * NUM_FRAMES];
    U32 m_readErrors;
};

}  // namespace ATmega128Demo

#endif
