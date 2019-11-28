# SPDX-License-Identifier: Apache-2.0

# /global/apps/mwdt_2019.09/MetaWare/arc/bin/ccac
set_ifndef(METAWARE_ROOT "$ENV{METAWARE_ROOT}")
set(       METAWARE_ROOT ${METAWARE_ROOT} CACHE PATH "")
assert(    METAWARE_ROOT "METAWARE_ROOT is not set")

set(TOOLCHAIN_HOME ${METAWARE_ROOT})

set(COMPILER ccac)
set(LINKER ld)
set(BINTOOLS gnu)

#set(CROSS_COMPILE_TARGET xtensa-esp32-elf)
set(SYSROOT_TARGET       "arc/lib/av2hs/le")

set(CROSS_COMPILE ${TOOLCHAIN_HOME}/arc/bin/)
set(SYSROOT_DIR   ${TOOLCHAIN_HOME}/${SYSROOT_TARGET})

set(TOOLCHAIN_HAS_NEWLIB ON CACHE BOOL "True if toolchain supports newlib")

