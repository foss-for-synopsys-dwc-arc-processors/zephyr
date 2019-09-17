/*
 * Copyright (c) 2019 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Board configuration macros for EM Starter kit board
 *
 * This header file is used to specify and describe board-level
 * aspects for the target.
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <sys/util.h>

/* default system clock */
/* On the EM Wuhan board, the peripheral bus clock frequency is 50Mhz */
#define SYSCLK_DEFAULT_IOSC_HZ			MHZ(50)

/* ARC EM Core IRQs */
#define IRQ_TIMER0				16
#define IRQ_TIMER1				17

#define IRQ_CORE_DMA_COMPLETE			20
#define IRQ_CORE_DMA_ERROR			21


#ifndef _ASMLANGUAGE

#include <sys/util.h>
#include <random/rand32.h>

#define INT_ENABLE_ARC				~(0x00000001 << 8)
#define INT_ENABLE_ARC_BIT_POS			(8)

/*
 * UARTs: UART0 & UART1 & UART2
 */
#define DT_UART_NS16550_PORT_0_IRQ_FLAGS	0 /* Default */
#define DT_UART_NS16550_PORT_1_IRQ_FLAGS	0 /* Default */
#define DT_UART_NS16550_PORT_2_IRQ_FLAGS	0 /* Default */


#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
