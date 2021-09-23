/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifndef PSE_OOB_SEC_H
#define PSE_OOB_SEC_H

#include "pse_oob_sec_base.h"
#include "pse_oob_sec_internal.h"
#include "pse_oob_sec_enum.h"

/*
 * To start up the oob security module.
 * This function will wait for the HECI interface
 * to get ready before proceed with any initialization steps.
 *
 * return:
 *
 * SEC_FAILED_OOB_DISABLED - OOB is disabled
 * SEC_SUCCESS - OOB enabling is successfully
 * SEC_FAILED - OOB enabling is failed
 */
SEC_RTN unsigned int sec_start(
	SEC_IN enum sec_mode mode,
	/* SEC_NORMAL:
	 * The oob security module will start up and wait for BIOS
	 * handshake to initialize the content of the AONRF.
	 *
	 * It supports initial provisioning, re-provisioning /
	 * decommissioning and normal post-provisioning flow.
	 * This function call return immediately after the AONRF is
	 * initialized properly after initial provisioning or normal
	 * post-provisioning flow complete successfully.
	 *
	 * The PSE is expected to call sec_start() again on BIOS reboot.
	 *
	 * SEC_DAEMON:
	 * (Not the default mode, backup for potential Zephyr optimization).
	 * The oob security module will start up and wait for
	 * BIOS handshake to initialize the content of the AONRF.
	 * This function call will continue monitoring new IPC from BIOS even
	 * after AONRF is successfully initialized until sec_stop()
	 * is called.
	 */
	SEC_IN unsigned int timeout
	/*
	 * Timeout value milliseconds
	 * only applicable for is_service=SEC_NORMAL.
	 */
	);

/*
 * (Note the default mode, backup for
 * potential Zephyr optimization)
 * To check the oob security module status in SEC_DAEMON mode.
 *
 * return:
 *
 * SEC_STATUS_OOB_NOT_STARTED - OOB is not started
 * SEC_STATUS_OOB_STARTING - OOB is starting up
 * SEC_STATUS_OOB_READY - OOB is ready
 * SEC_FAILED - OOB enabling is failed
 */
SEC_RTN unsigned int sec_get_status(void);

/*
 * (Not the default mode, backup for potential Zephyr optimization)
 * To stop the oob security module running in SEC_DAEMON mode.
 *
 * return:
 *
 * SEC_FAILED_OOB_DISABLED - OOB stopping is
 * failed as OOB is disabled
 * SEC_STATUS_OOB_NOT_STARTED - OOB stopping is not
 * happening as OOB is not started
 * SEC_STATUS_OOB_STARTING - OOB stopping is not
 * happening as OOB is starting up
 * SEC_SUCCESS - OOB stopping is successfully
 * SEC_FAILED - OOB stopping is failed
 */
SEC_RTN unsigned int sec_stop(void);

/*
 * Get ROOT_CA for TLS session setup.
 *
 * return:
 *
 * SEC_FAILED_OOB_BUFFER_OVERFLOW -
 * The ROOT_CA size is larger than the provided buffer
 * SEC_FAILED_OOB_DISABLED - ROOT_CA is failed
 * to return as OOB is disabled
 * SEC_STATUS_OOB_NOT_STARTED - ROOT_CA is failed to
 * return as OOB is not started
 * SEC_STATUS_OOB_STARTING - ROOT_CA is failed to
 * return as OOB is starting up
 * SEC_SUCCESS - ROOT_CA is returned successfully
 * SEC_FAILED - ROOT_CA is failed to return
 */
SEC_RTN unsigned int sec_get_root_ca(
	SEC_INOUT void *p_sec_data,
	/* Pre-allocated buffer from the calling function.
	 * The ROOT_CA will be copied to this buffer.
	 */
	SEC_INOUT unsigned int *p_sec_len
	/* Size of pre-allocated buffer from the calling function.
	 * The ROOT_CA size will be updated here.
	 */
	);

/*
 *   Get DEVICE_ID for cloud login.
 *
 *   return:
 *   SEC_FAILED_OOB_BUFFER_OVERFLOW - The DEVICE_ID size is
 *   larger than the provided buffer
 *   SEC_FAILED_OOB_DISABLED - DEVICE_ID is failed
 *   to return as OOB is disabled
 *   SEC_STATUS_OOB_NOT_STARTED - DEVICE_ID is failed
 *   to return as OOB is not started
 *   SEC_STATUS_OOB_STARTING - DEVICE_ID is failed to
 *   return as OOB is starting up
 *   SEC_SUCCESS - DEVICE_ID is returned successfully
 *   SEC_FAILED - DEVICE_ID is failed to return
 */
SEC_RTN unsigned int sec_get_dev_id(
	SEC_INOUT void *p_sec_data,
	/* Pre-allocated buffer from the calling function.
	 * The DEVICE_ID will be copied to this buffer.
	 */
	SEC_INOUT unsigned int *p_sec_len
	/* Size of pre-allocated buffer from the calling function.
	 * The DEVICE_ID size will be updated here.
	 */
	);

/*
 *   Get MQTT_CLIENT_ID for cloud login.
 *
 *   return:
 *   SEC_FAILED_OOB_BUFFER_OVERFLOW - The MQTT_CLIENT_ID size is
 *   larger than the provided buffer
 *   SEC_FAILED_OOB_DISABLED - MQTT_CLIENT_ID is failed
 *   to return as OOB is disabled
 *   SEC_STATUS_OOB_NOT_STARTED - MQTT_CLIENT_ID is failed
 *   to return as OOB is not started
 *   SEC_STATUS_OOB_STARTING - MQTT_CLIENT_ID is failed to
 *   return as OOB is starting up
 *   SEC_SUCCESS - MQTT_CLIENT_ID is returned successfully
 *   SEC_FAILED - MQTT_CLIENT_ID is failed to return
 */
SEC_RTN unsigned int sec_get_mqtt_client_id(
	SEC_INOUT void *p_sec_data,
	/* Pre-allocated buffer to get MQTT_CLIENT_ID.
	 */
	SEC_INOUT unsigned int *p_sec_len
	/* Size of MQTT_CLIENT_ID.
	 */
	);

/*
 *   Get TOKEN_ID for cloud login.
 *
 *   return:
 *
 *       SEC_FAILED_OOB_BUFFER_OVERFLOW - The TOKEN_ID size is larger
 *       than the provided buffer
 *       SEC_FAILED_OOB_DISABLED - TOKEN_ID is failed to return as
 *       OOB is disabled
 *       SEC_STATUS_OOB_NOT_STARTED - TOKEN_ID is failed to
 *       return as OOB is not started
 *       SEC_STATUS_OOB_STARTING - TOKEN_ID is failed to
 *       return as OOB is starting up
 *       SEC_SUCCESS - TOKEN_ID is returned successfully
 *       SEC_FAILED - TOKEN_ID is failed to return
 */
SEC_RTN unsigned int sec_get_tok_id(
	SEC_INOUT void *p_sec_data,
	/* Pre-allocated buffer from the calling function.
	 * The TOKEN_ID will be copied to this buffer.
	 */
	SEC_INOUT unsigned int *p_sec_len
	/* Size of pre-allocated buffer from the calling function.
	 * The TOKEN_ID size will be updated here.
	 */
	);

/*
 *   Get CLD_ADAPTER for cloud login.
 *
 *   return:
 *
 *       SEC_FAILED_OOB_BUFFER_OVERFLOW - The CLD_ADAPTER size is larger
 *       than the provided buffer
 *       SEC_FAILED_OOB_DISABLED - CLD_ADAPTER is failed to return as
 *       OOB is disabled
 *       SEC_STATUS_OOB_NOT_STARTED - CLD_ADAPTER is failed to
 *       return as OOB is not started
 *       SEC_STATUS_OOB_STARTING - CLD_ADAPTER is failed to
 *       return as OOB is starting up
 *       SEC_SUCCESS - CLD_ADAPTER is returned successfully
 *       SEC_FAILED - CLD_ADAPTER is failed to return
 */
SEC_RTN unsigned int sec_get_cld_adapter(
	SEC_INOUT void *p_sec_data,
	/* Pre-allocated buffer from the calling function.
	 * The CLD_ADAPTER will be copied to this buffer.
	 */
	SEC_INOUT unsigned int *p_sec_len
	/* Size of pre-allocated buffer from the calling function.
	 * The CLD_ADAPTER size will be updated here.
	 */
	);

/*
 *   Get CLD_HOST_URL for cloud login.
 *
 *   return:
 *
 *       SEC_FAILED_OOB_BUFFER_OVERFLOW - The CLD_HOST_URL size is
 *       larger than the provided buffer
 *       SEC_FAILED_OOB_DISABLED - CLD_HOST_URL is failed to
 *       return as OOB is disabled
 *       SEC_STATUS_OOB_NOT_STARTED - CLD_HOST_URL is failed to
 *       return as OOB is not started
 *       SEC_STATUS_OOB_STARTING - CLD_HOST_URL is failed to
 *       return as OOB is starting up
 *       SEC_SUCCESS - CLD_HOST_URL is returned successfully
 *       SEC_FAILED - CLD_HOST_URL is failed to return
 */
SEC_RTN unsigned int sec_get_cld_host_url(
	SEC_INOUT void *p_sec_data,
	/* Pre-allocated buffer from the calling function.
	 * The CLD_HOST_URL will be copied to this buffer.
	 */
	SEC_INOUT unsigned int *p_sec_len
	/* Size of pre-allocated buffer from the calling function.
	 * The CLD_HOST_URL size will be updated here.
	 */
	);

/*
 *   Get CLD_HOST_PORT for cloud login.
 *
 *   return:
 *
 * SEC_FAILED_OOB_BUFFER_OVERFLOW - The CLD_HOST_PORT size is larger
 * than the provided buffer
 * SEC_FAILED_OOB_DISABLED - CLD_HOST_PORT is failed to return
 * as OOB is disabled
 * SEC_STATUS_OOB_NOT_STARTED - CLD_HOST_PORT is failed to return
 * as OOB is not started
 * SEC_STATUS_OOB_STARTING - CLD_HOST_PORT is failed to return
 * as OOB is starting up
 * SEC_SUCCESS - CLD_HOST_PORT is returned successfully
 * SEC_FAILED - CLD_HOST_PORT is failed to return
 */
SEC_RTN unsigned int sec_get_cld_host_port(
	SEC_OUT unsigned int *p_sec_data
	/* Pre-allocated buffer from the calling function.
	 * The CLD_HOST_PORT will be copied to this buffer.
	 */
	);

/*
 *   Get PXY_HOST_URL for cloud login.
 *
 *   return:
 *
 * SEC_FAILED_OOB_BUFFER_OVERFLOW - The PXY_HOST_URL size is
 * larger than the provided buffer
 * SEC_FAILED_OOB_DISABLED - PXY_HOST_URL is failed
 * to return as OOB is disabled
 * SEC_STATUS_OOB_NOT_STARTED - PXY_HOST_URL is failed to
 * return as OOB is not started
 * SEC_STATUS_OOB_STARTING - PXY_HOST_URL is failed to
 * return as OOB is starting up
 * SEC_SUCCESS - PXY_HOST_URL is returned successfully
 * SEC_FAILED - PXY_HOST_URL is failed to return
 */
SEC_RTN unsigned int sec_get_pxy_host_url(
	SEC_INOUT void *p_sec_data,
	/* Pre-allocated buffer from the calling function.
	 * The PXY_HOST_URL will be copied to this buffer.
	 */
	SEC_INOUT unsigned int *p_sec_len
	/* Size of pre-allocated buffer from the calling function.
	 * The PXY_HOST_URL size will be updated here.
	 */
	);

/*
 * Get PXY_HOST_PORT for cloud login.
 *
 * return:
 * SEC_FAILED_OOB_BUFFER_OVERFLOW - The PXY_HOST_PORT size is
 * larger than the provided buffer
 * SEC_FAILED_OOB_DISABLED - PXY_HOST_PORT is failed
 * to return as OOB is disabled
 * SEC_STATUS_OOB_NOT_STARTED - PXY_HOST_PORT is failed
 * to return as OOB is not started
 * SEC_STATUS_OOB_STARTING - PXY_HOST_PORT is failed to
 * return as OOB is starting up
 *
 * SEC_SUCCESS - PXY_HOST_PORT is returned successfully
 * SEC_FAILED - PXY_HOST_PORT is failed to return
 */
SEC_RTN unsigned int sec_get_pxy_host_port(
	SEC_OUT unsigned int *p_sec_data
	/* Pre-allocated buffer from the calling function.
	 * The PXY_HOST_PORT will be copied to this buffer.
	 */
	);

/*
 *   Update DEVICE_ID, TOKEN_ID, CLD_HOST_URL, CLD_HOST_PORT,
 *   PXY_HOST_URL, or PXY_HOST_PORT. More than one call to this
 *   function is expected if more than one value is updated.
 *
 *   Calling function should trigger host reset to reboot BIOS and
 *   call sec_start() as preparation for IPC handshake with BIOS.
 *
 *   return:
 *   SEC_FAILED_OOB_DISABLED - PXY_HOST_PORT is failed to return
 *   as OOB is disabled
 *   SEC_STATUS_OOB_NOT_STARTED - PXY_HOST_PORT is failed to
 *   return as OOB is not started
 *   SEC_STATUS_OOB_STARTING - PXY_HOST_PORT is failed to
 *   return as OOB is starting up
 *   SEC_SUCCESS - PXY_HOST_PORT is returned successfully
 *   SEC_FAILED - PXY_HOST_PORT is failed to return
 */
SEC_RTN unsigned int sec_set_reprovision(
	SEC_IN void *p_sec_data,
	/* Pre-allocated buffer from the calling function.
	 * The updated asset will be copied to this buffer.
	 */
	SEC_IN unsigned int sec_len,
	/* Size of pre-allocated buffer from the calling function.
	 * The updated asset size will be updated here.
	 */
	SEC_IN enum sec_info_type type
	/* Type of updated asset from the calling function.
	 * e.g. SEC_DEV_ID, SEC_TOK_ID,
	 * SEC_CLD_HOST_URL,
	 * SEC_CLD_HOST_PORT, SEC_PXY_HOST_URL,
	 * SEC_PXY_HOST_PORT,
	 * SEC_PROV_STATE
	 */
	);


/*
 *   Get full context from AEONRF.
 *   This additional function is created per request of the OOB Agent
 *   aimed for fast buffer copying of the full AEONRF content.
 *   The validation of the content is expected to be done at the
 *   calling function. It is recommended to use the individual
 *   AEONRF parameter get() function for validated access.
 *   Another note is the AEONRF internal physical structure mapping may
 *   changed per request and corresponding update may be needed in
 *   the calling function from OOB Agent. It is advice to use
 *   the individual param get() function for a proper abstraction and forward
 *   compatibility.
 *
 *   return:
 *   SEC_FAILED_OOB_BUFFER_OVERFLOW - The AEONRF context  size is
 *   larger than the provided buffer
 *   SEC_FAILED_OOB_DISABLED - AEONRF context is failed to return as
 *   OOB is disabled
 *   SEC_STATUS_OOB_NOT_STARTED - AEONRF context is failed to return
 *   as OOB is not started
 *   SEC_STATUS_OOB_STARTING - AEONRF context is failed to return as
 *   OOB is starting up
 *   SEC_SUCCESS - AEONRF context is returned successfully
 *   SEC_FAILED - AEONRF context  is failed to return
 */
SEC_RTN unsigned int sec_get_context(
	SEC_OUT struct sec_context *p_sec_data
	/* Pre-allocated buffer from the calling function.
	 * The updated asset will be copied to this buffer.
	 */
	);

/*
 *
 *   Request a nonce from oob security module.
 *
 *   return:
 *
 *   SEC_FAILED_OOB_DISABLED - PXY_HOST_PORT is failed to return
 *   as OOB is disabled
 *   SEC_STATUS_OOB_NOT_STARTED - PXY_HOST_PORT is failed to
 *   return as OOB is not started
 *   SEC_STATUS_OOB_STARTING - PXY_HOST_PORT is failed to
 *   return as OOB is starting up
 *   SEC_SUCCESS - PXY_HOST_PORT is returned successfully
 *   SEC_FAILED - PXY_HOST_PORT is failed to return
 */
SEC_RTN unsigned int sec_get_nonce(
	SEC_INOUT unsigned int *p_nonce_data,
	/* Pre-allocated buffer from the calling function.
	 * The nonce will be copied to this buffer.
	 */
	SEC_IN unsigned int tran_id
	/* Transaction ID for the nonce challenge received
	 * from cloud server.
	 */
	);

/*
 *
 *   To check a nonce with oob security module.
 *
 *   return:
 *
 *  SEC_FAILED_OOB_DISABLED - PXY_HOST_PORT is failed to return as
 *  OOB is disabled
 *  SEC_STATUS_OOB_NOT_STARTED - PXY_HOST_PORT is failed to return
 *  as OOB is not started
 *  SEC_STATUS_OOB_STARTING - PXY_HOST_PORT is failed to return as
 *  OOB is starting up
 *  SEC_SUCCESS - PXY_HOST_PORT is returned successfully
 *  SEC_FAILED - PXY_HOST_PORT is failed to return
 */
SEC_RTN unsigned int sec_check_nonce(
	SEC_IN unsigned char *p_encrypted_nonce_data,
	/* Pre-allocated buffer from the calling function
	 * contains the encrypted nonce.
	 */
	SEC_IN unsigned int encrypted_nonce_data_len,
	/* Length of the encrypted nonce.
	 */
	SEC_IN unsigned char *p_encrypted_nonce_iv,
	/* Pre-allocated buffer from the calling function contains
	 * the initialization vector of the encrypted nonce.
	 */
	SEC_IN unsigned int encrypted_nonce_iv_len,
	/* Length of the initialization vector of the encrypted nonce.
	 */
	SEC_IN unsigned char *p_encrypted_nonce_tag,
	/* Pre-allocated buffer from the calling function
	 * contains the tag of the encrypted nonce.
	 */
	SEC_IN unsigned int encrypted_nonce_tag_len,
	/* Length of the tag of the encrypted nonce.
	 */
	SEC_IN unsigned int tran_id
	/* Transaction ID for the nonce challenge received from
	 * cloud server.
	 */
	);


/*
 *
 *   Request a random number from oob security module for e.g. TLS setup.
 *
 *   return:
 *   SEC_FAILED_OOB_DISABLED - PXY_HOST_PORT is failed to return as
 *   OOB is disabled
 *   SEC_STATUS_OOB_NOT_STARTED - PXY_HOST_PORT is failed to return
 *   as OOB is not started
 *   SEC_STATUS_OOB_STARTING - PXY_HOST_PORT is failed to return
 *   as OOB is starting up
 *   SEC_SUCCESS - PXY_HOST_PORT is returned successfully
 *   SEC_FAILED - PXY_HOST_PORT is failed to return
 */
SEC_RTN unsigned int sec_get_rand_num(
	SEC_INOUT unsigned int *p_rand_num_data,
	/* Pre-allocated buffer from the calling function.
	 * The random number will be copied to this buffer.
	 */
	SEC_IN unsigned int size_in_bit
	/* Requested random number size in bit. Recommended to use
	 * 24-bits by default.
	 */
	);

/*
 *   Set OOB State:
 *   Provisioned,
 *   provision pending,
 *   Decommission,
 *   reprovision pending with update
 *
 *   return:
 *   SEC_SUCCESS - Successful setup of OOB status
 *   SEC_FAILED - Failed to set OOB status
 */
SEC_RTN unsigned int sec_set_prov_state(
	SEC_IN unsigned int *p_sec_data);

/*
 *   Get OOB State:
 *   Provisioned,
 *   provision pending,
 *   Decommission,
 *   reprovision pending with update
 *
 *   return:
 *   SEC_SUCCESS - OOB prov state is returned successfully
 *   SEC_FAILED - OOB prov state is failed to return
 */
SEC_RTN unsigned int sec_get_prov_state(
	SEC_OUT unsigned int *p_sec_data);

#endif /* PSE_OOB_SEC_H */
