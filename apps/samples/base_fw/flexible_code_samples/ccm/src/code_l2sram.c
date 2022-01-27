/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief An elementary function to use with the Closely Coupled Memory (CCM)
 * sample application (main.c)
 *
 * This function uses one initialized variable, one uninitialized variable,
 * and prints the address of the variables and the function itself.
 * Both variables will be stored in L2 SRAM Cache (L2SRAM).
 * The uninitialized variable will be stored in "BSS" section of the L2SRAM
 * that is used especially for uninitialized variables.
 */

/* Local Includes */
#include <sys/printk.h>
#include <zephyr.h>

/* Initialized Variable to be stored in L2SRAM */
uint32_t l2sram_var2_in_data_section = 10U;
/* Uninitialized Variable to be stored in the BSS section of the L2SRAM */
uint32_t l2sram_var2_in_bss;

/* Invoke function in L2SRAM */
void function_in_l2sram(void)
{
	printk("function in L2SRAM at address:%p\n", function_in_l2sram);
	printk("var_in_data_section in L2SRAM at address:%p\n",
	       &l2sram_var2_in_data_section);
	printk("var_in_bss  in L2SRAM at address:%p\n", &l2sram_var2_in_bss);
}
