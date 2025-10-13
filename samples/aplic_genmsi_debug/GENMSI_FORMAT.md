# Copyright (c) 2025 Synopsys, Inc.
# SPDX-License-Identifier: Apache-2.0

# APLIC GENMSI Register Format

## Register Layout

The GENMSI register at offset 0x3000 has the following field layout:

```
Bits [31:18] - Hart Index (14 bits, 0-16383)
Bits [17:12] - Guest Index (6 bits, 0-63) [DMSI only]
Bit  [11]    - MSI_DEL (nSIM-specific) or Reserved
Bits [10:0]  - EIID (11 bits, 0-2047)
```

## Platform Differences

### QEMU Implementation

QEMU's APLIC extracts fields as follows (from `hw/intc/riscv_aplic.c`):

```c
aplic->genmsi = value & ~(APLIC_TARGET_GUEST_IDX_MASK << APLIC_TARGET_GUEST_IDX_SHIFT);
riscv_aplic_msi_send(aplic,
                     value >> APLIC_TARGET_HART_IDX_SHIFT,  // Bits [31:18]
                     0,                                       // Guest always 0
                     value & APLIC_TARGET_EIID_MASK);       // Bits [10:0]
```

Where:
- `APLIC_TARGET_HART_IDX_SHIFT = 18`
- `APLIC_TARGET_HART_IDX_MASK = 0x3FFF` (14 bits)
- `APLIC_TARGET_GUEST_IDX_SHIFT = 12`
- `APLIC_TARGET_GUEST_IDX_MASK = 0x3F` (6 bits)
- `APLIC_TARGET_EIID_MASK = 0x7FF` (11 bits = bits [10:0])

**Key point**: QEMU masks out the guest index field (bits [17:12]) but **bit 11 is part of the EIID mask**. QEMU doesn't have a separate MSI_DEL bit - it's always in MSI mode when `aplic->msimode` is true.

### nSIM ARC-V Implementation

nSIM's APLIC has an additional **MSI_DEL bit at bit 11** that controls delivery mode:
- Bit 11 = 0: DMSI delivery (direct CSR writes)
- Bit 11 = 1: MMSI delivery (memory-mapped MSI writes)

This is documented in the ARC-V APLIC TRM Table 6-37.

## Recommended GENMSI Value Format

For maximum compatibility across both platforms:

```c
uint32_t genmsi_val = (hart_id << 18) | (1U << 11) | (eiid & 0x7FF);
```

This works because:
1. **QEMU**: Extracts EIID as `value & 0x7FF`, which masks out everything above bit 10, so bit 11 ends up being ignored in the final EIID calculation
2. **nSIM**: Interprets bit 11 as MSI_DEL=1 for MMSI delivery, and EIID is extracted from the lower bits

## Test Results

The `aplic_genmsi_debug` sample tests three equivalent formats:

1. **Simple format**: `0x00000840` = `(1U << 11) | 64`
2. **Full format**: `0x00000840` = `(0 << 18) | (0 << 12) | (1U << 11) | 64`
3. **Explicit format**: Same as format 2

All three formats work correctly on both QEMU and nSIM because:
- QEMU extracts EIID as `0x840 & 0x7FF = 0x40 = 64` ✓
- nSIM sees MSI_DEL=1 and EIID=64 ✓

## Important Note on EIP Register Behavior

After a GENMSI write triggers an MSI delivery:
1. The corresponding EIP bit is set in the IMSIC
2. If interrupts are enabled and the EIID threshold is met, MEXT fires immediately
3. The CPU's interrupt handler claims the interrupt via MTOPEI CSR
4. **Claiming automatically clears the EIP bit**

Therefore, reading EIP registers after a successful interrupt delivery will show the bit as **already cleared**. The fact that the ISR was called proves the MSI delivery worked.
