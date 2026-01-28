cmake_minimum_required(VERSION 3.13)

set(CONFIG_SEGGER_RTT 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(adc_pwm)

sdk_app_src(src/main.c)

sdk_compile_definitions(-DCONFIG_NDEBUG_CONSOLE=1)
generate_ide_projects()