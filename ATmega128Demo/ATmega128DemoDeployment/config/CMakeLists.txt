add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/TlmPacketizerConfig")

register_fprime_config(
    CONFIGURATION_OVERRIDES
        "${CMAKE_CURRENT_LIST_DIR}/AcConstants.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/ComCfg.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/CommandDispatcherImplCfg.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/FpConfig.h"
        "${CMAKE_CURRENT_LIST_DIR}/FPrimeNumericalConfig.h"
        "${CMAKE_CURRENT_LIST_DIR}/FpConstants.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/PassiveRateGroupCfg.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/PlatformCfg.fpp"
    INTERFACE
)
