// ======================================================================
// \title  ATmega128DemoDeploymentTopology.cpp
// \brief cpp file containing the topology instantiation code
//
// ======================================================================
// Provides access to autocoded functions
#include <ATmega128Demo/ATmega128DemoDeployment/Top/ATmega128DemoDeploymentTopologyAc.hpp>
#include <config/FppConstantsAc.hpp>
#include <Fw/Logger/Logger.hpp>

// Necessary project-specified types
#include <Arduino/config/FprimeArduino.hpp>

// Allows easy reference to objects in FPP/autocoder required namespaces
using namespace ATmega128Demo;

// Base tick of the system, driven by the Timer1 interrupt
static constexpr U32 BASE_TICK_MS = 100;

// The rate group driver divides the 10Hz base tick into sub-signals: 10Hz and 1Hz
Svc::RateGroupDriver::DividerSet rateGroupDivisors{{{1, 0}, {10, 0}}};

// Rate groups may supply a context token to each of the attached children whose purpose is set by the project. The
// reference topology sets each token to zero as these contexts are unused in this project.
Svc::PassiveRateGroup::ContextArray rateGroup10HzContext(0);
Svc::PassiveRateGroup::ContextArray rateGroup1HzContext(0);

/**
 * \brief configure/setup components in project-specific way
 *
 * This is a *helper* function which configures/sets up each component requiring project specific input. This includes
 * allocating resources, passing-in arguments, etc. This function may be inlined into the topology setup function if
 * desired, but is extracted here for clarity.
 */
void configureTopology() {
    // Rate group driver needs a divisor list
    rateGroupDriver.configure(rateGroupDivisors);

    // Rate groups require context arrays.
    rateGroup10Hz.configure(rateGroup10HzContext);
    rateGroup1Hz.configure(rateGroup1HzContext);
}

// Public functions for use in main program are namespaced with deployment namespace ATmega128Demo
namespace ATmega128Demo {
void setupTopology(const TopologyState& state) {
    // Autocoded initialization. Function provided by autocoder.
    initComponents(state);
    // Autocoded id setup. Function provided by autocoder.
    setBaseIds();
    // Autocoded connection wiring. Function provided by autocoder.
    connectComponents();
    // Autocoded configuration. Function provided by autocoder.
    configComponents(state);
    // Project-specific component configuration. Function provided above. May be inlined, if desired.
    configureTopology();
    // Autocoded command registration. Function provided by autocoder.
    regCommands();
    // Autocoded parameter loading. Function provided by autocoder.
    // DISABLED FOR ARDUINO BOARDS. Loading parameters are not supported because there is typically no file system.
    // loadParameters();
    // Autocoded task kick-off (active components). Function provided by autocoder.
    startTasks(state);

    // Ground link on UART1. UART0 is used for programming and console output.
    Serial1.begin(state.uartBaud);
    comDriver.configure(&Serial1);
    
    rateDriver.configure(BASE_TICK_MS);
    rateDriver.start();
}

void teardownTopology(const TopologyState& state) {
    // Autocoded (active component) task clean-up. Functions provided by topology autocoder.
    stopTasks(state);
    freeThreads(state);
}
};  // namespace ATmega128Demo
