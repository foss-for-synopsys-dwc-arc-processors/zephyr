/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/mem_mgmt/mem_attr_heap.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr-sw.h>
#include <zephyr/sys/__assert.h>

int main(void)
{
	printf("Hello World! %s\n", CONFIG_BOARD);

	int ret = mem_attr_heap_pool_init();
	__ASSERT(ret == 0, "");

	void *block = mem_attr_heap_alloc(DT_MEM_SW_ALLOC_NON_CACHE, 0x100);
	__ASSERT(((uintptr_t) block), "");
	mem_attr_heap_free(block);

	block = mem_attr_heap_aligned_alloc(DT_MEM_SW_ALLOC_NON_CACHE, 0x100, 32);
	__ASSERT(((uintptr_t) block % 32 == 0), "");
	printf("alloc at %lx\n", (uintptr_t)block);

	return 0;
}
