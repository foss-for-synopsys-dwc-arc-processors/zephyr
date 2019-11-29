# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_cc_warning_base)

  zephyr_compile_options(
    -Weverything
    -Wformat
    -Wno-extra-semi-stmt
    -Wno-reserved-id-macro
    -Wno-sign-conversion
    -Wno-documentation
    -Wno-c11-extensions
    -Wno-implicit-cast-widening
    -Wno-gnu-zero-variadic-macro-arguments
    -Wno-padded
    -Wno-gnu-statement-expression
    -Wno-shorten-64-to-32
    -Wno-unused-macros
    -Wno-strict-prototypes
    -Wno-unused-parameter
    -Wno-c++-compat
    -Wno-documentation-unknown-command
    -Wno-gnu-empty-struct
    -Wno-typedef-redefinition
    -Wno-cast-align
    -Wno-shadow
    -Wno-missing-variable-declarations
    -Wno-implicit-int-conversion
    -Wno-extra-semi
    -Wno-bad-function-cast
    -Wno-packed
    -Wno-undef
    -Wno-gnu-empty-initializer
    -Wno-static-in-inline
  )

if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "9.1.0")
  zephyr_compile_options(
    # FIXME: Remove once #16587 is fixed
    -Wno-address-of-packed-member
  )
endif()

  zephyr_cc_option(-Wno-pointer-sign)

  # Prohibit void pointer arithmetic. Illegal in C99
  zephyr_cc_option(-Wpointer-arith)

endmacro()

macro(toolchain_cc_warning_dw_1)

  zephyr_compile_options(
    -Wextra
    -Wunused
    -Wno-unused-parameter
    -Wmissing-declarations
    -Wmissing-format-attribute
    -Wold-style-definition
    )
  zephyr_cc_option(
    -Wmissing-prototypes
    -Wmissing-include-dirs
    -Wunused-but-set-variable
    -Wno-missing-field-initializers
    )

endmacro()

macro(toolchain_cc_warning_dw_2)

  zephyr_compile_options(
    -Waggregate-return
    -Wcast-align
    -Wdisabled-optimization
    -Wnested-externs
    -Wshadow
    )
  zephyr_cc_option(
    -Wlogical-op
    -Wmissing-field-initializers
    )

endmacro()

macro(toolchain_cc_warning_dw_3)

  zephyr_compile_options(
    -Wbad-function-cast
    -Wcast-qual
    -Wconversion
    -Wpacked
    -Wpadded
    -Wpointer-arith
    -Wredundant-decls
    -Wswitch-default
    )
  zephyr_cc_option(
    -Wpacked-bitfield-compat
    -Wvla
    )

endmacro()

macro(toolchain_cc_warning_extended)

  zephyr_cc_option(
    -Wno-unused-but-set-variable
    )

endmacro()

macro(toolchain_cc_warning_error_implicit_int)

  # Force an error when things like SYS_INIT(foo, ...) occur with a missing header
  zephyr_cc_option(-Werror=implicit-int)

endmacro()

#
# The following macros leaves it up to the root CMakeLists.txt to choose
#  the variables in which to put the requested flags, and whether or not
#  to call the macros
#

macro(toolchain_cc_warning_error_misra_sane dest_var_name)
  set_ifndef(${dest_var_name} "-Werror=vla")
endmacro()

macro(toolchain_cc_cpp_warning_error_misra_sane dest_var_name)
  set_ifndef(${dest_var_name} "-Werror=vla")
endmacro()
