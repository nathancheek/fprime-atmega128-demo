module ATmega128Demo {

  # ----------------------------------------------------------------------
  # Symbolic constants for port numbers
  # ----------------------------------------------------------------------

    enum Ports_RateGroups {
      rateGroup10Hz
      rateGroup1Hz
    }

  deployment topology ATmega128DemoDeployment {

    # ----------------------------------------------------------------------
    # Instances used in the topology
    # ----------------------------------------------------------------------

    instance cmdDisp
    instance comDriver
    instance comSpacePacket
    instance rateDriver
    instance rateGroup10Hz
    instance rateGroup1Hz
    instance rateGroupDriver
    instance timeHandler
    instance tlmSend
    instance lifeLed
    instance prmDb
    instance resetHandler
    instance adc
    instance spiDriver
    instance thermistors

    # ----------------------------------------------------------------------
    # Telemetry packets
    # ----------------------------------------------------------------------

    include "ATmega128DemoPackets.fppi"

    # ----------------------------------------------------------------------
    # Pattern graph specifiers
    # ----------------------------------------------------------------------

    command connections instance cmdDisp

    param connections instance prmDb

    telemetry connections instance tlmSend

    time connections instance timeHandler

    # ----------------------------------------------------------------------
    # Direct graph specifiers
    # ----------------------------------------------------------------------

    connections RateGroups {
      # Timer1 overflow-based interrupt. Rate groups run from the main loop.
      rateDriver.CycleOut -> rateGroupDriver.CycleIn

      # 10 Hz rate group: poll the link UART
      rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup10Hz] -> rateGroup10Hz.CycleIn
      rateGroup10Hz.RateGroupMemberOut[0] -> comDriver.schedIn
      # Pet the watchdog (2 s timeout)
      rateGroup10Hz.RateGroupMemberOut[1] -> resetHandler.run
      # Blink the built-in LED once a second (LED_PERIOD of 10 ticks)
      rateGroup10Hz.RateGroupMemberOut[2] -> lifeLed.run
      # Write queued parameter saves to EEPROM, one byte per tick
      rateGroup10Hz.RateGroupMemberOut[3] -> prmDb.run

      # 1 Hz rate group: ADC and telemetry. The ADC is read first so its values go out in the same cycle.
      rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup1Hz] -> rateGroup1Hz.CycleIn
      rateGroup1Hz.RateGroupMemberOut[0] -> adc.run
      rateGroup1Hz.RateGroupMemberOut[1] -> tlmSend.Run
    }

    connections Communications {
      # ComDriver <-> ComSpacePacket. ComSpacePacket lends the driver its receive buffer.
      comDriver.allocate          -> comSpacePacket.drvAllocateIn
      comDriver.deallocate        -> comSpacePacket.drvDeallocateIn
      comDriver.$recv             -> comSpacePacket.drvReceiveIn
      comSpacePacket.drvReceiveReturnOut -> comDriver.recvReturnIn
      comSpacePacket.drvSendOut   -> comDriver.$send

      # Received commands
      comSpacePacket.comCmdOut -> cmdDisp.seqCmdBuff

      # Sent telemetry
      tlmSend.PktSend -> comSpacePacket.comIn
    }

    connections ATmega128DemoDeployment {
      # Add here connections to user-defined components
      # ADC128S102 on the SPI bus
      adc.spiWriteRead -> spiDriver.SpiWriteRead
      # Thermistors on IN2 and IN5 (see configureTopology)
      adc.countsOut -> thermistors.countsIn
    }

  }

}
