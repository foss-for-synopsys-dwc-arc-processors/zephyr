set(EMU_PLATFORM qemu)

set(QEMU_CPU_TYPE_${ARCH} arc)

set(QEMU_FLAGS_${ARCH}
  -M arc-sim
  -m 128M
  -nographic
  -no-reboot
  -monitor none
  -global cpu.firq=false
  -global cpu.mpu-numreg=16
  )

set(BOARD_DEBUG_RUNNER qemu)
