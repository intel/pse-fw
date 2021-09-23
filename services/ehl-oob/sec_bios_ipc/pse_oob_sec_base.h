/**
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifndef PSE_OOB_SEC_BASE_H
#define PSE_OOB_SEC_BASE_H

/**
 * General Define
 * ============================================================================
 */

/**
 * Debug
 */

/**
 * SEC_DEBUG MACRO could be enabled for extended
 * Debug Details, this will be default Disabled
 */
#define SEC_DEBUG 0

/**
 * Enable SEC_UNIT_TEST in case of Unit Test
 */
#define SEC_UNIT_TEST 0
#define SEC_UNIT_TEST_PARAM_SIZE 10

#if (defined(SEC_UNIT_TEST) && SEC_UNIT_TEST)
extern void *unit_test_param[SEC_UNIT_TEST_PARAM_SIZE];
#if (defined(_WIN32) && _WIN32)
#define SEC_ASSERT(condition) do {			  \
		if (!(condition)) {			  \
			printf("SEC Assertion: "	  \
			       "Failed at %s: line %d\n", \
			       __func__,		  \
			       __LINE__);		  \
			result = SEC_FAILED;		  \
		} } while (0)
#else
#define SEC_ASSERT(condition) do {			  \
		if (!(condition)) {			  \
			printk("SEC Assertion: "	  \
			       "Failed at %s: line %d\n", \
			       __func__, __LINE__);	  \
			result = SEC_FAILED;		  \
		} } while (0)
#endif  /* #if (defined(_WIN32) && _WIN32) */
#endif  /* SEC_UNIT_TEST */

#if defined SEC_DEBUG
#define SEC_ASSERT(condition) do {				  \
		if (!condition)					  \
			break;					  \
		printk("SEC Assertion: "			  \
		       "Failed at %s: line %d\n",		  \
		       __func__, __LINE__);			  \
		__ASSERT(condition != 0, "returned non success!"); \
} while (0)
#else
#define SEC_ASSERT(condition)			     \
	printk("SEC Assertion: "		     \
	       "Failed at %s: line %d, Param: %d\n", \
	       __func__, __LINE__, condition);
#endif /* SEC_DEBUG */

/**
 * Multi-threads or Tasks Support
 */
#define SEC_MULTI_THREAD_TASK_EN 1

/**
 * Alias for function parameters' direction
 */
#define SEC_IN
#define SEC_OUT
#define SEC_INOUT
#define SEC_RTN

/**
 * Alias for BOOLEAN value
 */
#define SEC_TRUE 1
#define SEC_FALSE 0

/**
 * Alias for FAILED/SUCCESS value
 */
#define SEC_FAILED 1
#define SEC_SUCCESS 0

/**
 * Alias for specific error code
 */
#define SEC_FAILED_INVALID_PARAM 2
#define SEC_FAILED_INVALID_PARAM_LEN 3
#define SEC_FAILED_INVALID_PARAM_VAL 4

/**
 * Alias for specific error code
 */
#define SEC_BYTE0_SET_MSK 0x000000FF
#define SEC_BYTE0_CLR_MSK 0xFFFFFF00
#define SEC_BYTE1_SET_MSK 0x0000FF00
#define SEC_BYTE1_CLR_MSK 0xFFFF00FF
#define SEC_BYTE2_SET_MSK 0x00FF0000
#define SEC_BYTE2_CLR_MSK 0xFF00FFFF
#define SEC_BYTE012_SET_MSK 0x00FFFFFF
#define SEC_BYTE012_CLR_MSK 0xFF000000

#define SEC_BYTE0 0
#define SEC_BYTE1 1
#define SEC_BYTE2 2
#define SEC_BYTE3 3
#endif /* PSE_OOB_SEC_BASE_H */
