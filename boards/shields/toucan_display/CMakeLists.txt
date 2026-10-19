if(CONFIG_ZMK_TOUCAN_SCREEN)
  zephyr_library()
  zephyr_library_sources(screen.c)
endif()
