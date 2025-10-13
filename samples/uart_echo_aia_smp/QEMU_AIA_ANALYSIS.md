# QEMU RISC-V AIA Implementation Analysis

## Problem Summary

**Issue**: RISC-V AIA MMSI mode not working in SMP with QEMU
**Root Cause**: QEMU IMSIC implementation masks EIDELIVERY write to only bit 0, ignoring mode bits [30:29]

## Current Status

✅ **Working**:
- Both harts boot and run independently (SMP works)
- APLIC MSI geometry configured for SMP (LHXS=12, LHXW=1)
- APLIC GENMSI sends MSI writes to correct IMSIC addresses
- IMSIC receives MSI writes and sets pending bits
- MEXT (IRQ 11) enabled on both harts
- EIE (interrupt enable) registers configured correctly

❌ **Not Working**:
- EIDELIVERY mode bits [30:29] being masked out
- IMSIC not delivering interrupts via mtopei/MEXT
- ISRs never execute

## QEMU IMSIC Bug

### Location
File: `/home/afonsoo/qemu-aia/hw/intc/riscv_imsic.c`
Function: `riscv_imsic_eidelivery_rmw()` (lines 90-106)

### The Bug
```c
static int riscv_imsic_eidelivery_rmw(RISCVIMSICState *imsic, uint32_t page,
                                      target_ulong *val,
                                      target_ulong new_val,
                                      target_ulong wr_mask)
{
    target_ulong old_val = imsic->eidelivery[page];

    if (val) {
        *val = old_val;
    }

    wr_mask &= 0x1;  // ❌ BUG: Only allows bit 0 to be written!
    imsic->eidelivery[page] = (old_val & ~wr_mask) | (new_val & wr_mask);

    riscv_imsic_update(imsic, page);
    return 0;
}
```

**Problem**: Line 101 masks `wr_mask` to `0x1`, which means only bit 0 (EIDELIVERY_ENABLE) can be written. Bits [30:29] (delivery mode) are completely ignored.

### Expected Behavior (per AIA spec v1.0)

EIDELIVERY register format:
```
Bits [31:30] - Reserved (WARL 0)
Bits [30:29] - Delivery Mode:
               00 = Reserved
               01 = Direct MSI delivery (DMSI)
               10 = MSI-via-memory (MMSI) ← What we need!
               11 = Both modes
Bits [28:1]  - Reserved (WARL 0)
Bit  [0]     - Enable interrupt delivery
```

For MMSI mode with APLIC, we need: `EIDELIVERY = 0x40000001`
- Bit 0 = 1 (enable delivery)
- Bits [30:29] = 10 (MMSI mode)

### Current vs Expected

| Write Attempt | Current Readback | Expected Readback |
|---------------|------------------|-------------------|
| 0x40000001    | 0x00000001       | 0x40000001        |

## IMSIC Operation Flow

### 1. MSI Write Path (Working)
```
APLIC GENMSI write
  ↓
Calculate MSI address using geometry (LHXS, LHXW, HHXS, HHXW)
  ↓
Address = (base_ppn | hart_bits | guest_bits) << 12
  ↓
Write EIID to IMSIC MMIO (riscv_imsic_write, line 285)
  ↓
Set pending bit: eistate[page][eiid] |= PENDING (line 319)
  ↓
Call riscv_imsic_update() (line 323)
```

### 2. Interrupt Delivery Path (Broken)
```
riscv_imsic_update(imsic, page) - line 67
  ↓
Check: if (imsic->eidelivery[page] && riscv_imsic_topei(imsic, page))
  ↓
  ❌ FAILS HERE: eidelivery[page] = 0x00000001
     Mode bits [30:29] = 00 (Reserved mode)
     IMSIC doesn't know how to deliver!
  ↓
If check passed, would:
  - qemu_irq_raise(imsic->external_irqs[page])  ← Assert MEXT
  - Update mtopei with (eiid << 16) | eiid
```

### 3. riscv_imsic_topei() - line 49
```c
static uint32_t riscv_imsic_topei(RISCVIMSICState *imsic, uint32_t page)
{
    uint32_t i, max_irq, base;

    base = page * imsic->num_irqs;
    max_irq = (imsic->eithreshold[page] &&
               (imsic->eithreshold[page] <= imsic->num_irqs)) ?
               imsic->eithreshold[page] : imsic->num_irqs;

    // Scan for highest priority enabled+pending interrupt
    for (i = 1; i < max_irq; i++) {
        if ((qatomic_read(&imsic->eistate[base + i]) & IMSIC_EISTATE_ENPEND) ==
                IMSIC_EISTATE_ENPEND) {
            return (i << IMSIC_TOPEI_IID_SHIFT) | i;
        }
    }

    return 0;
}
```

This function works correctly - it finds pending+enabled interrupts. The problem is in the delivery mode check.

## APLIC MSI Send (Working Correctly)

### riscv_aplic_msi_send() - line 386

```c
static void riscv_aplic_msi_send(RISCVAPLICState *aplic,
                                 uint32_t hart_idx, uint32_t guest_idx,
                                 uint32_t eiid)
{
    // Extract geometry parameters
    lhxs = (msicfgaddrH >> APLIC_xMSICFGADDRH_LHXS_SHIFT) &
            APLIC_xMSICFGADDRH_LHXS_MASK;  // 12 for our config
    lhxw = (msicfgaddrH >> APLIC_xMSICFGADDRH_LHXW_SHIFT) &
            APLIC_xMSICFGADDRH_LHXW_MASK;  // 1 for our config
    hhxs = (msicfgaddrH >> APLIC_xMSICFGADDRH_HHXS_SHIFT) &
            APLIC_xMSICFGADDRH_HHXS_MASK;  // 24 for our config
    hhxw = (msicfgaddrH >> APLIC_xMSICFGADDRH_HHXW_SHIFT) &
            APLIC_xMSICFGADDRH_HHXW_MASK;  // 0 for our config

    // Calculate target IMSIC address
    addr = msicfgaddr;  // Base PPN
    addr |= ((uint64_t)(msicfgaddrH & APLIC_xMSICFGADDRH_BAPPN_MASK)) << 32;
    addr |= ((uint64_t)(group_idx & APLIC_xMSICFGADDR_PPN_HHX_MASK(hhxw))) <<
             APLIC_xMSICFGADDR_PPN_HHX_SHIFT(hhxs);
    addr |= ((uint64_t)(hart_idx & APLIC_xMSICFGADDR_PPN_LHX_MASK(lhxw))) <<
             APLIC_xMSICFGADDR_PPN_LHX_SHIFT(lhxs);
    addr |= (uint64_t)(guest_idx & APLIC_xMSICFGADDR_PPN_HART(lhxs));
    addr <<= APLIC_xMSICFGADDR_PPN_SHIFT;

    // Write EIID to calculated address
    address_space_stl_le(&address_space_memory, addr,
                         eiid, MEMTXATTRS_UNSPECIFIED, &result);
}
```

For our SMP config with 2 harts:
- Base: 0x24000000
- LHXS=12: Hart index in bits [23:12] of physical address
- LHXW=1: 1 bit encodes hart (0 or 1)
- Hart 0: 0x24000000 + (0 << 12) = 0x24000000
- Hart 1: 0x24000000 + (1 << 12) = 0x24001000

This is working correctly!

## Fix Applied

### Fix 1: EIDELIVERY Mode Bits ✅ COMPLETED

Changed line 101 in `/home/afonsoo/qemu-aia/hw/intc/riscv_imsic.c`:

```c
// Before:
wr_mask &= 0x1;

// After (with comment explaining per AIA spec):
/* Per AIA spec v1.0, EIDELIVERY writable bits:
 * - Bit 0: Enable interrupt delivery
 * - Bits [30:29]: Delivery mode (00=Reserved, 01=DMSI, 10=MMSI, 11=Both)
 * Allow both enable bit and mode bits to be written
 */
wr_mask &= 0x60000001;
```

**Result**: EIDELIVERY now correctly reads back as 0x40000001 (MMSI mode with enable bit set).

**Status**: This fix is working! The mode bits are now preserved in QEMU.

### Option 2: Workaround in Zephyr (Not Recommended)

Could try forcing EIDELIVERY to just 0x1 and hope QEMU defaults to some working mode, but this won't follow the spec.

## Testing After Fix

Once QEMU is fixed, verify:

1. **EIDELIVERY readback**:
   ```
   Write: 0x40000001
   Read:  0x40000001  ← Should match!
   ```

2. **GENMSI triggers ISR**:
   ```
   riscv_aplic_inject_genmsi(0, 32)
   → ISR executes on Hart 0
   → stats[0].isr_count increments
   ```

3. **SMP interrupt routing**:
   ```
   Route UART to Hart 0, type character
   → Hart 0 handles interrupt
   Route UART to Hart 1, type character
   → Hart 1 handles interrupt
   ```

## Reference: AIA Spec Sections

- **Section 3.7**: IMSIC EIDELIVERY register format
- **Section 4.9**: APLIC MSI address calculation
- **Section 3.6**: IMSIC interrupt file memory layout

## Files to Check

- QEMU IMSIC: `/home/afonsoo/qemu-aia/hw/intc/riscv_imsic.c`
- QEMU APLIC: `/home/afonsoo/qemu-aia/hw/intc/riscv_aplic.c`
- Zephyr IMSIC driver: `/home/afonsoo/zephyr/drivers/interrupt_controller/intc_riscv_imsic.c`
- Zephyr APLIC driver: `/home/afonsoo/zephyr/drivers/interrupt_controller/intc_riscv_aplic_msi.c`
- Test application: `/home/afonsoo/zephyr/samples/uart_echo_aia_smp/src/main.c`
