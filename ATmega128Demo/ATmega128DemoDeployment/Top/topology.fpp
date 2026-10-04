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
    # Subtopology imports
    # ----------------------------------------------------------------------


    import ComFprime.Subtopology

    # ----------------------------------------------------------------------
    # Instances used in the topology
    # ----------------------------------------------------------------------

    instance cmdDisp
    instance comDriver
    instance eventLogger
    instance fatalHandler
    instance rateDriver
    instance rateGroup10Hz
    instance rateGroup1Hz
    instance rateGroupDriver
    instance timeHandler
    instance tlmSend

    # ----------------------------------------------------------------------
    # Pattern graph specifiers
    # ----------------------------------------------------------------------

    command connections instance cmdDisp

    event connections instance eventLogger

    telemetry connections instance tlmSend

    time connections instance timeHandler

    # ----------------------------------------------------------------------
    # Direct graph specifiers
    # ----------------------------------------------------------------------

    connections RateGroups {
      # Timer1 overflow-based interrupt. Rate groups run from the main loop.
      rateDriver.CycleOut -> rateGroupDriver.CycleIn

      # 10 Hz rate group: poll the ground UART for uplink
      rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup10Hz] -> rateGroup10Hz.CycleIn
      rateGroup10Hz.RateGroupMemberOut[0] -> comDriver.schedIn

      # 1 Hz rate group: keep downlink traffic low enough for the ComQueue to keep up
      rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup1Hz] -> rateGroup1Hz.CycleIn
      rateGroup1Hz.RateGroupMemberOut[0] -> tlmSend.Run
      rateGroup1Hz.RateGroupMemberOut[1] -> cmdDisp.run
    }

    connections FaultProtection {
      eventLogger.FatalAnnounce -> fatalHandler.FatalReceive
    }


    connections Communications {
      # Inputs to ComQueue (events, telemetry, file)
      eventLogger.PktSend -> ComFprime.comQueue.comPacketQueueIn[ComFprime.Ports_ComPacketQueue.EVENTS]
      tlmSend.PktSend     -> ComFprime.comQueue.comPacketQueueIn[ComFprime.Ports_ComPacketQueue.TELEMETRY]

      # ComDriver buffer allocations
      comDriver.allocate      -> ComFprime.commsBufferManager.bufferGetCallee
      comDriver.deallocate    -> ComFprime.commsBufferManager.bufferSendIn
      
      # ComDriver <-> ComStub (Uplink)
      comDriver.$recv                     -> ComFprime.comStub.drvReceiveIn
      ComFprime.comStub.drvReceiveReturnOut -> comDriver.recvReturnIn
      
      # ComStub <-> ComDriver (Downlink)
      ComFprime.comStub.drvSendOut      -> comDriver.$send
      comDriver.ready         -> ComFprime.comStub.drvConnected

      # Router <-> CmdDispatcher
      ComFprime.fprimeRouter.commandOut  -> cmdDisp.seqCmdBuff
      cmdDisp.seqCmdStatus     -> ComFprime.fprimeRouter.cmdResponseIn
    }

    connections ATmega128DemoDeployment {
      # Add here connections to user-defined components
    }

  }

}
