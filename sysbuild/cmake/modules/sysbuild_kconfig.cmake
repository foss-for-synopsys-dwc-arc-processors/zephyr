# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

set(EXTRA_KCONFIG_TARGET_COMMAND_FOR_sysbuild_menuconfig
  ${ZEPHYR_BASE}/scripts/kconfig/menuconfig.py
  )

set(EXTRA_KCONFIG_TARGET_COMMAND_FOR_sysbuild_guiconfig
  ${ZEPHYR_BASE}/scripts/kconfig/guiconfig.py
  )

set(KCONFIG_TARGETS sysbuild_menuconfig sysbuild_guiconfig)
list(TRANSFORM EXTRA_KCONFIG_TARGETS PREPEND "sysbuild_")

set_ifndef(SB_CONF_FILE ${CMAKE_CURRENT_BINARY_DIR}/empty.conf)

# Empty files to make kconfig.py happy.
file(TOUCH ${CMAKE_CURRENT_BINARY_DIR}/empty.conf)
set(APPLICATION_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(KCONFIG_BINARY_DIR     ${CMAKE_CURRENT_BINARY_DIR})
set(AUTOCONF_H             ${CMAKE_CURRENT_BINARY_DIR}/autoconf.h)
set(CONF_FILE              ${SB_CONF_FILE})
set(BOARD_DEFCONFIG        "${CMAKE_CURRENT_BINARY_DIR}/empty.conf")
list(APPEND ZEPHYR_KCONFIG_MODULES_DIR BOARD=${BOARD})
set(KCONFIG_NAMESPACE CONFIG_SB)

if(EXISTS   ${APP_DIR}/Kconfig.sysbuild)
  set(KCONFIG_ROOT ${APP_DIR}/Kconfig.sysbuild)
endif()
file(TOUCH ${KCONFIG_BINARY_DIR}/Kconfig.modules)
include(${ZEPHYR_BASE}/cmake/modules/kconfig.cmake)
set(CONF_FILE)
