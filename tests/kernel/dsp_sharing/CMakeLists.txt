# Copyright (c) 2022 Synopsys
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dsp_sharing)

target_sources(app PRIVATE
    src/main.c
    src/load_store.c
    )

if(CONFIG_ISA_ARCV2)
zephyr_include_directories(
      ${ARCMWDT_TOOLCHAIN_PATH}/MetaWare/arc/lib/src/fx/include/
    )
target_sources(app PRIVATE src/calculation_arc.c)
endif()
