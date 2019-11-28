# SPDX-License-Identifier: Apache-2.0

# Configures CMake for using MetaWare C compiler

find_program(CMAKE_C_COMPILER ccac)

#if(CONFIG_CPLUSPLUS)
#  set(cplusplus_compiler g++)
#else()
#  if(EXISTS g++)
#    set(cplusplus_compiler g++)
#  else()
#    # When the toolchain doesn't support C++, and we aren't building
#    # with C++ support just set it to something so CMake doesn't
#    # crash, it won't actually be called
#    set(cplusplus_compiler ${CMAKE_C_COMPILER})
#  endif()
#endif()
#find_program(CMAKE_CXX_COMPILER ${cplusplus_compiler}     CACHE INTERNAL " " FORCE)

## The x32 version of libgcc is usually not available (can't trust gcc
## -mx32 --print-libgcc-file-name) so don't fail to build for something
## that is currently not needed. See comments in compiler/gcc/target.cmake
#if (CONFIG_X86)
#  # Convert to list as cmake Modules/*.cmake do it
#  STRING(REGEX REPLACE " +" ";" PRINT_LIBGCC_ARGS ${CMAKE_C_FLAGS})
#  # This libgcc code is partially duplicated in compiler/*/target.cmake
#  execute_process(
#    COMMAND ${CMAKE_C_COMPILER} ${PRINT_LIBGCC_ARGS} --print-libgcc-file-name
#    OUTPUT_VARIABLE LIBGCC_FILE_NAME
#    OUTPUT_STRIP_TRAILING_WHITESPACE
#    )
#  assert_exists(LIBGCC_FILE_NAME)
#endif()

set(NOSTDINC "")

## Note that NOSYSDEF_CFLAG may be an empty string, and
## set_ifndef() does not work with empty string.
#if(NOT DEFINED NOSYSDEF_CFLAG)
#  set(NOSYSDEF_CFLAG -undef)
#endif()

list(APPEND NOSTDINC ${METAWARE_ROOT}/arc/inc)

# For CMake to be able to test if a compiler flag is supported by the
# toolchain we need to give CMake the necessary flags to compile and
# link a dummy C file.
#
# CMake checks compiler flags with check_c_compiler_flag() (Which we
# wrap with target_cc_option() in extentions.cmake)
foreach(isystem_include_dir ${NOSTDINC})
  list(APPEND isystem_include_flags -isystem "\"${isystem_include_dir}\"")
endforeach()

# Load toolchain_cc-family macros
# Significant overlap with freestanding gcc compiler so reuse it
#include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_freestanding.cmake)

macro(toolchain_cc_security_fortify)
  # No op, MetaWare libraries don't understand fortify at all
endmacro()

#include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_security_canaries.cmake)
include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_optimizations.cmake)
include(${ZEPHYR_BASE}/cmake/compiler/ccac/target_cpp.cmake)
include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_asm.cmake)
include(${ZEPHYR_BASE}/cmake/compiler/ccac/target_baremetal.cmake)
include(${ZEPHYR_BASE}/cmake/compiler/ccac/target_warnings.cmake)
include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_imacros.cmake)
include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_base.cmake)
#include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_coverage.cmake)
#include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_sanitizers.cmake)
