/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * \file credentials.h
 *
 * \brief This contains declaration for all the structures and variables
 * needed by managability to establish connection with the cloud.
 *
 */

#ifndef _COMMON_CREDENTIALS_H_
#define _COMMON_CREDENTIALS_H_

#if defined(CONFIG_SOC_INTEL_PSE)
#include "pse_app_framework.h"
#elif defined(CONFIG_BOARD_SAM_E70_XPLAINED)
#include <kernel.h>
#endif

#include "utils.h"

#define MAX_ARR_LEN 64
#define MAX_ID_LEN 512
#define MAX_CERT_LEN 2048

/**
 * cloud_credentials structure
 *
 * @details structure variable to hold all credentials info. The fields are
 * self-explanatory and structure variable is initialized in ehl_oob by
 * calling function populate_credentials
 *
 * This is passed down to different parts of managability code
 * where it is derefernced.
 *
 */
struct cloud_credentials {
	const char cloud_type[MAX_ARR_LEN];
	const char cloud_host[MAX_ARR_LEN];
	const char cloud_port[MAX_ARR_LEN];
	const char token[MAX_ID_LEN];
	size_t token_size;
	const char username[MAX_ID_LEN];
	size_t username_size;
	const char mqtt_client_id[MAX_ID_LEN];
	size_t mqtt_client_id_size;
	const char proxy_url[MAX_ARR_LEN];
	const char proxy_port[MAX_ARR_LEN];
	unsigned char trusted_ca[MAX_CERT_LEN];
	size_t trusted_ca_size;
	enum cloud_adapter cloud_adapter;
#if defined(CONFIG_MQTT_LIB_TLS)
	const char cloud_tls_sni[MAX_ARR_LEN];
#endif
};

/**
 * Pointer to cloud_credentials structure type
 */
extern VAR_DEFINER_BSS struct cloud_credentials cred_var;
extern VAR_DEFINER_BSS struct cloud_credentials *creds;

/*
 * OOB thread commands states are handled through
 * managability fifo after processing each command OOB thread
 * was suspend on fifo, sec heci thread once not needed for PSE
 * this thread can send cancel request to oob thread using same fifo
 * to exit wait loop
 */
extern VAR_DEFINER_BSS struct k_fifo managability_fifo;

/**
 * Function used to interface with sec_bios_ipc and populate
 * fields from the bios into the creds
 *
 * @retval nonzero on failure and 0 on success
 **/
int oob_set_provision_state(unsigned int oob_prov_state);
bool oob_get_provision_state(void);
int populate_credentials(void);
void oob_release_credentials(void);
#endif
