# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2021, Nordic Semiconductor ASA

# Convert Zephyr roots to absolute paths.
#
# This CMake module will convert all relative paths in existing ROOT lists to
# absolute path relative from APPLICATION_SOURCE_DIR.
#
# Outcome:
# The following variables will be defined when this CMake module completes:
#
# - ARCH_ROOT
# - BOARD_ROOT
# - SOC_ROOT
# - MODULE_EXT_ROOT
# with all path converted to absolute path, relative from APPLICATION_SOURCE_DIR
#
# CMake module dependencies:
# - extensions CMake module.

include_guard(GLOBAL)

# Dependencies of this module.
include(extensions)

# Convert paths to absolute, relative from APPLICATION_SOURCE_DIR
zephyr_file(APPLICATION_ROOT MODULE_EXT_ROOT)

# Convert paths to absolute, relative from APPLICATION_SOURCE_DIR
zephyr_file(APPLICATION_ROOT BOARD_ROOT)

# Convert paths to absolute, relative from APPLICATION_SOURCE_DIR
zephyr_file(APPLICATION_ROOT SOC_ROOT)

# Convert paths to absolute, relative from APPLICATION_SOURCE_DIR
zephyr_file(APPLICATION_ROOT ARCH_ROOT)
