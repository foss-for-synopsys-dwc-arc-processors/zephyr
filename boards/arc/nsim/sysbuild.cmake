# Copyright (c) 2016, 2019 Synopsys, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SB_NSIM_SEM_SECURESHIELD)
  set(SECURESHIELD_BINARY_DIR ${CMAKE_BINARY_DIR}/secureshield)
#  add_custom_target(secureshield_cmake
#    COMMAND ${CMAKE_COMMAND} -DBOARD=${CONFIG_SECURESHIELD_BOARD} -B${SECURESHIELD_BINARY_DIR} -S${ZEPHYR_BASE}/${CONFIG_SECURESHIELD_IMAGE_PATH} -G${CMAKE_GENERATOR}
#    COMMAND_EXPAND_LISTS
#  )
  ExternalZephyrProject_Add(
    APPLICATION secureshield
    BOARD ${CONFIG_SECURESHIELD_BOARD}
    SOURCE_DIR ${ZEPHYR_BASE}/${CONFIG_SECURESHIELD_IMAGE_PATH}
    BINARY_DIR ${SECURESHIELD_BINARY_DIR}
    BUILD_ALWAYS True
#    DEPENDS secureshield_cmake
  )
  list(APPEND IMAGES "secureshield")
  set(empty_cpu0_BINARY_DIR "${CMAKE_BINARY_DIR}/secureshield")
endif()

