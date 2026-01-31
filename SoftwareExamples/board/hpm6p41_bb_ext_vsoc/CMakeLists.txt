add_subdirectory(./drivers)

sdk_inc(.)
sdk_src(pinmux.c)
sdk_src(board.c)

sdk_compile_definitions(-DCONFIG_NDEBUG_CONSOLE=1)
sdk_compile_definitions(-DSDK_VERSION_STRING=${SDK_VERSION_STRING})
sdk_ses_opt_lib_io_type(RTT)
sdk_ses_opt_debug_connection(J-Link)
sdk_ses_opt_debug_jlink_speed(20000)

sdk_ses_compile_options(-mabi=ilp32d)
sdk_ses_compile_options(-march=rv32gc)