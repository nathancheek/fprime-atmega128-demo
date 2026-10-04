# Every packet is sent at 1 Hz by tlmSend, at most config/TlmPacketizerConfig's MAX_PACKETIZER_PACKETS.
# Keep each packet's channel values within PassiveTlmPacketizer::MAX_VALUES_SIZE (103 bytes).
telemetry packets ATmega128DemoPackets {

  packet Telemetry id 1 group 1 {
    cmdDisp.CommandsDispatched
    cmdDisp.CommandErrors
    comSpacePacket.BytesReceived
    comSpacePacket.BytesSent
    comSpacePacket.BytesDiscarded
    prmDb.Status

    resetHandler.ResetReason
    resetHandler.ResetFlags
    resetHandler.LastAssert
    resetHandler.WatchdogEnabled

    rateGroup10Hz.CycleCount
    rateGroup10Hz.CycleTime
    rateGroup10Hz.MaxCycleTime
    rateGroup1Hz.CycleCount
    rateGroup1Hz.CycleTime
    rateGroup1Hz.MaxCycleTime

  }

  packet Adc id 2 group 1 {
    adc.Counts
    adc.ReadErrors
    thermistors.Temperatures
    thermistors.NominalResistance
    thermistors.NominalTemperature
    thermistors.Beta
    thermistors.FixedResistance
  }

} omit {
  # Per-port cycle times (new in fprime v4.4) are turned off in config/PassiveRateGroupCfg.hpp
  rateGroup10Hz.PortCycleTime
  rateGroup10Hz.PortCycleTimeHWM
  rateGroup1Hz.PortCycleTime
  rateGroup1Hz.PortCycleTimeHWM
}
