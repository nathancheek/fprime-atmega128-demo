module ATmega128Demo {

  # ----------------------------------------------------------------------
  # Passive component instances
  #
  # No active components: everything runs from the rate groups in loop().
  # ----------------------------------------------------------------------

  instance cmdDisp: ATmega128Demo.LiteCmdDispatcher base id 0x0100

  instance tlmSend: ATmega128Demo.PassiveTlmPacketizer base id 0x0400

  instance prmDb: ATmega128Demo.EepromPrmDb base id 0x0700

  instance resetHandler: ATmega128Demo.ATmegaReset base id 0x0800

  instance lifeLed: Arduino.LifeLed base id 0x0E00

  instance rateGroup10Hz: Svc.PassiveRateGroup base id 0x1000

  instance rateGroup1Hz: Svc.PassiveRateGroup base id 0x1100

  instance comSpacePacket: ATmega128Demo.ComSpacePacket base id 0x2000

  instance adc: ATmega128Demo.Adc128s102 base id 0x2100

  instance thermistors: ATmega128Demo.Thermistors base id 0x2200

  @ Communications driver. May be swapped with other com drivers like Arduino.StreamDriver, Arduino.TcpServer, or Arduino.TcpClient.
  instance comDriver: Arduino.StreamDriver base id 0x4000

  instance timeHandler: Arduino.ArduinoTime base id 0x4400

  instance rateGroupDriver: Svc.RateGroupDriver base id 0x4500

  instance rateDriver: Arduino.HardwareRateDriver base id 0x4900

  @ SPI bus to the ADC128S102, chip select on PB4
  instance spiDriver: Arduino.SpiDriver base id 0x5100

}
