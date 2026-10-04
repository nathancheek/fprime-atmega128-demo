set(SOURCE_FILES
  "${CMAKE_CURRENT_LIST_DIR}/PassiveTlmPacketizer.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/PassiveTlmPacketizer.cpp"
)
# TlmPacketizerTypes.hpp includes TlmPacketizerConfig/TlmPacketizerCfg.hpp
set(MOD_DEPS
  Svc_TlmPacketizer_config_TlmPacketizerConfig
)

register_fprime_module()
