/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * \file credentials.c
 *
 * \brief This file contains implementations for communicating with
 * sec_bios_ipc module to retrieve credentials like root_ca, username, password
 * etc.
 *
 * \see credentials.h and utils.h
 */
#include "credentials.h"
#include <adapter/telit.h>
#include <adapter/thingsboard.h>
#include <adapter/azure_iot.h>
#include <logging/log.h>
#include <stdio.h>
#include <string.h>

LOG_MODULE_REGISTER(OOB_CREDENTIALS, CONFIG_OOB_LOGGING);

#if defined(CONFIG_SOC_INTEL_PSE)
#if defined(CONFIG_OOB_BIOS_IPC)
#include <sec_bios_ipc/pse_oob_sec.h>
#endif
#endif

#if !defined(CONFIG_OOB_BIOS_IPC) && defined(CONFIG_MQTT_LIB_TLS)
VAR_DEFINER unsigned char telit_ca_certificate[] =
	"-----BEGIN CERTIFICATE-----\r\n"
	"MIIDXzCCAkegAwIBAgILBAAAAAABIVhTCKIwDQYJKoZIhvcNAQELBQAwTDEgMB4G\r\n"
	"A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjMxEzARBgNVBAoTCkdsb2JhbFNp\r\n"
	"Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDkwMzE4MTAwMDAwWhcNMjkwMzE4\r\n"
	"MTAwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMzETMBEG\r\n"
	"A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\r\n"
	"hvcNAQEBBQADggEPADCCAQoCggEBAMwldpB5BngiFvXAg7aEyiie/QV2EcWtiHL8\r\n"
	"RgJDx7KKnQRfJMsuS+FggkbhUqsMgUdwbN1k0ev1LKMPgj0MK66X17YUhhB5uzsT\r\n"
	"gHeMCOFJ0mpiLx9e+pZo34knlTifBtc+ycsmWQ1z3rDI6SYOgxXG71uL0gRgykmm\r\n"
	"KPZpO/bLyCiR5Z2KYVc3rHQU3HTgOu5yLy6c+9C7v/U9AOEGM+iCK65TpjoWc4zd\r\n"
	"QQ4gOsC0p6Hpsk+QLjJg6VfLuQSSaGjlOCZgdbKfd/+RFO+uIEn8rUAVSNECMWEZ\r\n"
	"XriX7613t2Saer9fwRPvm2L7DWzgVGkWqQPabumDk3F2xmmFghcCAwEAAaNCMEAw\r\n"
	"DgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0OBBYEFI/wS3+o\r\n"
	"LkUkrk1Q+mOai97i3Ru8MA0GCSqGSIb3DQEBCwUAA4IBAQBLQNvAUKr+yAzv95ZU\r\n"
	"RUm7lgAJQayzE4aGKAczymvmdLm6AC2upArT9fHxD4q/c2dKg8dEe3jgr25sbwMp\r\n"
	"jjM5RcOO5LlXbKr8EpbsU8Yt5CRsuZRj+9xTaGdWPoO4zzUhw8lo/s7awlOqzJCK\r\n"
	"6fBdRoyV3XpYKBovHd7NADdBj+1EbddTKJd+82cEHhXXipa0095MJ6RMG3NzdvQX\r\n"
	"mcIfeg7jLQitChws/zyrVQ4PkX4268NXSb7hLi18YIvDQVETI53O9zJrlAGomecs\r\n"
	"Mx86OyXShkDOOyyGeMlhLxS67ttVb9+E7gUJTb0o2HLO02JQZR7rkpeDMdmztcpH\r\n"
	"WD9f\r\n"
	"-----END CERTIFICATE-----\r\n";

VAR_DEFINER unsigned char tb_ca_certificate[] =
	"-----BEGIN CERTIFICATE-----\n"
	"MIIDiTCCAnGgAwIBAgIEH+ozvzANBgkqhkiG9w0BAQsFADB1MQswCQYDVQQGEwJN\r\n"
	"WTEMMAoGA1UECBMDUE5HMQswCQYDVQQHEwJCTDEUMBIGA1UEChMLVGhpbmdzYm9h\r\n"
	"cmQxFDASBgNVBAsTC1RoaW5nc2JvYXJkMR8wHQYDVQQDExZwb3J0YWwudGhpbmdz\r\n"
	"Ym9hcmQuY29tMB4XDTIwMDMxMjAxNTkyMloXDTQ3MDcyODAxNTkyMlowdTELMAkG\r\n"
	"A1UEBhMCTVkxDDAKBgNVBAgTA1BORzELMAkGA1UEBxMCQkwxFDASBgNVBAoTC1Ro\r\n"
	"aW5nc2JvYXJkMRQwEgYDVQQLEwtUaGluZ3Nib2FyZDEfMB0GA1UEAxMWcG9ydGFs\r\n"
	"LnRoaW5nc2JvYXJkLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\r\n"
	"AI2REpJmzVdLNKMKUUfhI5CrAm+KpJySnbYPx1BK3uh20bvl7SOKEasTBqSw3/Iw\r\n"
	"jgmz+qz3erSru+JGhdrnWQ0fdikXmpzxKktUO+U4bTqWLnlMYR7BY/oFbKc6CFSg\r\n"
	"gW54gneS9sIPgvaQU8NqrJUTkJTK/ZqU1+M8hv65X1V3pw5wgnEQSxTqefnZJP1u\r\n"
	"A3Ob3zD/dIKRU1d+z5y+1D7j1nZAYEp8wU1b3uELzuSa5K6weTFrOvIMp8buVhdB\r\n"
	"mzlB4mX59i+vdzE/6DioLHhAFbJqVGIM9+KV58OgvVeoTfr4w7U7kmWnKZsD+F7x\r\n"
	"PwPwKRTU2meyvHs+p/66tIECAwEAAaMhMB8wHQYDVR0OBBYEFEK1JJTUG7mf31cH\r\n"
	"qj/qZ7unSKe/MA0GCSqGSIb3DQEBCwUAA4IBAQCMjeZXL4ET/fHB6JA6CKEYab/i\r\n"
	"9S28bbhB+K4yT3gIUApzEEO7Cnaxg/JYXTgqpwBWgdrrAws+0Vuc5a3ViprmaJWX\r\n"
	"uMjEMSofm3mkUabk1U6z+jvjuahhE5339KZEzi44SbUNp2UTaMkVMvwE6zNmHLKv\r\n"
	"MKJqclDImTVHUTH/uKMGsjlOA1sTsc1OVzZ5Flzrf8I9E2KkQwW5iXoqDgSRJyOt\r\n"
	"vkmv+amtj9Cnw93GcI+8ynmzvKTu/ryEQscC5ScMQSpxU/8XsPln46p2gG6tYG/h\r\n"
	"YPd7DxHHBAPQAo4DZEAlwOF0icDgpFHT2EZ2mSOkyRZwkXQfBSitMbV+ZtCQ\n"
	"-----END CERTIFICATE-----\n";

VAR_DEFINER unsigned char azure_iot_ca_certificate[] =
	"-----BEGIN CERTIFICATE-----\r\n"
	"MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\r\n"
	"RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\r\n"
	"VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\r\n"
	"DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\r\n"
	"ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\r\n"
	"VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\r\n"
	"mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\r\n"
	"IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\r\n"
	"mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\r\n"
	"XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\r\n"
	"dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\r\n"
	"jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\r\n"
	"BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\r\n"
	"DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\r\n"
	"9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\r\n"
	"jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\r\n"
	"Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\r\n"
	"ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\r\n"
	"R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\r\n"
	"-----END CERTIFICATE-----\r\n";

#endif  /* end of CONFIG_MQTT_LIB_TLS */

/**
 * Function used to extract the adapter sent by oob_sec which is
 * a string for now. Called by populate_credentials
 *
 * @param [in] cloud_type char *
 * @retval enum messages NOADAPTER if adapter could not be extracted
 * @retval enum representation of adapter if adapter could be extracted
 *
 **/
enum cloud_adapter extract_adapter(char *cloud_type)
{
	if (!strncmp(cloud_type, "Telit", strlen(cloud_type))) {
		return TELIT;
	}
	if (!strncmp(cloud_type, "ThingsBoard", strlen(cloud_type))) {
		return THINGSBOARD;
	}
	if (!strncmp(cloud_type, "Azure", strlen(cloud_type))) {
		return AZURE;
	}
	LOG_INF("Failed to extract adapter");
	return NOADAPTER;
}

/**
 * Function used to populate the credentials sent from ehl_oob
 *
 * \note When HECI and bios modules are not in place, we need to
 *  stub it out.
 * @param [in] cloud_type char *
 * @retval no-return value
 *
 **/
#if !defined(CONFIG_OOB_BIOS_IPC)
void stubbed_credentials(char *cloud_type)
{
	/* Set all the credentials for the respective cloud adapter
	 * in the ehl-oob/Kconfig.
	 * No values will be set in this file.
	 **/

	/* TELIT cloud adapter taken as default, when BIOS IPC not used */
	enum cloud_adapter cloud = TELIT;

	if (cloud == TELIT) {
		cloud_type = "Telit";
		strncpy(creds->cloud_host,
			CONFIG_OOB_TELIT_CLD_HOST, MAX_ARR_LEN);
		strncpy(creds->cloud_port,
			CONFIG_OOB_CLD_PORT_SECURE, MAX_ARR_LEN);
		creds->token_size = strlen(CONFIG_OOB_TELIT_TOKEN_ID);
		strncpy(creds->token,
			CONFIG_OOB_TELIT_TOKEN_ID, creds->token_size);
		creds->username_size = strlen(CONFIG_OOB_TELIT_USERNAME);
		strncpy(creds->username,
			CONFIG_OOB_TELIT_USERNAME, creds->username_size);
		creds->mqtt_client_id_size = strlen(CONFIG_OOB_CLD_MQTT_CLIENT_ID);
		strncpy(creds->mqtt_client_id,
			CONFIG_OOB_CLD_MQTT_CLIENT_ID, creds->mqtt_client_id_size);
#if defined(CONFIG_MQTT_LIB_TLS)
		creds->trusted_ca_size = sizeof(telit_ca_certificate);
		strncpy(creds->trusted_ca,
			telit_ca_certificate, creds->trusted_ca_size);
		strncpy(creds->cloud_tls_sni,
			TELIT_TLS_SNI_HOSTNAME, MAX_ARR_LEN);
#endif
	} else if (cloud == THINGSBOARD) {
		cloud_type = "ThingsBoard";
		strncpy(creds->cloud_host,
			CONFIG_OOB_THINGSBOARD_CLD_HOST, MAX_ARR_LEN);
		strncpy(creds->cloud_port,
			CONFIG_OOB_CLD_PORT_SECURE, MAX_ARR_LEN);
		creds->token_size = strlen(CONFIG_OOB_THINGSBOARD_TOKEN_ID);
		strncpy(creds->token,
			CONFIG_OOB_THINGSBOARD_TOKEN_ID, creds->token_size);
		creds->username_size = strlen(CONFIG_OOB_THINGSBOARD_USERNAME);
		strncpy(creds->username,
			CONFIG_OOB_THINGSBOARD_USERNAME, creds->username_size);
		creds->mqtt_client_id_size = strlen(CONFIG_OOB_CLD_MQTT_CLIENT_ID);
		strncpy(creds->mqtt_client_id,
			CONFIG_OOB_CLD_MQTT_CLIENT_ID, creds->mqtt_client_id_size);
#if defined(CONFIG_MQTT_LIB_TLS)
		creds->trusted_ca_size = sizeof(tb_ca_certificate);
		strncpy(creds->trusted_ca,
			tb_ca_certificate, creds->trusted_ca_size);
		strncpy(creds->cloud_tls_sni,
			THINGSBOARD_TLS_SNI_HOSTNAME, MAX_ARR_LEN);
#endif
	} else if (cloud == AZURE) {
		cloud_type = "Azure";
		strncpy(creds->cloud_host,
			CONFIG_OOB_AZURE_IOT_CLD_HOST, MAX_ARR_LEN);
		strncpy(creds->cloud_port,
			CONFIG_OOB_CLD_PORT_SECURE, MAX_ARR_LEN);
		creds->token_size = strlen(CONFIG_OOB_AZURE_IOT_TOKEN_ID);
		strncpy(creds->token,
			CONFIG_OOB_AZURE_IOT_TOKEN_ID, creds->token_size);
		creds->username_size = strlen(CONFIG_OOB_AZURE_IOT_USERNAME);
		strncpy(creds->username,
			CONFIG_OOB_AZURE_IOT_USERNAME, creds->username_size);
		creds->mqtt_client_id_size = strlen(CONFIG_OOB_CLD_MQTT_CLIENT_ID);
		strncpy(creds->mqtt_client_id,
			CONFIG_OOB_CLD_MQTT_CLIENT_ID, creds->mqtt_client_id_size);
#if defined(CONFIG_MQTT_LIB_TLS)
		creds->trusted_ca_size = sizeof(azure_iot_ca_certificate);
		strncpy(creds->trusted_ca,
			azure_iot_ca_certificate, creds->trusted_ca_size);
		strncpy(creds->cloud_tls_sni,
			AZURE_IOT_TLS_SNI_HOSTNAME, MAX_ARR_LEN);
#endif
	}

	strncpy(creds->proxy_url, CONFIG_OOB_PROXY_URL, MAX_ARR_LEN);
	strncpy(creds->proxy_port, CONFIG_OOB_PROXY_PORT, MAX_ARR_LEN);
	creds->cloud_adapter = extract_adapter(cloud_type);

}
#endif

/*
 * Function: Set provision States for
 *           Sec context static region
 *
 * return:   Success/Fail
 */
int oob_set_provision_state(unsigned int oob_prov_state)
{
	LOG_INF("Set OOB State: %d\n", oob_prov_state);
#if defined(CONFIG_OOB_BIOS_IPC)
	return sec_set_prov_state((unsigned int *) &oob_prov_state);
#endif
	return OOB_SUCCESS;
}

/*
 * Function: API to get OOB states
 *
 * return:   True for prov state is decommission
 *           False for other states
 */
bool oob_get_provision_state(void)
{
	unsigned int oob_state;

#if defined(CONFIG_OOB_BIOS_IPC)
	sec_get_prov_state(&oob_state);
	LOG_INF("OOB State Is: %d\n", oob_state);
	return (oob_state == SEC_DECOM ? true : false);
#endif
	return false;
}

/*
 * Function: Release memory
 *           populated credentials allocated memory
 *           get release
 *
 * return:   void
 */

void oob_release_credentials(void)
{
	LOG_INF("Release credential data\n");
	memset((void *)creds->cloud_type, 0x00, MAX_ARR_LEN);
	memset((void *)creds->token, 0x00, MAX_ID_LEN);
	memset((void *)creds->username, 0x00, MAX_ID_LEN);
	memset((void *)creds->mqtt_client_id, 0x00, MAX_ID_LEN);
	memset((void *)creds->cloud_host, 0x00, MAX_ARR_LEN);
	memset((void *)creds->cloud_port, 0x00, MAX_ARR_LEN);
	memset((void *)creds->proxy_url, 0x00, MAX_ARR_LEN);
	memset((void *)creds->proxy_port, 0x00, MAX_ARR_LEN);
	memset((void *)creds->trusted_ca, 0x00, MAX_CERT_LEN);
#if defined(CONFIG_MQTT_LIB_TLS)
	memset((void *)creds->cloud_tls_sni, 0x00, MAX_ARR_LEN);
#endif
}

/**
 * Function used to populate the credentials sent from ehl_oob
 * It allocates buffers for each piece of information on the heap and expects
 * pse_oob_sec_<field_specific> to populate it.
 *
 * \note Since th HECI and bios modules are not in place, we need to currently
 * stub it out
 *
 * @retval nonzero on failure and 0 on success
 *
 **/
int populate_credentials(void)
{

#if !defined(CONFIG_OOB_BIOS_IPC)
	LOG_WRN("Executing without BIOS_IPC");
	char *cloud_type = "";

	stubbed_credentials(cloud_type);
#else
	char buffer[SEC_URL_LEN * 2];
	int size;
	int port;
	int fetch_result = OOB_ERR_FETCH_ERROR;

	LOG_DBG("Fetching credentials through BIOS IPC");

	/* Cloud Adapter Fetching */
	memset(buffer, '\0', SEC_URL_LEN);
	fetch_result = sec_get_cld_adapter(buffer, &size);
	if (fetch_result) {
		LOG_ERR("Failed to fetch cloud_adapter");
		return OOB_ERR_FETCH_CLOUD_ADAPTER;
	}
	memset((char *)creds->cloud_type, '\0', size + 1);
	memcpy((char *)creds->cloud_type, buffer, size);
	LOG_DBG("Cloud type from BIOS: %s", creds->cloud_type);
	creds->cloud_adapter = extract_adapter((char *)creds->cloud_type);
	LOG_DBG("cloud_adapter: %d size: %d", creds->cloud_adapter,
		strlen(creds->cloud_type));

	/* Token-id/Password Fetching */
	memset(buffer, '\0', SEC_URL_LEN);
	fetch_result = sec_get_tok_id(buffer, &size);
	if (fetch_result) {
		LOG_ERR("Failed to fetch token_id");
		return OOB_ERR_FETCH_CLOUD_PASSWORD;
	}
	memset((char *)creds->token, '\0', size + 1);
	memcpy((char *)creds->token, buffer, size);
	creds->token_size = size;
	LOG_DBG("token: %s size: %d", creds->token, creds->token_size);

	/* Device-id/Username Fetching */
	memset(buffer, '\0', SEC_URL_LEN);
	fetch_result = sec_get_dev_id(buffer, &size);
	if (fetch_result) {
		LOG_ERR("Failed to fetch device_id");
		return OOB_ERR_FETCH_CLOUD_USERNAME;
	}
	memset((char *)creds->username, '\0', size + 1);
	memcpy((char *)creds->username, buffer, size);
	creds->username_size = size;
	LOG_DBG("username: %s size: %d", creds->username,
		creds->username_size);

	/* MQTT Client-id Fetching */
	memset(buffer, '\0', SEC_URL_LEN);
	fetch_result = sec_get_mqtt_client_id(buffer, &size);
	if (fetch_result) {
		LOG_ERR("Failed to fetch mqtt client id");
		return OOB_ERR_FETCH_CLOUD_MQTT_CLIENT_ID;
	}
	memset((char *)creds->mqtt_client_id, '\0', size + 1);
	memcpy((char *)creds->mqtt_client_id, buffer, size);
	creds->mqtt_client_id_size = size;
	LOG_DBG("mqtt client id : %s size: %d", creds->mqtt_client_id,
		creds->mqtt_client_id_size);

	/* Cloud host Fetching */
	memset(buffer, '\0', SEC_URL_LEN);
	fetch_result = sec_get_cld_host_url(buffer, &size);
	if (fetch_result) {
		LOG_ERR("Failed to fetch cld_host_url");
		return OOB_ERR_FETCH_CLOUD_HOST_URL;
	}
	memset((char *)creds->cloud_host, '\0', size + 1);
	memcpy((char *)creds->cloud_host, buffer, size);
	LOG_DBG("cloud host url: %s size: %d", creds->cloud_host,
		strlen(creds->cloud_host));

	/* Cloud host port Fetching */
	port = 0;
	memset(buffer, '\0', SEC_URL_LEN);
	fetch_result = sec_get_cld_host_port(&port);
	if (fetch_result) {
		LOG_ERR("Failed to fetch cld_host_port");
		return OOB_ERR_FETCH_CLOUD_HOST_PORT;
	}
	sprintf(buffer, "%d", port);
	size = strlen(buffer);
	memset((char *)creds->cloud_port, '\0', size + 1);
	memcpy((char *)creds->cloud_port, buffer, size);
	LOG_DBG("cloud port :%s size: %d", creds->cloud_port,
		strlen(creds->cloud_port));

	/* Proxy host URL Fetching */
	memset(buffer, '\0', SEC_URL_LEN);
	fetch_result = sec_get_pxy_host_url(buffer, &size);
	if (fetch_result) {
		LOG_ERR("Failed to fetch pxy_host_url");
		return OOB_ERR_FETCH_PROXY_HOST_URL;
	}
	memset((char *)creds->proxy_url, '\0', size + 1);
	memcpy((char *)creds->proxy_url, buffer, size);
	LOG_DBG("proxy url :%s size: %d", creds->proxy_url,
		strlen(creds->proxy_url));

	/* Proxy host port Fetching */
	port = 0;
	memset(buffer, '\0', SEC_URL_LEN);
	sec_get_pxy_host_port(&port);
	if (fetch_result) {
		LOG_ERR("Failed to fetch pxy_host_port");
		return OOB_ERR_FETCH_PROXY_HOST_PORT;
	}
	sprintf(buffer, "%d", port);
	size = strlen(buffer);
	memset((char *)creds->proxy_port, '\0', size + 1);
	memcpy((char *)creds->proxy_port, buffer, size);
	LOG_DBG("proxy port :%s size: %d", creds->proxy_port,
		strlen(creds->proxy_port));

#if defined CONFIG_MQTT_LIB_TLS
	memset(buffer, '\0', SEC_URL_LEN * 2);
	fetch_result = sec_get_root_ca(buffer, &size);
	if (fetch_result) {
		LOG_ERR("Failed to fetch root_ca");
		return OOB_ERR_FETCH_ROOT_CA;
	}

	/* SNI as per cloud type, for now it's TELIT */
	if (creds->cloud_adapter == TELIT) {
		strncpy((char *)creds->cloud_tls_sni,
			TELIT_TLS_SNI_HOSTNAME, MAX_ARR_LEN);
	} else if (creds->cloud_adapter == THINGSBOARD) {
		strncpy((char *)creds->cloud_tls_sni,
			THINGSBOARD_TLS_SNI_HOSTNAME, MAX_ARR_LEN);
	} else if (creds->cloud_adapter == AZURE) {
		strncpy((char *)creds->cloud_tls_sni,
			AZURE_IOT_TLS_SNI_HOSTNAME, MAX_ARR_LEN);
	}

	creds->trusted_ca_size = size + 1;
	memset((char *)creds->trusted_ca, '\0', creds->trusted_ca_size);
	memcpy(creds->trusted_ca, buffer, creds->trusted_ca_size);
	LOG_DBG("creds->trusted_ca: %s size: %d", creds->trusted_ca,
		creds->trusted_ca_size);
#endif  /* CONFIG_MQTT_LIB_TLS */
#endif  /* !CONFIG_OOB_BIOS_IPC */

	return OOB_SUCCESS;
}
