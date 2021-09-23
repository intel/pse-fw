/**
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 *
 * @brief Private APIs for the BIOS OOB IPC handler.
 */

#ifndef PSE_OOB_SEC_HECI_CLIENT_INTERNAL_H
#define PSE_OOB_SEC_HECI_CLIENT_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* {892D1D46-6B91-4B62-8A99-EBF72878667E} */
#define SEC_HC_GUID {						       \
		0x892D1D46, 0x6B91, 0x4B62, {			       \
			0x8A, 0x99, 0xEB, 0xF7, 0x28, 0x78, 0x66, 0x7E \
		}						       \
}

#define SEC_HC_MAX_RX_SIZE        4096
#define SEC_HC_STACK_SIZE         4096*2
#define SEC_HC_HEAP_SIZE          4096

#define SEC_HC_MAJOR_MINOR_VERSION 0x2
/* 1.00 = 0x0
 * 1.01 = 0x1
 * 1.02 = 0x2
 */

#ifdef __cplusplus
}
#endif

#endif /* PSE_OOB_SEC_HECI_CLIENT_INTERNAL_H */
