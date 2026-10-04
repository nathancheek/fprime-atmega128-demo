set(SOURCE_FILES
  "${CMAKE_CURRENT_LIST_DIR}/ATmegaReset.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/ATmegaReset.cpp"
)
# Reads the rate driver's pending tick count (Arduino::s_pendingTicks) to detect watchdog timeouts
set(MOD_DEPS
  Arduino/Drv/HardwareRateDriver
)

register_fprime_module()
