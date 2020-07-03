# SPDX-License-Identifier: Apache-2.0

set_ifndef(MWDT_TOOLCHAIN_PATH "$ENV{MWDT_TOOLCHAIN_PATH}")
set(MWDT_TOOLCHAIN_PATH ${MWDT_TOOLCHAIN_PATH} CACHE PATH "mwdt tools install directory")
assert(MWDT_TOOLCHAIN_PATH "MWDT_TOOLCHAIN_PATH is not set")

if(NOT EXISTS ${MWDT_TOOLCHAIN_PATH})
  message(FATAL_ERROR "Nothing found at MWDT_TOOLCHAIN_PATH: '${MWDT_TOOLCHAIN_PATH}'")
endif()

set(TOOLCHAIN_HOME ${MWDT_TOOLCHAIN_PATH}/MetaWare)

set(COMPILER arcmwdt)
set(LINKER arcmwdt)
set(BINTOOLS arcmwdt)

set(SYSROOT_TARGET arc)

set(CROSS_COMPILE ${TOOLCHAIN_HOME}/arc/bin/)
set(SYSROOT_DIR ${TOOLCHAIN_HOME}/${SYSROOT_TARGET})

set(TOOLCHAIN_HAS_NEWLIB OFF CACHE BOOL "True if toolchain supports newlib")
