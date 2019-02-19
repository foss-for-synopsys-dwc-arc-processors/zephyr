set(EMU_PLATFORM qemu)

set(QEMU_CPU_TYPE_${ARCH} arc)

set(QEMU_FLAGS_${ARCH}
  -M arc-sim
  -m 128M
  -nographic
  -no-reboot
  -monitor none
  )

set(BOARD_DEBUG_RUNNER qemu)
