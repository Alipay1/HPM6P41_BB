# Copyright (c) 2021 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

set(CONFIG_SEGGER_RTT 1)
set(CONFIG_HPM_I2C 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(hello_world)

sdk_app_src(src/hello_world.c)
generate_ide_projects()
