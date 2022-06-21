# Copyright (c) 2022 Synopsys
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

zephyr_compile_options("-Wno-ignored-qualifiers")

zephyr_compile_definitions(
  V2DSP_XY
)

target_sources(app PRIVATE src/main.c)
