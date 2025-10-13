# Copyright (c) 2025 Synopsys, Inc.
# SPDX-License-Identifier: Apache-2.0

# UART Echo AIA Demo - Summary

## What This Demo Shows

This demo demonstrates **the recommended way** to use RISC-V AIA (APLIC + IMSIC) in Zephyr applications.

### Key Achievements

✅ **Device Tree Integration**
- Configuration pulled from device tree using `DT_*` macros
- No hardcoded addresses or IRQ numbers

✅ **Standard Zephyr APIs**
- `IRQ_CONNECT()` for ISR registration
- `irq_enable()` for enabling interrupts
- Compatible with existing Zephyr code patterns

✅ **AIA-Specific APIs**
- `riscv_aplic_msi_config_src()` - Configure interrupt source mode
- `riscv_aplic_msi_route()` - Route APLIC source to IMSIC EIID
- `riscv_aplic_inject_genmsi()` - Software-generated MSI for testing

✅ **Complete Interrupt Path**
```
UART Hardware → APLIC Source 10 → MSI Write → IMSIC EIID 32 → CPU MEXT → ISR
```

## Test Results

```bash
$ bash /tmp/test_uart_proper.sh

Hello
[00:00:02.970,000] <inf> intc_riscv_imsic: MEXT claimed EIID 32, dispatching to ISR table
[Status] ISR entries: 2, RX chars: 6

Test
[00:00:03.980,000] <inf> intc_riscv_imsic: MEXT claimed EIID 32, dispatching to ISR table
[Status] ISR entries: 3, RX chars: 11
```

### What This Proves

1. **APLIC MSI delivery works**: Hardware UART interrupts trigger APLIC
2. **MSI write succeeds**: APLIC successfully writes MSI to IMSIC
3. **IMSIC receives and delivers**: IMSIC claims EIID and triggers CPU MEXT
4. **ISR executes correctly**: ISR reads UART data and echoes it back
5. **GENMSI works**: Software-generated MSI can test the interrupt path

## Quick Start

### Build
```bash
west build -p -b qemu_riscv32_aia samples/uart_echo_aia_zephyr
```

### Test
```bash
bash /tmp/test_uart_proper.sh
```

### Run Interactively
```bash
bash /tmp/run_aia_interactive.sh
# Type characters, press Enter
# Press Ctrl+A then X to exit
```

## Code Structure

```
samples/uart_echo_aia_zephyr/
├── CMakeLists.txt          # Build configuration
├── prj.conf                # Project Kconfig settings
├── src/
│   └── main.c              # Demo implementation
├── README.md               # Detailed documentation
└── DEMO_SUMMARY.md         # This file
```

### Main Components (src/main.c)

1. **Device Tree Configuration** (lines 23-26)
   ```c
   #define UART_BASE DT_REG_ADDR(UART_NODE)
   #define UART_IRQ_NUM DT_IRQ(UART_NODE, irq)
   ```

2. **AIA Configuration** (lines 92-132)
   ```c
   riscv_aplic_msi_config_src(aplic, UART_IRQ_NUM, APLIC_SM_EDGE_RISE);
   riscv_aplic_msi_route(aplic, UART_IRQ_NUM, 0, UART_EIID);
   riscv_aplic_enable_source(UART_IRQ_NUM);
   ```

3. **ISR Registration** (lines 199-201)
   ```c
   IRQ_CONNECT(UART_EIID, UART_IRQ_PRIORITY, uart_eiid_isr, NULL, 0);
   irq_enable(UART_EIID);
   ```

4. **ISR Implementation** (lines 63-87)
   ```c
   static void uart_eiid_isr(const void *arg) {
       // Read UART data and echo back
   }
   ```

## Comparison: PLIC vs AIA

### Traditional PLIC Approach
```c
// Simple: IRQ number maps directly to ISR
IRQ_CONNECT(UART_IRQ_NUM, priority, uart_isr, NULL, 0);
irq_enable(UART_IRQ_NUM);
```

### AIA Approach (This Demo)
```c
// Step 1: Configure APLIC routing (IRQ -> EIID mapping)
riscv_aplic_msi_route(aplic, UART_IRQ_NUM, hart, EIID);

// Step 2: Connect ISR to EIID (not IRQ number!)
IRQ_CONNECT(EIID, priority, uart_isr, NULL, 0);
irq_enable(EIID);
```

**Key Insight**: With AIA, ISRs connect to EIIDs, not hardware IRQ numbers. The APLIC handles the mapping from hardware IRQs to EIIDs via MSI delivery.

## Benefits of This Approach

1. **Flexible Routing**: APLIC can route any source to any EIID
2. **MSI Delivery**: Modern interrupt delivery mechanism
3. **Per-Hart Control**: Each hart has independent IMSIC interrupt file
4. **Software Testing**: GENMSI allows interrupt path testing without hardware
5. **Virtualization Ready**: Guest/host interrupt separation

## Platform Support

- **Tested on**: QEMU RISC-V32 virt machine with AIA
- **Board**: qemu_riscv32_aia
- **Required**: QEMU with AIA support (virt,aia=aplic-imsic)

## Related Demos

1. **uart_echo_aia**: Manual UART demo (no Zephyr UART driver)
   - Shows low-level UART register handling
   - Educational for understanding hardware details

2. **uart_echo_aia_zephyr** (this demo): Zephyr-integrated approach
   - Uses device tree and standard Zephyr APIs
   - Recommended for production driver development

## Next Steps

To integrate AIA into your own Zephyr driver:

1. Get APLIC device: `riscv_aplic_get_dev()`
2. Choose an EIID (32+ typically available for applications)
3. Configure source mode: `riscv_aplic_msi_config_src()`
4. Route to EIID: `riscv_aplic_msi_route()`
5. Enable source: `riscv_aplic_enable_source()`
6. Connect ISR to EIID: `IRQ_CONNECT(EIID, ...)`
7. Test with GENMSI: `riscv_aplic_inject_genmsi()`
8. Enable hardware interrupts normally

See `src/main.c` for a complete working example.
