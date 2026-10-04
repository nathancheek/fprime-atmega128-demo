module ATmega128Demo {
    @ Converts ADC128S102 counts from NTC thermistor dividers to temperatures. Each thermistor is the top
    @ of a divider from the ADC's reference (VA), with FIXED_RESISTANCE below it, so the conversion is
    @ ratiometric and doesn't depend on VA. The temperature comes from the beta equation:
    @ 1/T = 1/T0 + ln(R / R0) / BETA, with T in kelvin.
    passive component Thermistors {

        @ Temperatures of the thermistors, in configure() order
        array TemperatureArray = [2] F32 format "{.2f}"

        @ ADC counts to convert
        sync input port countsIn: Adc128s102CountsPort

        @ Thermistor resistance at NOMINAL_TEMPERATURE (R0), in ohms
        param NOMINAL_RESISTANCE: F32 default 10000.0

        @ Temperature at which the thermistor has NOMINAL_RESISTANCE (T0), in degrees C
        param NOMINAL_TEMPERATURE: F32 default 25.0

        @ Beta constant of the thermistor, in kelvin (the B25/50 value of the datasheet)
        param BETA: F32 default 3380.0

        @ Resistor between the ADC input and ground, in ohms
        param FIXED_RESISTANCE: F32 default 43000.0

        @ Thermistor temperatures in degrees C. NaN when the count is at either end of the ADC range
        @ (thermistor open or shorted)
        telemetry Temperatures: TemperatureArray id 0

        @ Current value of the NOMINAL_RESISTANCE parameter
        telemetry NominalResistance: F32 id 1 format "{.1f}"

        @ Current value of the NOMINAL_TEMPERATURE parameter
        telemetry NominalTemperature: F32 id 2 format "{.2f}"

        @ Current value of the BETA parameter
        telemetry Beta: F32 id 3 format "{.1f}"

        @ Current value of the FIXED_RESISTANCE parameter
        telemetry FixedResistance: F32 id 4 format "{.1f}"

        @ Port for requesting the current time
        time get port timeCaller

        @ Port for sending command registrations
        command reg port cmdRegOut

        @ Port for receiving commands
        command recv port cmdIn

        @ Port for sending command responses
        command resp port cmdResponseOut

        @ Port to return the value of a parameter
        param get port prmGetOut

        @ Port to set the value of a parameter
        param set port prmSetOut

        @ Port for sending telemetry channels to downlink
        telemetry port tlmOut
    }
}
