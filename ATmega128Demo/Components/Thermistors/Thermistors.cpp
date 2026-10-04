// ======================================================================
// \title  Thermistors.cpp
// \brief  cpp file for Thermistors component implementation class
// ======================================================================

#include "ATmega128Demo/Components/Thermistors/Thermistors.hpp"
#include <math.h>
#include "Fw/Types/Assert.hpp"

namespace ATmega128Demo {

namespace {
constexpr F32 KELVIN_OFFSET = 273.15f;
constexpr U16 ADC_FULL_SCALE = 4096;  // 12-bit: the count is VA * counts / 4096
}  // namespace

Thermistors ::Thermistors(const char* const compName) : ThermistorsComponentBase(compName), m_channels{} {}

Thermistors ::~Thermistors() {}

void Thermistors ::configure(const U8 (&channels)[NUM_THERMISTORS]) {
    for (U8 i = 0; i < NUM_THERMISTORS; i++) {
        FW_ASSERT(channels[i] < Adc128s102Counts::SIZE, static_cast<FwAssertArgType>(channels[i]));
        this->m_channels[i] = channels[i];
    }
}

void Thermistors ::countsIn_handler(FwIndexType portNum, const Adc128s102Counts& counts) {
    Fw::ParamValid valid;
    const F32 nominalResistance = this->paramGet_NOMINAL_RESISTANCE(valid);
    const F32 nominalTemperature = this->paramGet_NOMINAL_TEMPERATURE(valid);
    const F32 beta = this->paramGet_BETA(valid);
    const F32 fixedResistance = this->paramGet_FIXED_RESISTANCE(valid);

    Thermistors_TemperatureArray temperatures;
    for (U8 i = 0; i < NUM_THERMISTORS; i++) {
        temperatures[i] =
            this->toTemperature(counts[this->m_channels[i]], nominalResistance, nominalTemperature, beta, fixedResistance);
    }
    this->tlmWrite_Temperatures(temperatures);
}

void Thermistors ::parameterUpdated(FwPrmIdType id) {
    Fw::ParamValid valid;
    switch (id) {
        case PARAMID_NOMINAL_RESISTANCE:
            this->tlmWrite_NominalResistance(this->paramGet_NOMINAL_RESISTANCE(valid));
            break;
        case PARAMID_NOMINAL_TEMPERATURE:
            this->tlmWrite_NominalTemperature(this->paramGet_NOMINAL_TEMPERATURE(valid));
            break;
        case PARAMID_BETA:
            this->tlmWrite_Beta(this->paramGet_BETA(valid));
            break;
        case PARAMID_FIXED_RESISTANCE:
            this->tlmWrite_FixedResistance(this->paramGet_FIXED_RESISTANCE(valid));
            break;
        default:
            FW_ASSERT(0, static_cast<FwAssertArgType>(id));
            break;
    }
}

F32 Thermistors ::toTemperature(U16 count,
                                F32 nominalResistance,
                                F32 nominalTemperature,
                                F32 beta,
                                F32 fixedResistance) const {
    // 0 means the thermistor is open, full scale means it is shorted
    if ((count == 0) || (count >= ADC_FULL_SCALE - 1)) {
        return NAN;
    }
    // count / 4096 = Rfixed / (R + Rfixed), so R = Rfixed * (4096 - count) / count
    const F32 resistance = fixedResistance * static_cast<F32>(ADC_FULL_SCALE - count) / static_cast<F32>(count);
    // Beta equation: 1/T = 1/T0 + ln(R / R0) / beta, with temperatures in kelvin
    const F32 inverseKelvin = 1.0f / (nominalTemperature + KELVIN_OFFSET) + logf(resistance / nominalResistance) / beta;
    return 1.0f / inverseKelvin - KELVIN_OFFSET;
}

}  // namespace ATmega128Demo
