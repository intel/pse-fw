/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Helper file for pse_base main application.
 * This contains the following functions:
 * cmd_reg_read() -- Read data at given address
 * cmd_reg_write() -- Write data at given address
 * cmd_mem_dump() -- Display memory dump
 * @{
 */

/* Local Includes */
#include <kernel.h>
#include <logging/log.h>
#include <sys/printk.h>
#include <sedi.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>
#include <soc.h>
#include <stdlib.h>
#include <zephyr.h>

/** Note: To use commands register read, register write and memory dump
 * enable the MEM_UTILS macro.
 */
#undef MEM_UTILS

char *pse_io_list[] = {
	"I2C 0",  "I2C 1",  "I2C 2",  "I2C 3",  "I2C 4",  "I2C 5",
	"I2C 6",  "I2C 7",  "UART 0", "UART 1", "UART 2", "UART 3",
	"UART 4", "UART 5", "SPI 0",  "SPI 1",  "SPI 2",  "SPI 3",
	"GBE 0",  "GBE 1",  "CAN 0",  "CAN 1",  "GPIO0",  "GPIO1",
	"DMA0",   "DMA1",   "DMA2",   "QEP 0",  "QEP 1",  "QEP 2",
	"QEP 3",  "I2S 0",  "I2S 1",  "PWM",    "IPC", NULL
};

/* @brief display IO ownership
 * Display PSE IO's ownerread settings status.
 */
static int cmd_ownership(const struct shell *shell, size_t argc, char **argv)
{
	/* Use ARG_UNUSED([arg]) to avoid compiler
	 * warnings about unused arguments
	 */
	ARG_UNUSED(shell);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int i = 0;
	sedi_dev_ownership_t ownership = 0;

	for (i = 0; i < PSE_DEV_MAX; i++) {
		printk("%2d. %8s : ", i, pse_io_list[i]);
		ownership = sedi_get_dev_ownership((sedi_dev_table_t)i);
		if (ownership == DEV_PSE_OWNED) {
			printk("[ PSE  ]");
		} else if ((ownership == DEV_LH_OWNED_MSI) ||
			   (ownership == DEV_LH_OWNED_SB)) {
			printk("[ Host ]");
			if (ownership == DEV_LH_OWNED_MSI) {
				printk(" [ MSI ]");
			} else if (ownership == DEV_LH_OWNED_SB) {
				printk(" [ SB  ]");
			}
		} else {
			printk("[ None ]");
		}
		printk("\n");
	}
	return 0;
}

#ifdef MEM_UTILS
/* @brief read register
 * Read data at address given
 * by command line argument.
 */
static int cmd_reg_read(const struct shell *shell, size_t argc, char **argv)
{
	int size;
	unsigned int addr;
	unsigned int data;

	/* Get the command line arguments. */
	if (argc > 1) {
		size = atoi(argv[1]);
	}

	if (argc > 2) {
		addr = strtol(argv[2], NULL, 16);
	} else {
		printk(
			"Invalid arguments: reg_read <size 8/16/32> <address>\n");
		return 0;
	}

	/* Read data. */
	switch (size) {
	case 8:
		data = sys_read8(addr);
		break;

	case 16:
		data = sys_read16(addr);
		break;

	case 32:
		data = sys_read32(addr);
		break;

	default:
		printk("Invalid size, reg_read <size 8/16/32> <address>\n");
		return 0;
	}
	printk("%d bit value at %x :  %x\n", size, addr, data);
	return 0;
}

/* @brief write register
 * Write data at address given by
 * command line arguments.
 */
static int cmd_reg_write(const struct shell *shell, size_t argc, char **argv)
{
	int size;
	unsigned int addr;
	unsigned int data;

	/* Get the command line arguments. */
	if (argc > 1) {
		size = atoi(argv[1]);
	}

	if (argc > 2) {
		addr = strtol(argv[2], NULL, 16);
	}

	if (argc > 3) {
		data = strtol(argv[3], NULL, 16);
	} else {
		printk("Invalid arguments: reg_write <size 8/16/32> <address> "
		       "<data>\n");
		return 0;
	}

	/* Write data. */
	switch (size) {
	case 8:
		sys_write8((data & 0xFF), addr);
		break;

	case 16:
		sys_write16((data & 0xFFFF), addr);
		break;

	case 32:
		sys_write32(data, addr);
		break;

	default:
		printk("Invalid size, reg_read <size 8/16/32> <address>\n");
		return 0;
	}
	printk("%d bit value wrote at %x :  %x\n", size, addr, data);
	return 0;
}

/* @brief memory dump
 * Display memory dump
 */
static int cmd_mem_dump(const struct shell *shell, size_t argc, char **argv)
{
	int dwords, i, j = 0;
	unsigned int addr;
	unsigned int data;
	unsigned int columns = 8;

	/* Get the command line arguments. */
	if (argc > 1) {
		dwords = atoi(argv[1]);
	}

	if (argc > 2) {
		addr = strtol(argv[2], NULL, 16);
	} else {
		printk("Invalid arguments: mem_dump <dwords> <address>"
		       "[<columns>]\n");
		return 0;
	}

	if (argc > 3) {
		columns = strtol(argv[3], NULL, 16);
		if (columns < 1) {
			printk("Invalid arguments: mem_dump <dwords> <address>"
			       "<columns (should be greater than 0)>\n");
			return 0;
		}
	}

	/* Print memory dump. */
	printk("\n %08X: ", addr);
	for (i = 0; i < (dwords * 4); i += 4) {
		if (j == columns) {
			printk("\n %08X: ", (addr + i));
			j = 0;
		}
		data = sys_read32((addr + i));
		printk("%08X ", data);
		j++;
	}
	printk("\n");
	return 0;
}

SHELL_CMD_REGISTER(reg_read, NULL, "Read register", cmd_reg_read);
SHELL_CMD_REGISTER(reg_write, NULL, "Write register", cmd_reg_write);
SHELL_CMD_REGISTER(mem_dump, NULL, "Dump memory", cmd_mem_dump);
#endif

SHELL_CMD_REGISTER(ownership, NULL, "Display IO's ownership", cmd_ownership);

/**
 * @}
 */
