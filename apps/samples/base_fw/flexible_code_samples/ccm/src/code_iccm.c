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
 * Both variables will be stored in Data Closely Coupled Memory (DCCM).
 * The uninitialized variable will be stored in "BSS" section of the DCCM
 * that is used especially for uninitialized variables.
 */

/* Local Includes */
#include <sys/printk.h>
#include <zephyr.h>

/* Initialized Variable to be stored in DCCM */
uint32_t dccm_var_in_data_section = 10U;
/* Uninitialized Variable to be stored in the BSS section of the DCCM */
uint32_t dccm_var_in_bss;

/* Invoke function in CCM */
void function_in_ccm(void)
{
	printk("function in CCM at address:%p\n", function_in_ccm);
	printk("var_in_data_section in DCCM at address:%p\n",
	       &dccm_var_in_data_section);
	printk("var_in_bss in DCCM  at address:%p\n", &dccm_var_in_bss);
}
