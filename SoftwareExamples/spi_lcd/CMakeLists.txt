cmake_minimum_required(VERSION 3.13)

# set(SES_TOOLCHAIN_VARIANT "Andes")
# set(RV_ARCH "rv32imafc")
# set(RV_ABI "ilp32f")

set(CONFIG_SEGGER_RTT 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(spi_lcd)

sdk_app_src(src/spi_lcd.c)
sdk_app_inc(src)

sdk_compile_definitions(-DCONFIG_NDEBUG_CONSOLE=1)
generate_ide_projects()
