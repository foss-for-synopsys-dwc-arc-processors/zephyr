# SPDX-License-Identifier: Apache-2.0

# /global/apps/mwdt_2019.09/MetaWare/arc/bin/ccac
set_ifndef(METAWARE_TOOLCHAIN_PATH "$ENV{METAWARE_TOOLCHAIN_PATH}")
set(       METAWARE_TOOLCHAIN_PATH ${METAWARE_TOOLCHAIN_PATH} CACHE PATH "")
assert(    METAWARE_TOOLCHAIN_PATH "METAWARE_TOOLCHAIN_PATH is not set")

set(TOOLCHAIN_HOME ${METAWARE_TOOLCHAIN_PATH})

set(COMPILER ccac)
set(LINKER ldac)
set(BINTOOLS gnu)

#set(CROSS_COMPILE_TARGET xtensa-esp32-elf)
set(SYSROOT_TARGET       "arc/lib/av2hs/le")

set(CROSS_COMPILE ${TOOLCHAIN_HOME}/arc/bin/)
set(SYSROOT_DIR   ${TOOLCHAIN_HOME}/${SYSROOT_TARGET})

set(TOOLCHAIN_HAS_NEWLIB ON CACHE BOOL "True if toolchain supports newlib")

