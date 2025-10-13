# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

# Use custom QEMU with AIA support
set(QEMU_BIN_PATH "$ENV{HOME}/qemu-aia/build")

set(QEMU_binary_suffix riscv32)
set(QEMU_CPU_TYPE_${ARCH} riscv32)

set(QEMU_FLAGS_${ARCH}
  -nographic
  -machine virt,aia=aplic-imsic
  -bios none
  -m 256M
)

# Add SMP flags for SMP variant
if(CONFIG_SMP)
  list(APPEND QEMU_FLAGS_${ARCH} -smp ${CONFIG_MP_MAX_NUM_CPUS})
endif()

board_set_debugger_ifnset(qemu)
