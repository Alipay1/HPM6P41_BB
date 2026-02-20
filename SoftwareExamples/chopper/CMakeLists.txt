cmake_minimum_required(VERSION 3.13)

set(CONFIG_SEGGER_RTT 1)
set(CONFIG_HPM_I2C 1)
set(CONFIG_CHERRYUSB 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_CDC_ACM 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(chopper)

sdk_inc(./config)
sdk_app_src(src/adc.c)
sdk_app_src(src/cdc_acm.c)

sdk_compile_definitions(-DCONFIG_NDEBUG_CONSOLE=1)
generate_ide_projects()