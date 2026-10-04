// ======================================================================
// \title  Main.cpp
// \brief main program for the F' application. Intended for Arduino-based systems
//
// ======================================================================
// Used to access topology functions
#include <ATmega128Demo/ATmega128DemoDeployment/Top/ATmega128DemoDeploymentTopologyAc.hpp>
#include <ATmega128Demo/ATmega128DemoDeployment/Top/ATmega128DemoDeploymentTopology.hpp>

// Used for Baremetal TaskRunner
#include <fprime-baremetal/Os/TaskRunner/TaskRunner.hpp>

// Used for logging
#include <Arduino/Os/Console.hpp>

// Enables the external SRAM interface at startup
#include <ATmega/ATmegaOs/AVR/XMem.hpp>

/**
 * \brief setup the program
 *
 * This is an extraction of the Arduino setup() function.
 * 
 */
void setup() {
    // Initialize OSAL
    Os::init();

    // Setup console logging on UART0 (the programming port)
    Serial.begin(115200);
    static_cast<Os::Arduino::StreamConsoleHandle*>(Os::Console::getSingleton().getHandle())->setStreamHandler(Serial);

    // Object for communicating state to the reference topology
    ATmega128Demo::TopologyState inputs;
    inputs.uartNumber = 1;
    inputs.uartBaud = 115200;

    // Setup topology
    ATmega128Demo::setupTopology(inputs);

    Fw::Logger::log("Program Started\n");
}

/**
 * \brief run the program
 *
 * This is an extraction of the Arduino loop() function.
 * 
 */
void loop() {
#ifdef USE_BASIC_TIMER
    rateDriver.cycle();
#endif
    Os::Baremetal::TaskRunner::getSingleton().run();
}
