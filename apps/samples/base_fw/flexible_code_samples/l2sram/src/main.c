/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample Application for code relocation
 * The sample application will demonstrate how to relocate the application
 * code text, data and bss section to available memory options, such as
 * ICCM, DCCM, and L2SRAM memory.
 * This sample app will use L2SRAM for Base FW.
 * There is a BSS section of the DCCM and L2SRAM that is used
 * especially for uninitialized variables.
 * Refer to the helper C files in this folder for the functions used
 * in the main function below.
 * The functions used below print the address of a variable stored in data
 * section and address of a varible stored in bss section of that memory.
 * CMakeLists file controls which memory is used for each of the helper C files
 * All the instructions, initialized and uninitialized variables in a helper
 * C file will be stored in the memory option mentioned in CMakeLists.
 * @{
 */

/**
 * @brief How to Build sample application.
 * Please refer “IntelPSE_SDK_Get_Started_Guide” for more details how
 * to build the sample codes.
 */

/**
 * @brief Hardware setup.
 * There is no specific external hardware setup required for the flexible
 * code relocation sample application.
 */

/* Local Includes */
#include <sys/printk.h>
#include <zephyr.h>

extern void function_in_ccm(void);
extern void function_in_l2sram(void);
extern void function_in_split_mem(void);

/* @brief main function
 * Code relocation using L2SRAM for Base FW.
 * and CCM for code text, data and bss sections.
 */
void main(void)
{
	printk("%s\n", __func__);

	printk("invoke function in L2SRAM..\n");
	function_in_l2sram();

	printk("invoke function in CCM..\n");
	function_in_ccm();

	printk("invoke function in split memory..\n");
	function_in_split_mem();
}
/**
 * @}
 */
