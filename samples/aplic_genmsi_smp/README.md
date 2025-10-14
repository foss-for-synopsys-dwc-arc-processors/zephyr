# Copyright (c) 2025 Synopsys, Inc.
# SPDX-License-Identifier: Apache-2.0

# APLIC GENMSI SMP Test

This sample validates APLIC GENMSI (software-triggered MSI injection) functionality in an SMP (multi-processor) environment with multiple IMSIC controllers.

## Overview

The RISC-V Advanced Interrupt Architecture (AIA) includes a GENMSI register in the APLIC that allows software to inject MSIs to any hart's IMSIC. This is useful for:
- Inter-processor interrupts (IPIs)
- Software-triggered interrupts for testing
- Cross-CPU signaling

This test validates that:
1. GENMSI can target specific harts correctly
2. Each hart's IMSIC receives only the MSIs targeted to it
3. Multiple MSI injections work correctly
4. The APLIC MSI address geometry is configured correctly for SMP

## Hardware Requirements

- RISC-V platform with AIA support (APLIC-MSI + IMSIC)
- At least 2 CPU cores
- Per-hart IMSIC controllers at consecutive 4KB-aligned addresses

## Supported Platforms

- `qemu_riscv32_aia/qemu_virt_riscv32_aia/smp` (2 CPUs) - **NOTE: Currently blocked by IMSIC SMP registration issue**
- `qemu_riscv32_aia` (single CPU) - Works for testing GENMSI targeting logic

## Building and Running

### Known Limitation

Currently, the IMSIC driver has a limitation where multiple IMSIC instances (in SMP) both try to register IRQ 11 (MEXT), causing a build failure:
```
gen_isr_tables.py: error: multiple registrations at table_index 11 for irq 11 (0xb)
```

This needs to be fixed in the IMSIC driver to properly handle per-CPU MEXT handlers in SMP configurations.

### QEMU (Single CPU - Current Workaround)

```bash
west build -b qemu_riscv32_aia samples/aplic_genmsi_smp
west build -t run
```

### QEMU (2 CPUs - After IMSIC SMP Fix)

```bash
west build -b qemu_riscv32_aia/qemu_virt_riscv32_aia/smp samples/aplic_genmsi_smp
west build -t run
```

The QEMU command uses:
```
qemu-system-riscv32 -M virt,aia=aplic-imsic -smp 2
```

## Test Steps

The test performs the following validation steps:

1. **APLIC Configuration Check**
   - Reads DOMAINCFG, MSIADDRCFG, MSIADDRCFGH
   - Verifies MSI address points to IMSIC0 base
   - Decodes geometry fields (LHXS, LHXW, HHXS, HHXW)

2. **ISR Setup**
   - Registers interrupt handlers on both CPUs
   - CPU 0: EIID 64
   - CPU 1: EIID 65

3. **GENMSI to CPU 0**
   - Writes GENMSI with hart=0, EIID=64
   - Verifies only CPU 0 ISR fires

4. **GENMSI to CPU 1**
   - Writes GENMSI with hart=1, EIID=65
   - Verifies only CPU 1 ISR fires

5. **Multiple Injections**
   - Sends 5 MSIs to each CPU
   - Verifies each CPU receives exactly 5 interrupts

6. **Broadcast Pattern**
   - Sends same EIID to both CPUs sequentially
   - Verifies both can receive the same interrupt number

## Expected Output

```
╔═══════════════════════════════════════════════╗
║       APLIC GENMSI SMP Test (2 CPUs)         ║
╚═══════════════════════════════════════════════╝

Hardware Configuration:
  APLIC Base:   0x0c000000
  IMSIC0 Base:  0x24000000 (CPU 0)
  IMSIC1 Base:  0x24001000 (CPU 1)
  Num CPUs:     2

STEP 1: Reading APLIC Configuration
======================================
  DOMAINCFG:    0x00000104
    - IE (bit 8):  Enabled
    - DM (bit 2):  MSI mode
  MSIADDRCFG:   0x00024000 (PPN for IMSIC base)
  MSIADDRCFGH:  0x00000000 (geometry fields)
    Geometry: LHXS=0, LHXW=1, HHXS=0, HHXW=0
  ✓ MSIADDRCFG matches IMSIC0 address

STEP 2: Setting up Test ISRs
======================================
  CPU 0: Registered ISR for EIID 64
  CPU 1: Registered ISR for EIID 65

STEP 3: Testing GENMSI to CPU 0 (hart 0, EIID 64)
======================================
  Writing 0x00000840 to GENMSI (Hart=0, MSI_DEL=1, EIID=64)
  [CPU 0 ISR] Fired! Count=1, EIID=64
  Results:
    CPU 0 ISR count: 1 ✓
    CPU 1 ISR count: 0 ✓ (expected 0)

STEP 4: Testing GENMSI to CPU 1 (hart 1, EIID 65)
======================================
  Writing 0x00040841 to GENMSI (Hart=1, MSI_DEL=1, EIID=65)
  [CPU 1 ISR] Fired! Count=1, EIID=65
  Results:
    CPU 0 ISR count: 0 ✓ (expected 0)
    CPU 1 ISR count: 1 ✓

...
```

## GENMSI Register Format

The GENMSI register format (per RISC-V AIA spec):

```
Bits [31:18]: Hart Index (14 bits)
Bits [17:13]: Context/Guest (5 bits, for DMSI)
Bit  [12]:    Busy (read-only status)
Bit  [11]:    MSI_DEL (0=DMSI, 1=MMSI)
Bits [10:0]:  EIID (11 bits)
```

For MMSI delivery (memory-mapped MSI), set bit 11 to 1.

## MSI Address Calculation

The APLIC calculates the target MSI address using geometry fields:

```
MSI_ADDR = (base_ppn | hart_bits | guest_bits) << 12
```

For 2 CPUs at 4KB spacing:
- LHXW = 1 (log2(2) = 1 bit for hart index)
- LHXS = 0 (hart bits start at bit 0 of PPN = bit 12 of physical address)
- Hart 0: 0x24000000
- Hart 1: 0x24001000 (base + 0x1000)

## Troubleshooting

### CPU 1 ISR never fires
- Check that IMSIC1 is enabled in device tree
- Verify MSIADDRCFGH geometry fields are correct
- Ensure both IMSIC controllers are initialized

### All interrupts go to CPU 0
- Check LHXS/LHXW in MSIADDRCFGH
- Verify IMSIC spacing (should be 4KB = 0x1000)

### No interrupts fire at all
- Check APLIC DOMAINCFG IE bit (should be 1)
- Verify MSIADDRCFG points to correct IMSIC base
- Ensure IMSIC EIDELIVERY is enabled

## References

- RISC-V Advanced Interrupt Architecture Specification v1.0
- ARC-V APLIC Technical Reference Manual
- Zephyr RTOS SMP Documentation
