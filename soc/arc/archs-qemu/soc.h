/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Board configuration macros for HS QEMU
 *
 * This header file is used to specify and describe board-level
 * aspects for the target.
 */

#ifndef _SOC__H_
#define _SOC__H_

/* ARC HS Core IRQs */
#define IRQ_TIMER0			16
#define IRQ_TIMER1			17

#ifndef _ASMLANGUAGE

#include <random/rand32.h>

#define INT_ENABLE_ARC				~(0x00000001 << 8)
#define INT_ENABLE_ARC_BIT_POS			(8)

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
