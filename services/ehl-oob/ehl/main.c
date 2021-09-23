/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * \brief Entry point to ehl-oob manageability. Performs
 * the following:
 *
 * Starts the Manageability application by calling
 * the ehl_oob_bootstrap
 *
 **/

#include <ehl/ehl-oob.h>
#include <common/utils.h>

int main(void)
{
	ehl_oob_bootstrap();
	return OOB_SUCCESS;
}
