# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2021, Nordic Semiconductor ASA

# Configure user cache directory.
#
# The user cache can be used for caching of data that should be persistent
# across builds to speed up CMake configure / build system generation and/or
# compilation.
#
# Only data that can be safely re-generated should be placed in this cache.
#
# Zephyr build system uses this user cache to store Zephyr compiler check
# results which significantly improve toolchain testing performance.
# See https://github.com/zephyrproject-rtos/zephyr/pull/7102 for details.
#
# Outcome:
# The following variables will be defined when this CMake module completes:
#
# - USER_CACHE_DIR: User cache directory in use.
#
# CMake module dependencies:
# - extensions CMake module.

include_guard(GLOBAL)

# Dependencies of this module.
include(extensions)

# Populate USER_CACHE_DIR with a directory that user applications may
# write cache files to.
if(NOT DEFINED USER_CACHE_DIR)
  find_appropriate_cache_directory(USER_CACHE_DIR)
endif()
message(STATUS "Cache files will be written to: ${USER_CACHE_DIR}")
