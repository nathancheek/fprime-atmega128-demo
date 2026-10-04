module ATmega128Demo {
    @ Raw 12-bit ADC128S102 conversion results for IN0 to IN7
    array Adc128s102Counts = [8] U16

    @ Passes a set of ADC128S102 conversion results
    port Adc128s102CountsPort(counts: Adc128s102Counts)

    @ Reads all 8 channels of a TI ADC128S102 (8-channel, 12-bit SAR ADC) over SPI each time run is
    @ called, telemeters the raw counts, and passes them on through countsOut. A count is
    @ VA * counts / 4096 volts.
    passive component Adc128s102 {

        @ Reads the 8 channels
        sync input port run: Svc.Sched

        @ SPI transfer to the ADC. The driver must hold chip select low for the whole transfer.
        output port spiWriteRead: Drv.SpiWriteRead

        @ The counts from each successful read, for components that convert them
        output port countsOut: Adc128s102CountsPort

        @ Latest conversion results for IN0 to IN7, in counts (0 to 4095)
        telemetry Counts: Adc128s102Counts id 0

        @ Reads that failed: an SPI error, or a frame whose 4 leading bits were not zero
        telemetry ReadErrors: U32 id 1

        @ Port for requesting the current time
        time get port timeCaller

        @ Port for sending telemetry channels to downlink
        telemetry port tlmOut
    }
}
