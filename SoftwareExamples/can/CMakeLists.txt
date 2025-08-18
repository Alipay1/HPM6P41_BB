cmake_minimum_required(VERSION 3.13)

set(CONFIG_SEGGER_RTT 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(can_test)

sdk_app_src(src/can.c)
sdk_app_inc(src)

sdk_compile_definitions(-DCONFIG_NDEBUG_CONSOLE=1)
generate_ide_projects()
