# ATmega128 F´ Demo

A lightweight [F´ (F Prime)](https://fprime.jpl.nasa.gov) deployment for the ATmega128, an 8-bit AVR with 128 KB of
flash and up to 64 KB of SRAM (using XMEM). It starts from the
[ATmega128 deployment cookiecutter](https://github.com/nathancheek/fprime-atmega128-deployment-cookiecutter) and
replaces its communications stack and active components with smaller ones, which leaves room for real
flight-software features: parameters in EEPROM, reset and assert handling, a watchdog, and sensor readout.

For the F´ LED blinker tutorial on the same board, see
[fprime-atmega128-blinker](https://github.com/nathancheek/fprime-atmega128-blinker).

## A lighter CCSDS communications stack

The Space Packet link (F´ GDS on the bench, another subsystem in flight) carries CCSDS Space Packets, each starting with
a 2-byte sync word (0xC1F5) and ending in a CRC-16. Sync word, CRC, and APIDs are all configurable in
`config/ComCfg.fpp`. The comms stack skips TC/TM transfer frames, since those are built for space-to-ground links and
add unnecessary complexity.

* `ComSpacePacket` replaces the cookiecutter's `ComFprime` subtopology, with no buffer manager or com queue.
* There are no active components: everything runs from the rate groups in `loop()`, using `LiteCmdDispatcher` and
  `PassiveTlmPacketizer`.
* There are no events. Command results show up in the `cmdDisp.CommandsDispatched` and `cmdDisp.CommandErrors`
  channels.
* All telemetry is sent once per second via packetized telemetry packets.
* F´ GDS reads the link through the framing plugin in `gds/space_packet_crc.py`.

## Other features

The components are in `ATmega128Demo/Components`.

* **Parameters in EEPROM.** `EepromPrmDb` saves parameters to EEPROM in the background and checks them at boot,
  falling back to defaults if they're corrupt. See its [SDD](ATmega128Demo/Components/EepromPrmDb/docs/sdd.md).
* **Reset reason, asserts and watchdog.** `ATmegaReset` reports why the board last reset, prints asserts on the console
  before resetting, reboots on command and runs a watchdog.
* **ADC and thermistors.** `Adc128s102` reads a TI ADC128S102 over SPI, and `Thermistors` converts two of its channels
  to degrees C.
* **Heartbeat LED.** fprime-arduino's `LifeLed` blinks the built-in LED.

## Hardware

* ATmega128 with [MegaCore](https://github.com/MCUdude/MegaCore) and a bootloader that uploads over UART0 (for
  example urboot)
* 7.3728 MHz external crystal
* 64 KB of external SRAM on the XMEM interface
* A serial adapter on UART0 for programming and console output, and another on UART1 for the Space Packet link
* Optional: a TI ADC128S102 on the SPI bus with chip select on PB4, and NTC thermistors on IN2 and IN5

## Getting started

Clone the project with its submodules, then set up a virtual environment:

```sh
git clone --recurse-submodules https://github.com/nathancheek/fprime-atmega128-demo.git
cd fprime-atmega128-demo
python3 -m venv fprime-venv
. fprime-venv/bin/activate
pip install -r requirements.txt
```

Then follow the [arduino-cli installation guide](https://github.com/fprime-community/fprime-arduino/blob/main/docs/arduino-cli-install.md)
and install MegaCore:

```sh
arduino-cli core install MegaCore:avr --additional-urls https://mcudude.github.io/MegaCore/package_MCUdude_MegaCore_index.json
```

Building, flashing and running F´ GDS are covered in the
[deployment's README](ATmega128Demo/ATmega128DemoDeployment/README.md).
