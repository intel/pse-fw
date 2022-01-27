/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief An elementary function to use with the L2 SRAM
 * sample application (main.c)
 *
 * This function demonstrates using two types of memory for one source file.
 * It uses one initialized variable, one uninitialized variable,
 * and prints the address of the varibles and the function itself.
 * The initialized variable will be stored in DCCM Data section.
 * The uninitialized variable will be stored in "BSS" section of the
 * DCCM that is used especially for uninitialized variables.
 */

/* Local Includes */
#include <sys/printk.h>
#include <zephyr.h>

/* Initialized Variable to be stored in data section of DCCM */
uint32_t split_var_in_data_section = 10U;
/* Uninitialized Variable to be stored in the BSS section of DCCM */
uint32_t split_var_in_bss;

/* Invoke function in split memory.
 * Code text, data and bss sections are stored in CCM
 */
void function_in_split_mem(void)
{
	printk("function in split mem at address:%p\n", function_in_split_mem);
	printk("var_in_data_section in split mem (DCCM) at address:%p\n",
	       &split_var_in_data_section);
	printk("var_in_bss in split mem (DCCM) at address:%p\n",
	       &split_var_in_bss);
}
