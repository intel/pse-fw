/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief provide common function and macro for application.
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#include <zephyr.h>
#include <sys/printk.h>
#include <stdbool.h>
#include <logging/log.h>
#include <stdio.h>

/** Error codes.*/
#define SUCCESS         0
/** Error codes.*/
#define FAILURE         1


/** Error codes.*/
#define ERROR           -1

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are used internally.
 */
#define BIT0    BIT(0)
#define BIT1    BIT(1)
#define BIT2    BIT(2)
#define BIT3    BIT(3)
#define BIT4    BIT(4)
#define BIT5    BIT(5)
#define BIT6    BIT(6)
#define BIT7    BIT(7)
#define BIT8    BIT(8)
#define BIT9    BIT(9)
#define BIT10   BIT(10)
#define BIT11   BIT(11)
#define BIT12   BIT(12)
#define BIT13   BIT(13)
#define BIT14   BIT(14)
#define BIT15   BIT(15)
#define BIT16   BIT(16)
#define BIT17   BIT(17)
#define BIT18   BIT(18)
#define BIT19   BIT(19)
#define BIT20   BIT(20)
#define BIT21   BIT(21)
#define BIT22   BIT(22)
#define BIT23   BIT(23)
#define BIT24   BIT(24)
#define BIT25   BIT(25)
#define BIT26   BIT(26)
#define BIT27   BIT(27)
#define BIT28   BIT(28)
#define BIT29   BIT(29)
#define BIT30   BIT(30)
#define BIT31   BIT(31)
/**
 * @endcond
 */


/**
 * @brief This function finds bit position from given mask.
 *
 * @Param mask is bit map.
 *
 * @retval is bit position.
 */
static inline uint8_t find_bit_position(uint32_t mask)
{
	int position = -1;

	while (mask) {
		mask >>= 1;
		position++;
	}
	return position;
}

#ifdef CONFIG_ECLITE_DEBUG
/** EClite debug macro.*/
#define ECLITE_LOG_DEBUG(param ...) printk("\n[ECL_DBG] " param)
#else
/** EClite debug macro.*/
#define ECLITE_LOG_DEBUG(param ...)

#endif  /* CONFIG_ECLITE_DEBUG */

#endif  /* _COMMON_H_ */
