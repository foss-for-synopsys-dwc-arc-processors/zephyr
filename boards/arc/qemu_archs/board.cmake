set(EMU_PLATFORM qemu)

set(QEMU_CPU_TYPE_${ARCH} arc)

set(QEMU_FLAGS_${ARCH}
  -M simhs
  -m 128M
  -nographic
  -no-reboot
  -monitor none
  -global cpu.firq=false
  -global cpu.has-mpu=true
  -global cpu.mpu-numreg=16
  -global cpu.num-irqlevels=15
  -global cpu.num-irq=25
  -global cpu.ext-irq=20
  -global cpu.freq_hz=1000000
  -global cpu.timer0=true
  -global cpu.timer1=true
  )

set(BOARD_DEBUG_RUNNER qemu)
