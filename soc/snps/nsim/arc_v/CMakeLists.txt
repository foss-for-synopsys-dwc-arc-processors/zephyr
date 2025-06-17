# SPDX-License-Identifier: Apache-2.0

# Set default linker script for all RISC-V nsim_arc_v boards
set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")

# Add subdirectories based on CONFIG
if(CONFIG_SOC_SERIES_NSIM_ARC_V_RMX)
  add_subdirectory(rmx)
endif()

if(CONFIG_SOC_SERIES_NSIM_ARC_V_RHX)
  add_subdirectory(rhx)
endif()