# UART Echo Demo - AIA with Zephyr Integration

This demo shows how to use RISC-V AIA (APLIC + IMSIC) with Zephyr's standard APIs and device tree integration.

## Key Features

This demo demonstrates the **recommended approach** for using AIA in Zephyr drivers:

1. **Device Tree Integration**: Configuration pulled from device tree
   - `DT_REG_ADDR(UART_NODE)` for UART base address
   - `DT_IRQ(UART_NODE, irq)` for IRQ number
   - `DT_IRQ(UART_NODE, priority)` for priority

2. **Zephyr ISR APIs**: Standard interrupt connection
   - `IRQ_CONNECT()` macro for ISR registration
   - `irq_enable()` to enable the EIID

3. **AIA-specific APIs**: MSI routing configuration
   - `riscv_aplic_msi_config_src()` - Configure source mode
   - `riscv_aplic_msi_route()` - Route APLIC source to EIID
   - `riscv_aplic_enable_source()` - Enable the source
   - `riscv_aplic_inject_genmsi()` - Software MSI injection for testing

4. **Manual UART Register Access**: Direct hardware control
   - Simple and reliable for demonstration purposes
   - Shows exactly what's happening at the hardware level

## Interrupt Flow

```
UART Hardware RX Interrupt
         ↓
APLIC Source 10 (configured as edge-triggered)
         ↓
MSI Write (APLIC → IMSIC interrupt file)
         ↓
IMSIC Interrupt File (Hart 0, EIID 32)
         ↓
CPU MEXT (Machine External Interrupt)
         ↓
Zephyr ISR Dispatch Table
         ↓
uart_eiid_isr() - Our ISR function
```

## Building

```bash
export ZEPHYR_SDK_INSTALL_DIR="/home/afonsoo/zephyr-sdk-0.17.2"
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
west build -p -b qemu_riscv32_aia samples/uart_echo_aia_zephyr
```

## Running

### Interactive Mode
```bash
~/qemu-aia/build/qemu-system-riscv32 \
  -nographic \
  -machine virt,aia=aplic-imsic \
  -bios none \
  -m 256 \
  -kernel build/zephyr/zephyr.elf

# Type characters, press Enter, see them echoed
# Press Ctrl+A then X to exit
```

### Automated Test
```bash
{
  sleep 3;
  printf "Hello\n";
  sleep 1;
  printf "Test123\n";
  sleep 1;
} | timeout 10 ~/qemu-aia/build/qemu-system-riscv32 \
  -nographic \
  -machine virt,aia=aplic-imsic \
  -bios none \
  -m 256 \
  -kernel build/zephyr/zephyr.elf
```

## Expected Output

```
╔════════════════════════════════════════════════╗
║  UART Echo - AIA with Zephyr Integration     ║
╚════════════════════════════════════════════════╝

✓ APLIC device ready

═══════════════════════════════════════════════
  Configuration from Device Tree
═══════════════════════════════════════════════
  UART base: 0x10000000
  IRQ number: 10 (APLIC source)
  IRQ priority: 1
  Target EIID: 32

═══════════════════════════════════════════════
  Step 1: Configure AIA Routing
═══════════════════════════════════════════════

[AIA Configuration]
  APLIC source: 10
  Target EIID: 32
  Priority: 1

  1. Configuring APLIC source mode...
     ✓ Source configured as edge-triggered
  2. Routing APLIC source 10 → hart:0 eiid:32
     ✓ MSI route configured
  3. Enabling APLIC source...
     ✓ Source enabled

═══════════════════════════════════════════════
  Step 2: Connect EIID to ISR
═══════════════════════════════════════════════
  Connecting EIID 32 to uart_eiid_isr...
  ✓ ISR connected and EIID enabled

═══════════════════════════════════════════════
  Step 3: Enable UART Hardware Interrupts
═══════════════════════════════════════════════
  Writing 0x01 to UART IER (0x10000000 + 0x01)
  IER readback: 0x01
  ✓ UART RX interrupts enabled

═══════════════════════════════════════════════
  Step 4: Test AIA Path with GENMSI
═══════════════════════════════════════════════

[GENMSI Test]
  Injecting software MSI to EIID 32...
  ✓ GENMSI successfully triggered ISR!
    ISR entry count: 0 -> 1

═══════════════════════════════════════════════
  Interrupt Flow
═══════════════════════════════════════════════

  UART RX → APLIC Source 10 → MSI Write →
  IMSIC EIID 32 → CPU MEXT → uart_eiid_isr()

═══════════════════════════════════════════════
  Ready! Type characters to see them echoed.
═══════════════════════════════════════════════

Hello
[IMSIC claimed EIID 32, dispatching to ISR table]
[Status] ISR entries: 2, RX chars: 6

Test
[IMSIC claimed EIID 32, dispatching to ISR table]
[Status] ISR entries: 3, RX chars: 11
```

## Implementation Details

### Step 1: Device Tree Configuration

```c
/* Get UART configuration from device tree */
#define UART_NODE DT_CHOSEN(zephyr_console)
#define UART_BASE DT_REG_ADDR(UART_NODE)
#define UART_IRQ_NUM DT_IRQ(UART_NODE, irq)
#define UART_IRQ_PRIORITY DT_IRQ(UART_NODE, priority)
```

### Step 2: Configure AIA Routing

```c
const struct device *aplic = riscv_aplic_get_dev();

/* Configure source mode (edge-triggered for UART) */
riscv_aplic_msi_config_src(aplic, UART_IRQ_NUM, APLIC_SM_EDGE_RISE);

/* Route APLIC source to hart 0, EIID 32 */
riscv_aplic_msi_route(aplic, UART_IRQ_NUM, 0, UART_EIID);

/* Enable APLIC source */
riscv_aplic_enable_source(UART_IRQ_NUM);
```

### Step 3: Connect ISR

```c
/* Register ISR for EIID using Zephyr's standard API */
IRQ_CONNECT(UART_EIID, UART_IRQ_PRIORITY, uart_eiid_isr, NULL, 0);
irq_enable(UART_EIID);
```

### Step 4: Enable UART Interrupts

```c
/* Enable UART RX interrupts by writing to IER register */
sys_write8(UART_IER_RDI, UART_BASE + UART_IER);
```

### Step 5: ISR Implementation

```c
static void uart_eiid_isr(const void *arg)
{
	/* Check if data is available */
	uint8_t lsr = uart_read_reg(UART_LSR);
	while (lsr & UART_LSR_DR) {
		/* Read and echo character */
		uint8_t c = uart_read_reg(UART_RBR);
		uart_write_reg(UART_THR, c);

		/* Check for more data */
		lsr = uart_read_reg(UART_LSR);
	}
}
```

## Key Differences from PLIC/CLIC

### With PLIC/CLIC (Traditional)
```c
/* Direct IRQ connection - interrupt controller handles routing */
IRQ_CONNECT(UART_IRQ_NUM, priority, uart_isr, NULL, 0);
irq_enable(UART_IRQ_NUM);
```

### With AIA (This Demo)
```c
/* Step 1: Configure APLIC MSI routing */
riscv_aplic_msi_config_src(aplic, UART_IRQ_NUM, APLIC_SM_EDGE_RISE);
riscv_aplic_msi_route(aplic, UART_IRQ_NUM, hart, EIID);
riscv_aplic_enable_source(UART_IRQ_NUM);

/* Step 2: Connect to EIID (not IRQ number!) */
IRQ_CONNECT(EIID, priority, uart_isr, NULL, 0);
irq_enable(EIID);
```

The key difference: **AIA uses EIIDs for ISR connection**, while the APLIC source number is only used for routing configuration.

## Testing and Verification

The demo includes a GENMSI (software-generated MSI) test that verifies:
1. EIID is properly connected to ISR
2. IMSIC can deliver interrupts to CPU
3. ISR dispatch table is correct

This test runs before any UART activity, ensuring the interrupt path is working before relying on actual hardware interrupts.

## Platform Requirements

- **Board**: qemu_riscv32_aia
- **QEMU**: Build with AIA support (virt,aia=aplic-imsic)
- **Zephyr**: AIA drivers enabled (CONFIG_RISCV_HAS_APLIC + CONFIG_RISCV_HAS_IMSIC)

## Use Case: Driver Development

This demo shows how to write a Zephyr driver that uses AIA:

1. Use device tree macros for configuration
2. Get APLIC device via `riscv_aplic_get_dev()`
3. Configure source mode and routing via AIA APIs
4. Connect ISR to EIID (not source number!)
5. Enable hardware interrupts normally

The approach integrates cleanly with Zephyr's device model while leveraging AIA's advanced features.
