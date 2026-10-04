// ======================================================================
// \title  Thermistors.hpp
// \brief  hpp file for Thermistors component implementation class
// ======================================================================

#ifndef ATmega128Demo_Thermistors_HPP
#define ATmega128Demo_Thermistors_HPP

#include "ATmega128Demo/Components/Thermistors/ThermistorsComponentAc.hpp"

namespace ATmega128Demo {

class Thermistors final : public ThermistorsComponentBase {
  public:
    static constexpr U8 NUM_THERMISTORS = 2;

    Thermistors(const char* const compName);
    ~Thermistors();

    //! Set the ADC channel of each thermistor
    void configure(const U8 (&channels)[NUM_THERMISTORS]);

  private:
    void countsIn_handler(FwIndexType portNum, const Adc128s102Counts& counts) override;

    //! Reports the new value in telemetry. Also called for each parameter at boot by loadParameters().
    void parameterUpdated(FwPrmIdType id) override;

    //! Temperature in degrees C for an ADC count, or NaN at either end of the ADC range
    F32 toTemperature(U16 count, F32 nominalResistance, F32 nominalTemperature, F32 beta, F32 fixedResistance) const;

    U8 m_channels[NUM_THERMISTORS];
};

}  // namespace ATmega128Demo

#endif
