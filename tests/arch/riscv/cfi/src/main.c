/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISC-V Control Flow Integrity (CFI) test
 *
 * Tests for Zicfiss (shadow stack) and Zicfilp (landing pad) extensions.
 *
 * Shadow Stack tests:
 *  - test_shstk_enabled:    Verify shadow stack CSR (ssp) is non-zero on a
 *                           newly started thread (i.e. shadow stack is active).
 *  - test_shstk_ctx_switch: Create two threads, context-switch between them,
 *                           and verify each has its own independent ssp value.
 *  - test_shstk_irq:        Fire an irq_offload inside a thread (nested IRQ)
 *                           and verify execution returns cleanly, confirming
 *                           the IRQ shadow stack path works.
 *  - test_shstk_tamper:     Corrupt the return address on the regular stack.
 *                           The hardware should detect the mismatch and raise
 *                           a store/AMO access-fault or illegal instruction
 *                           exception (implementation-defined), caught by
 *                           k_sys_fatal_error_handler.
 *
 * Landing Pad tests:
 *  - test_lpad_direct_call: A direct function call must reach a valid lpad
 *                           target and return normally.
 *  - test_lpad_indirect_valid: An indirect call through a function pointer
 *                           that points to a lpad-prefixed function must
 *                           succeed.
 *
 * How to build and run (QEMU, 64-bit):
 *
 *   west build -b qemu_riscv64 \
 *       tests/arch/riscv/cfi \
 *       -- -DCONFIG_RISCV_CFI=y \
 *          -DCONFIG_RISCV_CFI_SHADOW_STACK=y \
 *          -DCONFIG_HW_SHADOW_STACK=y
 *   west build -t run
 *
 * Or via twister:
 *   ./scripts/twister -p qemu_riscv64 \
 *       -T tests/arch/riscv/cfi --inline-logs
 */

#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/arch/riscv/csr.h>

#define STACKSIZE 1024
#define THREAD_PRIORITY 5

/* ------------------------------------------------------------------ */
/*  Fatal-error hook (catches expected faults)                         */
/* ------------------------------------------------------------------ */

volatile bool expect_fault;
volatile unsigned int expect_reason;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	if (expect_fault) {
		printk("expected fatal error: reason=%u\n", reason);
		expect_fault = false;
		/* Returning from the handler causes the faulting thread to be
		 * aborted by the kernel, which is the desired behaviour here.
		 */
	} else {
		printk("unexpected fatal error: reason=%u\n", reason);
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}
}

/* ------------------------------------------------------------------ */
/*  Shadow Stack tests (CONFIG_HW_SHADOW_STACK)                        */
/* ------------------------------------------------------------------ */

#ifdef CONFIG_HW_SHADOW_STACK

/*
 * ssp = CSR 0x011 (Zicfiss Shadow Stack Pointer).
 * Use the numeric address because the assembler only recognises the
 * "ssp" mnemonic when -march includes zicfiss.  Zephyr's cmake march
 * string is built from the standard ISA letter extensions and does not
 * yet automatically append "zicfiss", so we reference the CSR by number.
 */
#define RISCV_CSR_SSP 0x011

static inline uintptr_t read_ssp(void)
{
	uintptr_t val;

	__asm__ volatile("csrr %0, " STRINGIFY(RISCV_CSR_SSP) : "=r"(val));
	return val;
}

/* ---------- test_shstk_enabled ---------- */

static void shstk_enabled_thread(void *p1, void *p2, void *p3)
{
	uintptr_t ssp = read_ssp();

	zassert_not_equal(ssp, 0,
		"shadow stack pointer is zero — shadow stack not active");
	printk("thread ssp = 0x%lx\n", (unsigned long)ssp);
}

K_THREAD_STACK_DEFINE(shstk_enabled_stack, STACKSIZE);
static struct k_thread shstk_enabled_tcb;

ZTEST(riscv_cfi, test_shstk_enabled)
{
	k_tid_t tid = k_thread_create(&shstk_enabled_tcb,
				      shstk_enabled_stack, STACKSIZE,
				      shstk_enabled_thread,
				      NULL, NULL, NULL,
				      THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_join(tid, K_FOREVER);
}

/* ---------- test_shstk_ctx_switch ---------- */

K_SEM_DEFINE(sem_a_go,   0, 1);
K_SEM_DEFINE(sem_b_go,   0, 1);
K_SEM_DEFINE(sem_a_done, 0, 1);
K_SEM_DEFINE(sem_b_done, 0, 1);

static uintptr_t ssp_thread_a;
static uintptr_t ssp_thread_b;

static void ctx_thread_a(void *p1, void *p2, void *p3)
{
	k_sem_take(&sem_a_go, K_FOREVER);
	ssp_thread_a = read_ssp();
	k_sem_give(&sem_a_done);
}

static void ctx_thread_b(void *p1, void *p2, void *p3)
{
	k_sem_take(&sem_b_go, K_FOREVER);
	ssp_thread_b = read_ssp();
	k_sem_give(&sem_b_done);
}

K_THREAD_STACK_DEFINE(ctx_stack_a, STACKSIZE);
K_THREAD_STACK_DEFINE(ctx_stack_b, STACKSIZE);
static struct k_thread ctx_tcb_a, ctx_tcb_b;

ZTEST(riscv_cfi, test_shstk_ctx_switch)
{
	k_thread_create(&ctx_tcb_a, ctx_stack_a, STACKSIZE,
			ctx_thread_a, NULL, NULL, NULL,
			THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_create(&ctx_tcb_b, ctx_stack_b, STACKSIZE,
			ctx_thread_b, NULL, NULL, NULL,
			THREAD_PRIORITY, 0, K_NO_WAIT);

	k_sem_give(&sem_a_go);
	k_sem_give(&sem_b_go);

	k_sem_take(&sem_a_done, K_FOREVER);
	k_sem_take(&sem_b_done, K_FOREVER);

	zassert_not_equal(ssp_thread_a, 0, "thread A ssp is zero");
	zassert_not_equal(ssp_thread_b, 0, "thread B ssp is zero");
	zassert_not_equal(ssp_thread_a, ssp_thread_b,
		"threads A and B share the same shadow stack pointer");

	printk("thread A ssp=0x%lx  thread B ssp=0x%lx\n",
	       (unsigned long)ssp_thread_a,
	       (unsigned long)ssp_thread_b);
}

/* ---------- test_shstk_irq ---------- */

K_SEM_DEFINE(irq_done_sem, 0, 1);

static void nested_irq_handler(const void *p)
{
	uintptr_t ssp = read_ssp();

	zassert_not_equal(ssp, 0, "ssp zero inside IRQ handler");
	printk("IRQ handler ssp = 0x%lx\n", (unsigned long)ssp);

	if (p != NULL) {
		/* One level of nesting. */
		irq_offload(nested_irq_handler, NULL);
		k_sem_give((struct k_sem *)p);
	}
}

static void irq_thread(void *p1, void *p2, void *p3)
{
	irq_offload(nested_irq_handler, &irq_done_sem);
	k_sem_take(&irq_done_sem, K_FOREVER);
}

K_THREAD_STACK_DEFINE(irq_stack, STACKSIZE);
static struct k_thread irq_tcb;

ZTEST(riscv_cfi, test_shstk_irq)
{
	k_tid_t tid = k_thread_create(&irq_tcb, irq_stack, STACKSIZE,
				      irq_thread,
				      NULL, NULL, NULL,
				      THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_join(tid, K_FOREVER);
}

/* ---------- test_shstk_tamper ---------- */
/*
 * Corrupt the saved return address on the normal stack, then explicitly
 * execute sspopchk to compare against the shadow stack.
 *
 * sspopchk ra  =  0x0010c073  (Zicfiss encoding, CSR instruction space)
 *
 * Encoding breakdown:
 *   funct7=0000000  rs2=00001(ra)  rs1=00001(ra)  funct3=100
 *   opcode=1110011  rd=00000
 *   → 0000000 00001 00001 100 00000 1110011 = 0x0010_C073
 *
 * We emit it via .insn so it assembles regardless of -march.
 * The hardware pops the original RA from the shadow stack and compares with
 * the register value — mismatch triggers a software-check exception (mcause=18).
 */

K_SEM_DEFINE(tamper_sem, 0, 1);

/* sspopchk ra: pops shadow stack top and checks it equals ra register. */
#define SSPOPCHK_RA() __asm__ volatile(".insn r 0x73, 0x4, 0x1, x0, ra, ra")

static void __attribute__((noinline, optimize("O0"))) tamper_ra(void)
{
	/*
	 * Frame layout at -O0 (s0 = frame pointer):
	 *   s0 - 1*8 = saved ra  ← overwrite this
	 *   s0 - 2*8 = saved s0
	 */
	register uintptr_t fp __asm__("s0");
	uintptr_t *ra_slot = (uintptr_t *)(fp - sizeof(uintptr_t));

	printk("Corrupting RA at %p (was 0x%lx)\n", ra_slot, *ra_slot);
	*ra_slot = 0xdeadbeef0000UL;

	/*
	 * Load the corrupted RA into the ra register then run sspopchk.
	 * The shadow stack still holds the original return address → MISMATCH
	 * → software-check exception → k_sys_fatal_error_handler.
	 */
	__asm__ volatile("ld ra, %0" :: "m"(*ra_slot));
	SSPOPCHK_RA();
}

static void tamper_thread(void *p1, void *p2, void *p3)
{
	tamper_ra();
	zassert_unreachable("tamper_ra should have faulted");
}

K_THREAD_STACK_DEFINE(tamper_stack, STACKSIZE);
static struct k_thread tamper_tcb;

ZTEST(riscv_cfi, test_shstk_tamper)
{
	k_tid_t tid = k_thread_create(&tamper_tcb, tamper_stack, STACKSIZE,
				      tamper_thread,
				      NULL, NULL, NULL,
				      THREAD_PRIORITY, 0, K_NO_WAIT);

	expect_fault = true;

	/* Wait for the thread to finish (via abort after fault). */
	k_thread_join(tid, K_FOREVER);

	zassert_false(expect_fault,
		"shadow stack tamper did not produce a fault — "
		"is Zicfiss enforcement enabled?");
}

#endif /* CONFIG_HW_SHADOW_STACK */

/* ------------------------------------------------------------------ */
/*  Landing Pad tests (CONFIG_RISCV_CFI_LANDING_PAD)                  */
/* ------------------------------------------------------------------ */

#ifdef CONFIG_RISCV_CFI_LANDING_PAD

/*
 * A function with a landing pad prefix.  When compiled with -mzicfilp the
 * compiler emits an `lpad 0` at the entry of every function that may be
 * called indirectly.
 */
static int __attribute__((noinline)) lpad_target(int x)
{
	return x + 1;
}

/* Indirect call wrapper — prevents the compiler from devirtualising. */
static int do_indirect_call(int (*fn)(int), int arg)
{
	return fn(arg);
}

ZTEST(riscv_cfi, test_lpad_direct_call)
{
	int ret = lpad_target(41);

	zassert_equal(ret, 42, "direct call to lpad target returned wrong value");
}

ZTEST(riscv_cfi, test_lpad_indirect_valid)
{
	int ret = do_indirect_call(lpad_target, 41);

	zassert_equal(ret, 42,
		"indirect call to valid lpad target returned wrong value");
}

#endif /* CONFIG_RISCV_CFI_LANDING_PAD */

/* ------------------------------------------------------------------ */
/*  Test suite registration                                            */
/* ------------------------------------------------------------------ */

ZTEST_SUITE(riscv_cfi, NULL, NULL, NULL, NULL, NULL);
