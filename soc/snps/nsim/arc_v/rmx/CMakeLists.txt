# SPDX-License-Identifier: Apache-2.0

if(COMPILER STREQUAL arcmwdt)
# CCAC:
  zephyr_compile_options_ifdef(CONFIG_SOC_SERIES_RMX
					-av5rmx -Zicsr -Zifencei -Zihintpause -Zba -Zbb
					-Zbs -Zca -Zcb -Zcmp -Zcmt -Za -Zm -Zicbom)

# Get version string from CCAC compiler
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} --version
    OUTPUT_VARIABLE MWDT_VERSION_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

# Extract version in the format YYYY.MM
  string(REGEX MATCH "[0-9]+\\.[0-9]+" MWDT_VERSION "${MWDT_VERSION_OUTPUT}")

# Version logic for linker
if("${MWDT_VERSION}" VERSION_LESS_EQUAL 2024.06)
    zephyr_ld_options(-Hlib=rmx100)
else()
    zephyr_ld_options(-Hlib=rmx100_mini)
endif()

endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
