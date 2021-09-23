/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifndef PSE_OOB_SEC_ENUM_H
#define PSE_OOB_SEC_ENUM_H

/*
 * Define provision state
 * e.g. provisioned, decommissioned,
 * pending decommissioned/reprovisioned.
 */
enum sec_prov_state {
	SEC_PROV,
	SEC_DECOM,
	SEC_RPROV,
	SEC_PEND
};

/*
 * This defines the type of mode OOB_SEC
 * is working in.It is default to daemon
 * mode per PSE architecture. Normal
 * mode is not supported currently.
 */
enum sec_mode {
	SEC_NORMAL,
	SEC_DAEMON
};

/*
 * This defines the type of credentials
 * to be decommissioned/reprovisioned
 * between OOB_AGENT and OOB_SEC.
 */
enum sec_info_type {
	SEC_DEV_ID,
	SEC_TOK_ID,
	SEC_CLD_HOST_URL,
	SEC_CLD_HOST_PORT,
	SEC_PXY_HOST_URL,
	SEC_PXY_HOST_PORT,
	SEC_PROV_STATE
};

/*
 * This is a flag indicating
 * a decommissioned/reprovisioned
 * is pending prior to warm reset.
 */
enum reprov_pend_flag {
	SEC_REPROV_NONE,
	SEC_REPROV_PEND,
	SEC_DECOMM_PEND
};

#endif /* PSE_OOB_SEC_ENUM_H */
