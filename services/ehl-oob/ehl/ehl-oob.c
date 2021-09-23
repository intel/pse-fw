/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * \file ehl-oob.c
 *
 * \brief This contains the APP entry point, service entry point
 * for EHL OOB. The application interacts with bios, sets
 * cloud connection credentials, connects to cloud, publishes
 * telemetry and wait for cloud actions
 *
 */

#include "ehl-oob.h"
#include "adapter/adapter.h"
#include <common/credentials.h>
#include <common/pse_app_framework.h>
#include <common/utils.h>
#include <sec_bios_ipc/pse_oob_sec.h>
#include <logging/log.h>

#include <driver/sedi_driver_pm.h>
#include <driver/sedi_driver_common.h>

#ifdef CONFIG_MQTT_LIB_TLS
#include <mqtt_client/mqtt_client.h>
#endif

#include <net/socket.h>
#include <net/net_core.h>
#include <net/net_if.h>
#include <net/net_mgmt.h>

LOG_MODULE_REGISTER(OOB_EHL, CONFIG_OOB_LOGGING);

/* Max retry to acquire dhcp (40*300ms) */
#define DHCP_MAX_RETRY 40

/* Decommission timeout wait for user thread */
#define DECOMM_TIMEOUT 40

/* Network Management L2 (iface) events */
#define L2_EVENT_MASK (NET_EVENT_IF_DOWN | NET_EVENT_IF_UP)

/* OOB thread additional memory pool to perform PMC
 * oob_pool_name: Name of the memory pool.
 * MIN_SIZE_BLOCK: Size of the smallest blocks in the pool (in bytes).
 * MAX_SIZE_BLOCK: Size of the largest blocks in the pool (in bytes).
 * MAX_BLOCKS: Number of maximum sized blocks in the pool.
 * ALIGN_VALUE: Alignment of the pool’s buffer (power of 2).
 */
K_HEAP_DEFINE(oob_pool_name,
	      OOB_POOL_MAX_SIZE_BLOCK * OOB_POOL_MAX_BLOCKS);

VAR_DEFINER_BSS struct cloud_credentials cred_var;
VAR_DEFINER_BSS struct cloud_credentials *creds;
VAR_DEFINER_BSS enum pm_status pm_sx_st;
static VAR_DEFINER_BSS bool pmc_reboot, is_thread_termi_set;
static VAR_DEFINER_BSS struct fifo_message *data_item;
static VAR_DEFINER_BSS struct fifo_message sx_timer_q, sx_state_q,
					   tls_state_q, nw_mgmt_q;
VAR_DEFINER_BSS uint32_t nw_mgmt_event;

#if defined(CONFIG_SOC_INTEL_PSE)
static void oob_init_config(void);
static void ehl_oob_main_service(void *p1, void *p2, void *p3);

/* Declare OOB kernel objects */
static struct k_thread ehl_oob_service_task;
static struct k_thread ehl_oob_cfg_task;
static struct net_mgmt_event_callback l2_mgmt_cb;
static struct net_mgmt_event_callback iface_cb;
static struct net_if *default_iface;

static K_THREAD_STACK_DEFINE(ehl_oob_service_task_stack,
			     OOB_THREAD_STACK_SIZE);

static K_THREAD_STACK_DEFINE(ehl_oob_cfg_stack,
			     OOB_THREAD_STACK_SIZE);

static K_SEM_DEFINE(oob_cfg_sem, 0, 1);
static K_SEM_DEFINE(oob_conn_sem, 0, 1);

/** Set timer periodically */
K_TIMER_DEFINE(oob_tls_session_timer, oob_tls_session_expiry, NULL);
K_TIMER_DEFINE(oob_power_trans_timer, oob_power_trans_expiry, NULL);
K_TIMER_DEFINE(oob_sx_trans_timer, oob_sx_trans_expiry, NULL);

extern struct k_timer ping_req_timer;
#if defined(CONFIG_OOB_BIOS_IPC)
extern struct k_mutex sec_ctx_mutex;
extern struct k_sem sec_hc_oob_sem;
#endif

#if defined(CONFIG_USERSPACE)
extern struct k_mem_domain dom2;
extern struct k_mem_partition k_mbedtls_partition;
extern struct k_mem_partition z_malloc_partition;
#endif

/* Add the OOB kernel object pointer into list. */
static const void *obj_list[] = {
	&managability_fifo,
	&ehl_oob_service_task,
	&ehl_oob_service_task_stack,
	&ping_req_timer,
	&oob_tls_session_timer,
	&oob_power_trans_timer,
	&oob_sx_trans_timer,
	&l2_mgmt_cb,
	&iface_cb,
#if defined(CONFIG_OOB_BIOS_IPC)
	&sec_ctx_mutex,
	&sec_hc_oob_sem,
#endif
	&oob_conn_sem,
	&oob_cfg_sem
};

#define K_OBJS ARRAY_SIZE(obj_list)
#define OOB_CFG_THREAD_PRIO 9

/**
 * Define a service which will execute immediately
 * after post kernel init but before Apps main.
 *
 * OOB is expected to run as APP_ID 2
 *
 * Note: The service main thread will be invoked in cooperative mode
 * and service main entry will invoke even before APP main function.
 * Service main should complete as quickly as possible and offload
 * all its time consuming work to a sub task to improve boot latency
 **/
DEFINE_USER_MODE_APP(2, USER_MODE_SERVICE | K_PART_GLOBAL | K_PART_SHARED,
		     ehl_oob_main_service, OOB_THREAD_STACK_SIZE,
		     (void **)obj_list, K_OBJS, oob_init_config);

#endif /*endif CONFIG_SOC_INTEL_PSE*/

/**
 * \brief Function which sends platform information like
 * static telemetry and events on startup
 *
 * @returnval void
 **/
void send_platform_info(void)
{
	/* Sending event messages */
	post_message("Elkhart Lake Out-of-band App - Start", EVENT);

	/* Sending static telemetry */
	send_telemetry("CONFIG_ARCH", CONFIG_ARCH, STATIC);
	send_telemetry("CONFIG_SOC_SERIES", "Elkhart_Lake", STATIC);
	send_telemetry("CONFIG_BOARD", CONFIG_BOARD, STATIC);
	send_telemetry("CONFIG_KERNEL_BIN_NAME",
		       CONFIG_KERNEL_BIN_NAME,
		       STATIC);
	LOG_INF("Static Telemetry published...\n");
	post_message("Static Telemetry published...", EVENT);
}

/**
 * brief Function:
 * cloud connection recovery/re-stablishing
 *
 *@returnval void
 **/
static void device_conn_recover(void)
{
	int status;

	status = device_cloud_init();
	if (status != OOB_SUCCESS) {
		LOG_ERR("PSE-OOB Cloud Re-init failed: %d\n", status);
	}
}

/**
 * brief Function:
 * abort connection at interface level
 * disconnected from cloud device
 *
 *@returnval void
 **/
static void device_conn_teardown(void)
{
	int status;

	status = device_cloud_abort();
	if (status != OOB_SUCCESS) {
		LOG_ERR("PSE-OOB Cloud Disconnection failed: %d\n", status);
	}
}

/**
 * brief Function:
 * Release existing mqtt connection,
 * release OOB credentials disables pow
 * state flag for oob and releases sem for
 * sec heci thread to complete decommission flow
 *
 * @returnval void
 **/
void oob_service_decommission(void)
{
	/* delay to give post message time */
	k_yield();
	k_sleep(K_MSEC(APP_MINI_LOOP_TIME));

	/* Terminate connection */
	device_conn_teardown();
	oob_release_credentials();
#if defined(CONFIG_OOB_BIOS_IPC)
	k_sem_give(&sec_hc_oob_sem);
#endif
}

/**
 * brief Function:
 * for configured schedule existing TLS connection
 * will be restablished teardown/restablish connection
 *
 * @returnval void
 **/
void oob_tls_session_expiry(struct k_timer *timer)
{
	int ret;

	tls_state_q.current_msg_type = TLS_SESSION_STATE_EVENT;
	ret = k_fifo_alloc_put(&managability_fifo, &tls_state_q);
	if (ret) {
		LOG_WRN("PSE_OOB Fifo allocation failed");
	}
}

/**
 * brief Function:
 * post message to cloud server when sx transition is
 * not happened with in configured timeout.
 *
 * @returnval void
 **/
void oob_power_trans_expiry(struct k_timer *timer)
{
	int ret;

	sx_timer_q.current_msg_type = PLT_STATE_TIMER_EVENT;
	ret = k_fifo_alloc_put(&managability_fifo, &sx_timer_q);
	if (ret) {
		LOG_WRN("PSE_OOB SX Power Fifo allocation failed");
	}
	pmc_reboot = false;
}

void oob_sx_trans_expiry(struct k_timer *timer)
{
	int ret;

	sx_state_q.current_msg_type = PLT_STATE_CHANGE_EVENT;
	ret = k_fifo_alloc_put(&managability_fifo, &sx_state_q);
	if (ret) {
		LOG_WRN("PSE_OOB SX state Change Fifo allocation failed");
	}
}

static void oob_post_sx_state(void)
{
	/* Start timer */
	sx_state_q.current_msg_type = PLT_STATE_CHANGE_EVENT;
	k_fifo_put(&managability_fifo, &sx_state_q);
}

/**
 * brief Function which
 * loops in the main thread waiting for messages which are entered
 * from the protocol level callback listening from cloud
 *
 * a.) Interact with Bios for cloud adapter credentials
 * b.) Sets the cloud adapter
 * c.) Connect to cloud
 * d.) Sends platform info
 * e.) wait for cloud events
 *
 * @returnval void
 **/
void wait_for_events(void)
{
	int rc;
	int sx_timer = CONFIG_OOB_PM_SX_TRANSITION_TIME;

	while (connected) {
		data_item = NULL;
		rc = OOB_ERR_MESSAGE_FAILED;

		data_item = k_fifo_get(&managability_fifo, K_FOREVER);

		if (data_item == NULL) {
			continue;
		}

		switch (data_item->current_msg_type) {
		case RELAY:
			rc = post_message(data_item->next_msg, API);
			if (rc != OOB_SUCCESS) {
				LOG_ERR(OOB_API_MSG_ERR "error: (%d)\n", rc);
				continue;
			}
			break;

		case REBOOT:
			rc = post_message(data_item->next_msg, API);
			if (rc != OOB_SUCCESS) {
				LOG_ERR(OOB_API_MSG_ERR "error: (%d)\n", rc);
			}
			rc = post_message(OOB_REBOOT_CMD, EVENT);
			if (rc != OOB_SUCCESS) {
				LOG_ERR(OOB_EVENT_MSG_ERR "error: (%d)\n", rc);
			}

			if (pm_sx_st == ACTIVE_POWER) {
				if (!pmc_action(PMC_ACTION_REBOOT_HOST)) {
					/* Start timer */
					k_timer_start(&oob_power_trans_timer,
						      K_SECONDS(sx_timer), K_NO_WAIT);
					pmc_reboot = true;
					LOG_INF(OOB_REBOOT_SUCCESS);
					post_message(OOB_REBOOT_SUCCESS, EVENT);
				} else {
					LOG_INF(OOB_REBOOT_FAILED);
					post_message(OOB_REBOOT_FAILED, EVENT);
				}
			} else {
				LOG_WRN(OOB_REBOOT_IGNORE);
				post_message(OOB_REBOOT_IGNORE, EVENT);
			}
			break;

		case POWERON:
			rc = post_message(data_item->next_msg, API);
			if (rc != OOB_SUCCESS) {
				LOG_ERR(OOB_API_MSG_ERR "error: (%d)\n", rc);
			}
			rc = post_message(OOB_POWER_ON_TRIGGER, EVENT);
			if (rc != OOB_SUCCESS) {
				LOG_ERR(OOB_EVENT_MSG_ERR "error: (%d)\n", rc);
			}

			if (pm_sx_st == LOW_POWER) {
				if (!pmc_action(PMC_ACTION_POWERON)) {
					/* Start timer */
					k_timer_start(&oob_power_trans_timer,
						      K_SECONDS(sx_timer), K_NO_WAIT);
					LOG_INF(OOB_PMC_POWERON_SUCCESS);
					post_message(OOB_PMC_POWERON_SUCCESS, EVENT);
				} else {
					LOG_INF(OOB_PMC_POWERON_FAILED);
					post_message(OOB_PMC_POWERON_FAILED, EVENT);
				}
			} else {
				post_message(OOB_PM_ACTIVE_PWR_STATE, EVENT);
			}
			break;

		case POWEROFF:
			rc = post_message(data_item->next_msg, API);
			if (rc != OOB_SUCCESS) {
				LOG_ERR(OOB_API_MSG_ERR "error: (%d)\n", rc);
			}
			rc = post_message(OOB_POWER_OFF_TRIGGER, EVENT);
			if (rc != OOB_SUCCESS) {
				LOG_ERR(OOB_EVENT_MSG_ERR "error: (%d)\n", rc);
			}

			if (pm_sx_st == ACTIVE_POWER) {
				if (!pmc_action(PMC_ACTION_POWEROFF)) {
					/* Start timer */
					k_timer_start(&oob_power_trans_timer,
						      K_SECONDS(sx_timer), K_NO_WAIT);
					LOG_INF(OOB_PMC_POWEROFF_SUCCESS);
					post_message(OOB_PMC_POWEROFF_SUCCESS, EVENT);
				} else {
					LOG_INF(OOB_PMC_POWEROFF_FAILED);
					post_message(OOB_PMC_POWEROFF_FAILED, EVENT);
				}
			} else {
				post_message(OOB_PM_LOW_PWR_STATE, EVENT);
			}
			break;

#if defined(CONFIG_OOB_BIOS_IPC)
		case DECOMMISSION:
			rc = post_message(data_item->next_msg, API);
			if (rc != OOB_SUCCESS) {
				LOG_ERR(OOB_API_MSG_ERR "error: (%d)\n", rc);
			}

			if (pm_sx_st == ACTIVE_POWER) {
				rc = post_message(OOB_DECOMMISSION_TRIGGER, EVENT);
				if (rc != OOB_SUCCESS) {
					LOG_ERR(OOB_API_MSG_ERR "error: (%d)\n",
						rc);
				}

				/* update decommistion state in SEC context
				 * SEC_DECOMM_PEND
				 */
				if (oob_set_provision_state(
					    (unsigned int)SEC_DECOM) ==
				    OOB_SUCCESS) {
					LOG_INF(OOB_DECOMMISSION_EN_SUCCESS);
					post_message(OOB_DECOMMISSION_EN_SUCCESS
						     , EVENT);
					if (!pmc_action(PMC_ACTION_REBOOT_HOST)) {
						LOG_INF(OOB_DECOMMISSION_SUCCESS);
						k_sem_take(&sec_hc_oob_sem,
							   K_SECONDS(
								   DECOMM_TIMEOUT));
						is_thread_termi_set = true;
						post_message(
							OOB_DECOMMISSION_SUCCESS,
							EVENT);
						goto exit;
					} else {
						LOG_INF(OOB_DECOMMISSION_FAILED);
						post_message(
							OOB_DECOMMISSION_FAILED,
							EVENT);
					}
				} else {
					LOG_INF(OOB_DECOMMISSION_EN_FAILED);
					post_message(OOB_DECOMMISSION_EN_FAILED,
						     EVENT);
				}
			} else {
				LOG_WRN(OOB_DECOMMISSION_IGNORE);
				post_message(OOB_DECOMMISSION_IGNORE, EVENT);
			}
			break;
#endif

		case PLT_STATE_CHANGE_EVENT:
			if (pm_sx_st == LOW_POWER) {
				LOG_INF(OOB_PM_SX_ENTRY);
				post_message(OOB_PM_SX_ENTRY, EVENT);
			} else if (pm_sx_st == ACTIVE_POWER) {
				LOG_INF(OOB_PM_SX_EXIT);
				post_message(OOB_PM_SX_EXIT, EVENT);
			}
			break;

		case PLT_STATE_TIMER_EVENT:
			post_message(OOB_PM_SX_TRANSITION_FAILED, EVENT);
			break;

		case TLS_SESSION_STATE_EVENT:
			LOG_INF("PSE-OOB TLS Session re-connecting.");
			oob_conn_st = TLS_SESSION_RECONNECT;
			k_sem_give(&oob_conn_sem);
			break;

		case MQTT_STATE_CHANGE_EVENT:
			rc = mqtt_live(&client);
			if (rc != 0 && rc != -EAGAIN) {
				PRINT_RESULT("mqtt_live", rc);
			}

			k_yield();
			k_sleep(K_MSEC(APP_MINI_LOOP_TIME));
			mqtt_input(&client);

			if (connected) {
				LOG_INF("Connected to cloud...\n");
				LOG_INF("Listening to commands from cloud...\n");
			} else {
				LOG_INF("Lost Connection to cloud\n");
				LOG_INF("Retrying connection...\n");
				device_cloud_abort();
				device_cloud_connect();
			}
			break;

		case PLT_NW_MGMT_EVENT:
			LOG_INF("PSE-OOB NW Mgmt Event received\n");
			oob_conn_st = NW_MGMT_EVENT_RECEIVED;
			k_sem_give(&oob_conn_sem);
			break;

		case IGNORE:
			/* No action required, protocol should not populate fifo
			 * with messages of IGNORE type
			 */
			break;
		}
	}
exit:
	LOG_INF("Exit: %s\n", __func__);
}

#if defined(CONFIG_NET_DHCPV4)
static void oob_ipv4_add_handler(struct net_mgmt_event_callback *cb,
				 uint32_t mgmt_event,
				 struct net_if *iface)
{
	char hr_addr[NET_IPV4_ADDR_LEN];
	int i = 0;

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		/* Spurious callback. */
		return;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		struct net_if_addr *if_addr =
			&iface->config.ip.ipv4->unicast[i];

		if (if_addr->addr_type != NET_ADDR_DHCP || !if_addr->is_used) {
			continue;
		}
		LOG_INF("DHCPv4 address: %s",
			log_strdup(net_addr_ntop(AF_INET,
						 &if_addr->address.in_addr,
						 hr_addr, NET_IPV4_ADDR_LEN)));
		LOG_INF("Lease time: %u seconds",
			iface->config.dhcpv4.lease_time);
		LOG_INF("Subnet: %s",
			log_strdup(net_addr_ntop(AF_INET,
						 &iface->config.ip.ipv4->netmask,
						 hr_addr, NET_IPV4_ADDR_LEN)));
		LOG_INF("Router: %s",
			log_strdup(net_addr_ntop(AF_INET,
						 &iface->config.ip.ipv4->gw,
						 hr_addr, NET_IPV4_ADDR_LEN)));
		break;
	}
}

/**
 * \brief Function which configures the DHCPV4
 * It loops till the retry limit exceeds.
 *
 * @param struct net_if *default_iface
 * @returnval returns a non-zero integer on error
 **/
int oob_setup_dhcpv4(struct net_if *default_iface)
{
	int retry = DHCP_MAX_RETRY;

	net_mgmt_init_event_callback(&iface_cb, oob_ipv4_add_handler,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&iface_cb);

	net_dhcpv4_start(default_iface);

	while (retry--) {
		if (default_iface->config.dhcpv4.state == NET_DHCPV4_BOUND) {
			LOG_INF("DHCPV4 acquired !!");
			return OOB_SUCCESS;
		}

		LOG_INF("Waiting for DHCP to assign address");
		k_sleep(K_MSEC(APP_MINI_LOOP_TIME));
	}

	LOG_WRN("Failed to acquire dhcpv4");
	return OOB_ERR_NO_IPV4_ADDR;
}
#endif /* End of CONFIG_NET_DHCPV4 */

/**
 * \brief Function which waits for the default interface to be up
 * It loops till the interface is up.
 * On success, configure DHCPV4 or fall back to static ipv4.
 * @returnval returns a non-zero integer on error
 **/
int wait_for_nw_iface_ip(void)
{
	default_iface = net_if_get_default();

	if (default_iface == NULL) {
		LOG_ERR("No default interface found");
		return OOB_ERR_NO_INTERFACE_FOUND;
	}

	while (!net_if_is_up(default_iface)) {
		LOG_INF("Waiting for Net Interface (%p) to be up",
			default_iface);
		k_sleep(K_MSEC(APP_MINI_LOOP_TIME));
	}

	LOG_INF("Net Interface (%p) is up", default_iface);

#if defined(CONFIG_NET_DHCPV4)
	if (oob_setup_dhcpv4(default_iface) != OOB_SUCCESS) {
		LOG_WRN("Configure Static IP through TSN Capsule !!!");
	}
#endif  /* End of CONFIG_NET_DHCPV4 */

	return OOB_SUCCESS;
}

static void oob_l2_event_handler(struct net_mgmt_event_callback *cb,
				 unsigned int mgmt_event,
				 struct net_if *iface)
{
	LOG_INF("PSE-OOB received %x event", mgmt_event);

	if ((mgmt_event & L2_EVENT_MASK) != mgmt_event) {
		return;
	}

	/* Save to global variable */
	nw_mgmt_event = mgmt_event;
	if (nw_mgmt_event == NET_EVENT_IF_UP) {
		LOG_INF("NW interface up received");
		oob_conn_st = NW_MGMT_EVENT_INIT;
	} else {
		nw_mgmt_q.current_msg_type = PLT_NW_MGMT_EVENT;
		k_fifo_put(&managability_fifo, &nw_mgmt_q);
	}
}

/**
 * \brief Function to register NW management events
 *
 * @returnval void
 **/
void oob_register_nw_mgmt_events(void)
{
	/* Register L2 iface Events*/
	net_mgmt_init_event_callback(&l2_mgmt_cb, oob_l2_event_handler,
				     L2_EVENT_MASK);
	net_mgmt_add_event_callback(&l2_mgmt_cb);

	oob_conn_st = NW_MGMT_EVENT_INIT;
}

static void oob_sx_handler(sedi_pm_sx_event_t event, void *ctx)
{
	switch (event) {
	case PM_EVENT_HOST_SX_ENTRY:
		pm_sx_st = LOW_POWER;
		break;
	case PM_EVENT_HOST_SX_EXIT:
		pm_sx_st = ACTIVE_POWER;
		pmc_reboot = false;
		break;
	default:
		LOG_ERR("Invalid PM SX notification");
		break;
	}

	/* Post the Sx events when triggered from host */
	oob_post_sx_state();

	/* Stop timer */
	if (pmc_reboot == false) {
		k_timer_stop(&oob_power_trans_timer);
	}
}

/**
 * \brief Function to interact with SEC to obtain OOB
 * credentials from BIOS via IPC or from AONRF
 *
 * @returnval returns a non-zero integer on error
 **/
int get_oob_credential(void)
{
	int result = OOB_SUCCESS;

#if defined(CONFIG_OOB_BIOS_IPC)
	if (sec_start(
		    SEC_DAEMON,
		    CONFIG_OOB_BIOS_IPC_TIMEOUT) == SEC_FAILED) {
		LOG_INF("PSE-OOB BIOS IPC startup failed...\n");
		result = SEC_FAILED;
	} else {
		LOG_INF("PSE-OOB BIOS IPC startup succeeded...");
	}
#else
	LOG_INF("PSE-OOB BIOS IPC skipped ...");
#endif

	if (!result) {
		memset(&cred_var, 0x00, sizeof(cred_var));
		creds = &cred_var;

		result = populate_credentials();
	}

	return result;
}

/**
 * \brief Function to register PM SX event notification
 *
 * @returnval void
 **/
void oob_register_sx_notification(void)
{
	if (sedi_pm_register_sx_notification(oob_sx_handler, NULL) != OOB_SUCCESS) {
		LOG_ERR("PSE_OOB failed to register pm sx notification");
	}
}

/**
 * \brief Function to set/reset the PM control bit.
 *
 * @param bool set_oob
 * @returnval void
 **/
void oob_set_pm_control(bool set_oob)
{
	if (set_oob) {
		sedi_pm_set_control(SEDI_PM_IOCTL_OOB_SUPPORT, 1);
		LOG_INF("Enable PM flows for OOB");
	} else {
		sedi_pm_set_control(SEDI_PM_IOCTL_OOB_SUPPORT, 0);
		LOG_INF("Disable PM flows for OOB");
	}
}

/**
 * \brief Entry point to ehl-oob manageability APP.
 * Performs the following:
 *
 * a.) Interact with Bios for cloud adapter credentials
 * b.) Sets the cloud adapter
 * c.) Connect to cloud
 * d.) Sends platform info
 * e.) wait for cloud events
 *
 * @returnval returns a non-zero integer on error
 *
 **/
int ehl_oob_bootstrap(void)
{
	int result = OOB_SUCCESS;
	int connect_status;

	k_sem_take(&oob_cfg_sem, K_FOREVER);

	/* Periodic timer for every 24hrs */
	k_timer_start(&oob_tls_session_timer,
		      K_HOURS(CONFIG_OOB_TLS_DURATION),
		      K_HOURS(CONFIG_OOB_TLS_DURATION));

	while (true) {
		if (oob_conn_st == NW_MGMT_EVENT_STOP) {
			LOG_WRN("Network unplugged, wait until plug in\n");
			k_yield();
			k_sleep(K_MSEC(APP_MINI_LOOP_TIME));
			continue;
		}

		connect_status = device_cloud_connect();
		LOG_WRN("Device cloud connect: 0x%x\n", connect_status);
		if (connect_status != OOB_SUCCESS) {
			k_timer_stop(&oob_tls_session_timer);
			return OOB_ERR_THREAD_ABORT;
		}

		/* Send platform telemetry information */
		send_platform_info();

		/* Wait for events - Power Control, Decommission etc */
		wait_for_events();

		/* Thread handle for graceful termination */
		if (is_thread_termi_set) {
			LOG_WRN("Device Decommissioned: OOB Service Thread Exit\n");
			oob_service_decommission();
			k_timer_stop(&oob_tls_session_timer);
			LOG_WRN("Device Decommissioned: OOB Main Config Thread Exit started\n");
			k_sem_give(&oob_conn_sem);
			return OOB_ERR_THREAD_ABORT;
		}
	}

	return result;
}

#if defined(CONFIG_SOC_INTEL_PSE)
static void oob_cfg_task(void *p1, void *p2, void *p3)
{
	int result = OOB_SUCCESS;
	int init_status;

	/* Set OOB Manageability from BIOS menu */
#if defined(CONFIG_OOB_BIOS_IPC)
	if (SEDI_CONFIG_SET !=
	    sedi_get_config(SEDI_CONFIG_OOB_EN, NULL)) {
		LOG_ERR("PSE_OOB Not available");
		return;
	}
#endif

	k_thread_heap_assign(&ehl_oob_service_task, &oob_pool_name);

	/* Register PM Sx event notification */
	oob_register_sx_notification();

	/* Get OOB credentials from BIOS via IPC */
	result = get_oob_credential();
	if (result != OOB_SUCCESS) {
		return;
	}

	/* Set cloud adapter information */
	initialize_cloud_adapter();

	/* Check for NW iface and IP address */
	if (wait_for_nw_iface_ip() != OOB_SUCCESS) {
		LOG_ERR("PSE_OOB failed to acquire IP address");
		return;
	}

	/* Register NW events */
	oob_register_nw_mgmt_events();

	init_status = device_cloud_init();
	if (init_status == OOB_ERR_TLS_CREDENTIALS_ADD) {
		LOG_ERR("PSE_OOB failed to add CERT..check if CERT is valid");
		return;
	}

	/* Enable PSE PM flow for OOB */
	oob_set_pm_control(true);

	LOG_INF("EHL-OOB Init configuration completed, resume Main thread.");
	k_sem_give(&oob_cfg_sem);

	/* Thread handle for gracefull TLS session & NW Mgmt Event */
	while (1) {
		k_sem_take(&oob_conn_sem, K_FOREVER);

		if ((connected == true) &&
		    (oob_conn_st == NW_MGMT_EVENT_RECEIVED)) {
			if (nw_mgmt_event == NET_EVENT_IF_DOWN) {
				LOG_INF("NW interface down. Abort MQTT session");
				oob_conn_st = NW_MGMT_EVENT_STOP;
				device_conn_teardown();
			}
		}
		if (oob_conn_st == TLS_SESSION_RECONNECT) {
			device_conn_teardown();
			LOG_INF("PSE-OOB Cloud Disconnected");
			device_conn_recover();
			LOG_INF("PSE-OOB Cloud Re-connected");
		}

		if (is_thread_termi_set == true) {
			LOG_INF("EHL OOB Main thread terminated.");
			return;
		}
		LOG_INF("EHL OOB suspend Main thread.");
	}
}

static void oob_init_config(void)
{
#if defined(CONFIG_USERSPACE)
	k_mem_domain_add_partition(&dom2, &z_malloc_partition);
	k_mem_domain_add_partition(&dom2, &k_mbedtls_partition);
#endif

	LOG_DBG("Starting EHL OOB CFG task\n");
	k_tid_t oob_cfg_thread = k_thread_create(&ehl_oob_cfg_task, ehl_oob_cfg_stack,
						 OOB_THREAD_STACK_SIZE,
						 oob_cfg_task, NULL, NULL, NULL,
						 OOB_CFG_THREAD_PRIO, 0, K_NO_WAIT);

	if (!oob_cfg_thread) {
		LOG_DBG("EHL OOB Conf task failed to start\n");
		return;
	}
}

/**
 * \brief The only sub-task for OOB.
 * It executes in ehl_oob_service_task thread in
 * user mode.
 *
 * It calls the ehl_oob_bootstrap function: main entry point
 * for the EHL OOB APP
 *
 * @param void pointer p1
 * @param void pointer p2
 * @param void pointer p3
 *
 * @returnval void
 *
 **/
static void ehl_oob_sub_task(void *p1, void *p2, void *p3)
{
	ehl_oob_bootstrap();
}

/**
 * \brief This is the main service function
 * that gets called after the kernel init process
 * in supervisory mode. This function must
 * finish its execution really fast to avoid any
 * kernel holdup. Hence the only thing it does is
 * launch APP entry point in another thread.
 *
 * The substask this function calls will eventually call
 * ehl_oob_bootstrap function which is the entry point of
 * the OOB APP.
 *
 * @param void pointer p1
 * @param void pointer p2
 * @param void pointer p3
 *
 * @returnval void
 *
 **/
static void ehl_oob_main_service(void *p1, void *p2, void *p3)
{

	LOG_DBG("Starting EHL OOB main service task\n");
	k_tid_t oob_thread = k_thread_create(&ehl_oob_service_task,
					     ehl_oob_service_task_stack,
					     OOB_THREAD_STACK_SIZE,
					     ehl_oob_sub_task, NULL,
					     NULL, NULL, 10,
					     APP_USER_MODE_TASK | K_INHERIT_PERMS, K_FOREVER);

#if !defined(CONFIG_USERSPACE)
	k_thread_heap_assign(&ehl_oob_service_task, &oob_pool_name);
#endif

	if (!oob_thread) {
		LOG_DBG("EHL OOB main service task failed to start\n");
		return;
	}

	k_thread_start(&ehl_oob_service_task);
	LOG_DBG("EHL OOB main service task started");
}

#endif /* End of CONFIG_SOC_INTEL_PSE */

