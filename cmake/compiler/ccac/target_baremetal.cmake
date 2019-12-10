# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_cc_nostdinc)

  if (NOT CONFIG_NEWLIB_LIBC AND
    NOT COMPILER STREQUAL "xcc" AND
    NOT CONFIG_NATIVE_APPLICATION)
    zephyr_compile_options( -Hno_default_include -Hnoarcexlib -Hnocopyr -O0)
    zephyr_system_include_directories(${NOSTDINC})
  endif()

endmacro()

# CCAC doesn't support "-ffreestanding"
macro(toolchain_cc_freestanding)
endmacro()
