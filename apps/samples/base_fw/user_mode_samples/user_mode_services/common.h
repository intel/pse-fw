/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMON_H
#define COMMON_H
struct data_item_t {
	void *fifo_reserved; /* 1st word reserved for use by fifo */
	char buff[100];
};
#endif
