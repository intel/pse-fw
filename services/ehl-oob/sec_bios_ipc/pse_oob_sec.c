/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pse_oob_sec_base.h"
#include "pse_oob_sec_internal.h"
#include "pse_oob_sec_status_code.h"
#include "pse_oob_sec_enum.h"
#include "pse_oob_sec.h"

SEC_RTN unsigned int sec_start(
	SEC_IN enum sec_mode mode,
	SEC_IN unsigned int timeout
	)
{
	return sec_int_start(mode, timeout);
}

SEC_RTN unsigned int sec_get_root_ca(
	SEC_INOUT void *p_sec_data,
	SEC_INOUT unsigned int *p_sec_len
	)
{

	return sec_int_get_root_ca(p_sec_data, p_sec_len);
};

SEC_RTN unsigned int sec_get_dev_id(
	SEC_INOUT void *p_sec_data,
	SEC_INOUT unsigned int *p_sec_len
	)
{

	return sec_int_get_dev_id(p_sec_data, p_sec_len);
};

SEC_RTN unsigned int sec_get_mqtt_client_id(
	SEC_INOUT void *p_sec_data,
	SEC_INOUT unsigned int *p_sec_len
	)
{

	return sec_int_get_mqtt_client_id(p_sec_data, p_sec_len);
};

SEC_RTN unsigned int sec_get_tok_id(
	SEC_INOUT void *p_sec_data,
	SEC_INOUT unsigned int *p_sec_len
	)
{

	return sec_int_get_tok_id(p_sec_data, p_sec_len);
};

SEC_RTN unsigned int sec_get_cld_adapter(
	SEC_INOUT void *p_sec_data,
	SEC_INOUT unsigned int *p_sec_len
	)
{

	return sec_int_get_cld_adapter(p_sec_data, p_sec_len);
};

SEC_RTN unsigned int sec_get_cld_host_url(
	SEC_INOUT void *p_sec_data,
	SEC_INOUT unsigned int *p_sec_len
	)
{

	return sec_int_get_cld_host_url(p_sec_data, p_sec_len);
};

SEC_RTN unsigned int sec_get_cld_host_port(
	SEC_OUT unsigned int *p_sec_data
	)
{

	return sec_int_get_cld_host_port(p_sec_data);
};

SEC_RTN unsigned int sec_get_pxy_host_url(
	SEC_INOUT void *p_sec_data,
	SEC_INOUT unsigned int *p_sec_len
	)
{

	return sec_int_get_pxy_host_url(p_sec_data, p_sec_len);
};

SEC_RTN unsigned int sec_get_pxy_host_port(
	SEC_OUT unsigned int *p_sec_data
	)
{

	return sec_int_get_pxy_host_port(p_sec_data);
};

SEC_RTN unsigned int sec_set_reprovision(
	SEC_IN void *p_sec_data,
	SEC_IN unsigned int sec_len,
	SEC_IN enum sec_info_type type
	)
{
	return sec_int_set_reprovision(p_sec_data, sec_len, type);
}

SEC_RTN unsigned int sec_set_prov_state(
	SEC_IN unsigned int *p_sec_data)
{
	return sec_int_set_prov_state(p_sec_data);
}
SEC_RTN unsigned int sec_get_prov_state(
	SEC_OUT unsigned int *p_sec_data)
{
	return sec_int_get_prov_state(p_sec_data);
}

SEC_RTN unsigned int sec_get_context(
	SEC_OUT struct sec_context *p_sec_data
	)
{
	return sec_int_get_context(p_sec_data);
};

SEC_RTN unsigned int sec_get_nonce(
	SEC_INOUT unsigned int *p_nonce_data,
	SEC_IN unsigned int tran_id
	)
{
	return sec_int_get_nonce(p_nonce_data, tran_id);
}

SEC_RTN unsigned int sec_check_nonce(
	SEC_IN unsigned char *p_encrypted_nonce_data,
	SEC_IN unsigned int encrypted_nonce_data_len,
	SEC_IN unsigned char *p_encrypted_nonce_iv,
	SEC_IN unsigned int encrypted_nonce_iv_len,
	SEC_IN unsigned char *p_encrypted_nonce_tag,
	SEC_IN unsigned int encrypted_nonce_tag_len,
	SEC_IN unsigned int tran_id
	)
{
	return sec_int_check_nonce(p_encrypted_nonce_data,
				   encrypted_nonce_data_len,
				   p_encrypted_nonce_iv,
				   encrypted_nonce_iv_len,
				   p_encrypted_nonce_tag,
				   encrypted_nonce_tag_len,
				   tran_id);
}

SEC_RTN unsigned int sec_get_rand_num(
	SEC_INOUT unsigned int *p_rand_num_data,
	SEC_IN unsigned int size_in_bit
	)
{
	return sec_int_get_rand_num(p_rand_num_data, size_in_bit);
}
