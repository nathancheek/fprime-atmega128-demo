set(SOURCE_FILES
  "${CMAKE_CURRENT_LIST_DIR}/ComSpacePacket.fpp"
  "${CMAKE_CURRENT_LIST_DIR}/ComSpacePacket.cpp"
)
set(MOD_DEPS
  ATmega128Demo/Utils/Crc16
)

register_fprime_module()
