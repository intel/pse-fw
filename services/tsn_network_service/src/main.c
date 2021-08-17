/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME tsn_net_service
#define LOG_LEVEL CONFIG_TSN_NET_SERVICE_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <init.h>
#include <zephyr.h>
#include <net/net_if.h>
#include <net/ethernet_mgmt.h>
#include <net/dns_resolve.h>
#include <ptp_clock.h>

#include "phyif_region.h"
#include "mac_region.h"
#include "ip_region.h"
#include "tsn_region.h"

/* Ensure TSN Network Service run before OOB service and user applications
 * Note: SYS_INIT priority field do NOT support symbolic expressions
 * (e.g. CONFIG_APPLICATION_INIT_PRIORITY - 5)
 */
#if (CONFIG_APPLICATION_INIT_PRIORITY > 90)
#define TSN_NET_INIT_PRIO       90
#elif (CONFIG_APPLICATION_INIT_PRIORITY > 80)
#define TSN_NET_INIT_PRIO       80
#elif (CONFIG_APPLICATION_INIT_PRIORITY > 70)
#define TSN_NET_INIT_PRIO       70
#elif (CONFIG_APPLICATION_INIT_PRIORITY > 60)
#define TSN_NET_INIT_PRIO       60
#elif (CONFIG_APPLICATION_INIT_PRIORITY > 50)
#define TSN_NET_INIT_PRIO       50
#elif (CONFIG_APPLICATION_INIT_PRIORITY > 40)
#define TSN_NET_INIT_PRIO       40
#elif (CONFIG_APPLICATION_INIT_PRIORITY > 30)
#define TSN_NET_INIT_PRIO       30
#elif (CONFIG_APPLICATION_INIT_PRIORITY > 20)
#define TSN_NET_INIT_PRIO       20
#elif (CONFIG_APPLICATION_INIT_PRIORITY > 10)
#define TSN_NET_INIT_PRIO       10
#elif (CONFIG_APPLICATION_INIT_PRIORITY >= 0)
#define TSN_NET_INIT_PRIO       0
#endif

/* AONRF memory partitions for TSN Network Ports */
#define PHYIF_DATA_AONRF_ADDR   PHYIF_REGION_AONRF_ADDR
#define MAC_DATA_AONRF_ADDR     (PHYIF_DATA_AONRF_ADDR \
				 + sizeof(tsn_phyif_region))
#define IP_DATA_AONRF_ADDR      (MAC_DATA_AONRF_ADDR \
				 + sizeof(tsn_mac_addr_sub_region.config))
#define TSN_DATA_AONRF_ADDR     (IP_DATA_AONRF_ADDR \
				 + sizeof(pse_ip_config_sub_region.config))
#define TSN_NET_AONRF_END_ADDR  (TSN_DATA_AONRF_ADDR \
				 + sizeof(pse_tsn_config_sub_region.config))

/* TSN Network Ports identification */
#ifndef PCI_ECAM_MASK
#define PCI_ECAM_MASK           0x001FF000
#endif
#ifndef PSE_GBE0_ECAM
#define PSE_GBE0_ECAM           0x000E9000  /* PCI bus: 0, dev: 29, func: 1 */
#endif
#ifndef PSE_GBE1_ECAM
#define PSE_GBE1_ECAM           0x000EA000  /* PCI bus: 0, dev: 29, func: 2 */
#endif
#ifndef PSE_MAX_GBE_PORTS
#define PSE_MAX_GBE_PORTS       2
#endif

static struct net_if *pciport_2_iface(uint32_t bus_dev_fnc)
{
	const struct device *dev;

	/* Get device from DWC EQoS config name if matched */
	if ((bus_dev_fnc & PCI_ECAM_MASK) == PSE_GBE0_ECAM &&
	    IS_ENABLED(CONFIG_ETH_DWC_EQOS_0)) {
#ifdef CONFIG_ETH_DWC_EQOS_0_NAME
		dev = device_get_binding(CONFIG_ETH_DWC_EQOS_0_NAME);
#else
		dev = device_get_binding("ETH_DWC_EQOS_0");
#endif
	} else if ((bus_dev_fnc & PCI_ECAM_MASK) == PSE_GBE1_ECAM &&
		   IS_ENABLED(CONFIG_ETH_DWC_EQOS_1)) {
#ifdef CONFIG_ETH_DWC_EQOS_1_NAME
		dev = device_get_binding(CONFIG_ETH_DWC_EQOS_1_NAME);
#else
		dev = device_get_binding("ETH_DWC_EQOS_1");
#endif
	} else {
		return NULL;
	}

	/* Kernel mode API to get and return net interface from device */
	if (dev) {
		return net_if_lookup_by_dev(dev);
	} else {
		return NULL;
	}
}

static void tsn_config_init(struct tsn_config_data *ptsn_conf)
{
	struct ethernet_req_params params = { 0 };
	struct net_if *iface;
	int i, j, k;

	/* Any PSE GbE ports available */
	if (!ptsn_conf->num_ports || ptsn_conf->num_ports > PSE_MAX_GBE_PORTS) {
		LOG_DBG("No valid entry for TSN region");
		return;
	}

	for (i = 0; i < ptsn_conf->num_ports; i++) {
		struct pse_tsn_mac_config *ptsn_port =
						&ptsn_conf->port[i].tsn_port;

		/* Next entry if valid bit unset */
		if (!ptsn_port->valid) {
			continue;
		}

		/* Obtain net_if from PCI port number */
		iface = pciport_2_iface(ptsn_conf->port[i].bus_dev_fnc);
		if (!iface) {
			LOG_WRN("PCI port [0x%08X] GbE interface not available",
				ptsn_conf->port[i].bus_dev_fnc);
			continue;
		}

		LOG_DBG("Configuring Qbv for interface index %d",
			net_if_get_by_iface(iface));

#ifdef CONFIG_ETH_DWC_EQOS_QAV
		if (!ptsn_port->cbs.valid) {
			goto NEXT_QBU;
		}

		for (j = 0; j < ptsn_port->cbs.num_queues &&
		     j < CONFIG_ETH_DWC_EQOS_TX_QUEUES; j++) {
			/* Skip Qav configuration if bandwidth = 100% */
			if (ptsn_port->cbs.queue[j].bandwidth == 100) {
				continue;
			}

			/* Extract CBS config from TSN Region structure */
			params.qav_param.queue_id = j;
			params.qav_param.delta_bandwidth =
				ptsn_port->cbs.queue[j].bandwidth;
			params.qav_param.type =
				ETHERNET_QAV_PARAM_TYPE_DELTA_BANDWIDTH;
			LOG_DBG("Obtained Q[%d] CBS Bandwidth: %d",
				j, params.qav_param.delta_bandwidth);

			/* Network Stack Kernel mode API to set bandwidth */
			if (net_mgmt(NET_REQUEST_ETHERNET_SET_QAV_PARAM,
				     iface, &params, sizeof(params))) {
				LOG_WRN("net_mgmt Qav set Q[%d] err", j);
				continue;
			}

			/* Network Stack Kernel mode API to turn on Qav */
			params.qav_param.enabled = 1;
			params.qav_param.type = ETHERNET_QAV_PARAM_TYPE_STATUS;
			if (net_mgmt(NET_REQUEST_ETHERNET_SET_QAV_PARAM,
				     iface, &params, sizeof(params))) {
				LOG_WRN("net_mgmt Qav turn on Q[%d] err", j);
				continue;
			}

		}
#endif  /* CONFIG_ETH_DWC_EQOS_QAV */

NEXT_QBU:
#ifdef CONFIG_ETH_DWC_EQOS_QBU
		if (!ptsn_port->fpe.valid) {
			goto NEXT_QBV;
		}

		/* Network Stack Kernel mode API to turn on Qbu */
		params.qbu_param.enabled = 1;
		params.qbu_param.type = ETHERNET_QBU_PARAM_TYPE_STATUS;
		if (net_mgmt(NET_REQUEST_ETHERNET_SET_QBU_PARAM,
			     iface, &params, sizeof(params))) {
			LOG_WRN("net_mgmt Qbu turn on err");
			continue;
		}

		/* Extract FPE Hold Advance Time from TSN Region structure */
		params.qbu_param.type = ETHERNET_QBU_PARAM_TYPE_HOLD_ADVANCE;
		params.qbu_param.hold_advance =
			ptsn_port->fpe.hold_advance_nsec;
		LOG_DBG("Obtained Qbu Hold Advance setting: %u",
			params.qbu_param.hold_advance);

		/* Network Stack Kernel mode API to set Qbu */
		if (net_mgmt(NET_REQUEST_ETHERNET_SET_QBU_PARAM, iface,
			     &params, sizeof(params))) {
			LOG_WRN("net_mgmt FPE Hold Advance Time set err");
		}

		/* Extract FPE Release Advance Time from TSN Region structure */
		params.qbu_param.type = ETHERNET_QBU_PARAM_TYPE_RELEASE_ADVANCE;
		params.qbu_param.release_advance =
			ptsn_port->fpe.release_advance_nsec;
		LOG_DBG("Obtained Qbu Release Advance setting: %u",
			params.qbu_param.release_advance);

		/* Network Stack Kernel mode API to set Qbu */
		if (net_mgmt(NET_REQUEST_ETHERNET_SET_QBU_PARAM, iface,
			     &params, sizeof(params))) {
			LOG_WRN("net_mgmt FPE Release Advance Time set err");
		}

		/* Extract FPE Additional Fragment from TSN Region structure */
		params.qbu_param.type =
			ETHERNET_QBR_PARAM_TYPE_ADDITIONAL_FRAGMENT_SIZE;
		params.qbu_param.additional_fragment_size =
			ptsn_port->fpe.additional_fragment;
		LOG_DBG("Obtained Qbu Additional Fragment setting: %u",
			params.qbu_param.additional_fragment_size);

		/* Network Stack Kernel mode API to set Qbu */
		if (net_mgmt(NET_REQUEST_ETHERNET_SET_QBU_PARAM, iface,
			     &params, sizeof(params))) {
			LOG_WRN("net_mgmt FPE Additional Fragment set err");
		}

		/* Extract FPE Preemption Status Table from TSN Region
		 * structure
		 */
		params.qbu_param.type =
			ETHERNET_QBU_PARAM_TYPE_PREEMPTION_STATUS_TABLE;
		for (j = 0; j < CONFIG_NET_TC_TX_COUNT; j++) {
			params.qbu_param.frame_preempt_statuses[j] = ((1 << j) &
				ptsn_port->fpe.frame_preemption_status_table) ?
				ETHERNET_QBU_STATUS_PREEMPTABLE :
				ETHERNET_QBU_STATUS_EXPRESS;
		}

		LOG_DBG("Obtained Qbu preemption status table setting: %u",
			ptsn_port->fpe.frame_preemption_status_table);

		/* Network Stack Kernel mode API to set Qbu */
		if (net_mgmt(NET_REQUEST_ETHERNET_SET_QBU_PARAM, iface,
			     &params, sizeof(params))) {
			LOG_WRN("net_mgmt FPE preemption status table set err");
		}
#endif  /* CONFIG_ETH_DWC_EQOS_QBU */

NEXT_QBV:
#ifdef CONFIG_ETH_DWC_EQOS_QBV
		if (!ptsn_port->est.valid) {
			goto NEXT_TBS;
		}

		/* Network Stack Kernel mode API to turn on Qbv */
		params.qbv_param.enabled = 1;
		params.qbv_param.type = ETHERNET_QBV_PARAM_TYPE_STATUS;
		if (net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM,
			     iface, &params, sizeof(params))) {
			LOG_WRN("net_mgmt Qbv turn on err");
			continue;
		}

		for (j = 0; j < ptsn_port->est.num_gc_lists &&
		     j < TSN_EST_GC_LIST_MAX; j++) {
			/* Extract GCL list from TSN Region structure */
			params.qbv_param.type =
				ETHERNET_QBV_PARAM_TYPE_GATE_CONTROL_LIST;
			params.qbv_param.gate_control.row = j;
			params.qbv_param.gate_control.time_interval =
				ptsn_port->est.gc_list[j].time_interval_nsec;
			params.qbv_param.gate_control.operation =
				ptsn_port->est.gc_list[j].operation_mode;
			for (k = 0; k < CONFIG_NET_TC_TX_COUNT; k++) {
				params.qbv_param.gate_control.gate_status[k] =
					!!((1 << k) &
					ptsn_port->est.gc_list[j].gate_control);
			}

			LOG_DBG("Obtained GCL[%d]: 0x%08X; %dns; %d mode",
				j, ptsn_port->est.gc_list[j].gate_control,
				ptsn_port->est.gc_list[j].time_interval_nsec,
				ptsn_port->est.gc_list[j].operation_mode);

			/* Network Stack Kernel mode API to set Qbv */
			if (net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM,
				     iface, &params, sizeof(params))) {
				LOG_WRN("net_mgmt GCL set entry[%d] err", j);
				continue;
			}
		}

		/* Set GCL list length base on the above programmed entries */
		params.qbv_param.type =
			ETHERNET_QBV_PARAM_TYPE_GATE_CONTROL_LIST_LEN;
		params.qbv_param.gate_control_list_len = j;
		LOG_DBG("Set GCL length: %d", j);
		if (net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM, iface,
			     &params, sizeof(params))) {
			LOG_WRN("net_mgmt GCL length(%d) set err", j);
		}

#ifdef CONFIG_PTP_CLOCK
		#define DEV_ACCESS_DELAY_OFFSET 2
		struct net_ptp_time time = {
			.second = 0,
			.nanosecond = 0,
		};
		const struct device *clk;

		/* Network Stack Kernel mode API to get clock device */
		clk = net_eth_get_ptp_clock(iface);
		if (!clk) {
			LOG_WRN("Interface no PTP support");
			goto NEXT_TBS;
		}

		/* Network Stack Kernel mode API to get PTP clock time */
		if (ptp_clock_get(clk, &time)) {
			LOG_WRN("Obtain PTP time failure");
			goto NEXT_TBS;
		}

		/* Extract EST times from TSN Region structure */
		params.qbv_param.type = ETHERNET_QBV_PARAM_TYPE_TIME;
		params.qbv_param.cycle_time.second =
			ptsn_port->est.cycle_time_sec;
		params.qbv_param.cycle_time.nanosecond =
			ptsn_port->est.cycle_time_nsec;
		params.qbv_param.extension_time =
			ptsn_port->est.time_extension_nsec;
		params.qbv_param.base_time.second = time.second
			+ DEV_ACCESS_DELAY_OFFSET
			+ ptsn_port->est.base_time_sec;
		params.qbv_param.base_time.fract_nsecond =
			ptsn_port->est.base_time_nsec;

		LOG_DBG("Obtained EST config setting:");
		LOG_DBG("Base time - %llus %lluns",
			params.qbv_param.base_time.second,
			params.qbv_param.base_time.fract_nsecond);
		LOG_DBG("Cycle time - %llus %uns",
			params.qbv_param.cycle_time.second,
			params.qbv_param.cycle_time.nanosecond);
		LOG_DBG("Extension time - %uns",
			params.qbv_param.extension_time);

		/* Network Stack Kernel mode API to set Qbv */
		if (net_mgmt(NET_REQUEST_ETHERNET_SET_QBV_PARAM, iface,
			     &params, sizeof(params))) {
			LOG_WRN("net_mgmt EST Time set err");
		}
#endif  /* CONFIG_PTP_CLOCK */
#endif  /* CONFIG_ETH_DWC_EQOS_QBV */

NEXT_TBS:
#ifdef CONFIG_ETH_DWC_EQOS_TBS
		if (!ptsn_port->tbs.valid) {
			continue;
		}

		/* Ensure link down only perform TBS configuration */
		int ret, iface_up = net_if_is_up(iface);

		if (unlikely(iface_up)) {
			ret = net_if_down(iface);
			if (ret) {
				LOG_WRN("Cannot take interface index[%d] down",
					net_if_get_by_iface(iface));
				continue;
			}
		}

		/* Extract TBS queues enable status from TSN Region structure */
		params.txtime_param.type =
			ETHERNET_TXTIME_PARAM_TYPE_ENABLE_QUEUES;
		for (j = 0; j < CONFIG_ETH_DWC_EQOS_TX_QUEUES; j++) {
			params.txtime_param.queue_id = j;
			if (ptsn_port->tbs.tbs_queues & BIT(j)) {
				params.txtime_param.enable_txtime = true;
			} else {
				params.txtime_param.enable_txtime = false;
			}

			if (net_mgmt(NET_REQUEST_ETHERNET_SET_TXTIME_PARAM,
				     iface, &params, sizeof(params))) {
				LOG_WRN("net_mgmt TBS set queue err");
			}
		}

		/* Link up back the interface if previously link downed */
		if (unlikely(iface_up)) {
			ret = net_if_up(iface);
			if (ret) {
				LOG_WRN("Cannot take interface index[%d] up",
					net_if_get_by_iface(iface));
			}
		}
#endif  /* CONFIG_ETH_DWC_EQOS_TBS */

		__NOP();
	}

	return;
}

static void ip_config_init(struct ip_config_data *pip_conf)
{
	struct net_if_addr *if_addr;
	struct net_if *iface;
	int i, j, k;

	/* Any PSE GbE ports available */
	if (!pip_conf->num_ports || pip_conf->num_ports > PSE_MAX_GBE_PORTS) {
		LOG_DBG("No valid entry for IP region");
		return;
	}

	for (i = 0; i < pip_conf->num_ports; i++) {
		/* Next entry if valid bit unset */
		if (!pip_conf->port[i].valid) {
			continue;
		}

		/* Obtain net_if from PCI port number */
		iface = pciport_2_iface(pip_conf->port[i].bus_dev_fnc);
		if (!iface) {
			LOG_WRN("PCI port [0x%08X] GbE interface not available",
				pip_conf->port[i].bus_dev_fnc);
			continue;
		}

		/* Configure IP address, Subnet Mask & Gateway if static IP
		 * selected.
		 */
		if (pip_conf->port[i].static_ip) {
#ifdef CONFIG_NET_IPV6
			struct in6_addr newaddr6;

			/* Extract IPv6 address from IP Region structure */
			for (j = 0; j < 16; j++) {
				newaddr6.s6_addr[j] =
					pip_conf->port[i].ipv6_addr[j];
			}

			LOG_DBG("Obtained IPv6 address:");
			LOG_DBG("%02X %02X %02X %02X %02X %02X %02X %02X",
				newaddr6.s6_addr[0], newaddr6.s6_addr[1],
				newaddr6.s6_addr[2], newaddr6.s6_addr[3],
				newaddr6.s6_addr[4], newaddr6.s6_addr[5],
				newaddr6.s6_addr[6], newaddr6.s6_addr[7]);
			LOG_DBG("%02X %02X %02X %02X %02X %02X %02X %02X",
				newaddr6.s6_addr[8], newaddr6.s6_addr[9],
				newaddr6.s6_addr[10], newaddr6.s6_addr[11],
				newaddr6.s6_addr[12], newaddr6.s6_addr[13],
				newaddr6.s6_addr[14], newaddr6.s6_addr[15]);

			/* Kernel mode API to add IPv6 address */
			if_addr = net_if_ipv6_addr_add(iface, &newaddr6,
						       NET_ADDR_OVERRIDABLE, 0);
			if (!if_addr) {
				LOG_ERR("Unable to set interface index[%d] IPv6"
					"addr", net_if_get_by_iface(iface));
			}
#endif  /* CONFIG_NET_IPV6 */

#ifdef CONFIG_NET_IPV4
			struct in_addr newaddr4;

			/* Extract IPv4 address from IP Region structure */
			for (j = 0; j < 4; j++) {
				newaddr4.s4_addr[j] =
					pip_conf->port[i].ipv4_addr[j];
			}

			LOG_DBG("Obtained IPv4 address: %u.%u.%u.%u",
				newaddr4.s4_addr[0], newaddr4.s4_addr[1],
				newaddr4.s4_addr[2], newaddr4.s4_addr[3]);

			/* Kernel mode API to add IPv4 address */
			if_addr = net_if_ipv4_addr_add(iface, &newaddr4,
						       NET_ADDR_OVERRIDABLE, 0);
			if (!if_addr) {
				LOG_ERR("Unable to set interface index[%d] IPv4"
					"addr", net_if_get_by_iface(iface));
			}

			/* Extract Subnet Mask from IP Region structure */
			for (j = 0; j < 4; j++) {
				newaddr4.s4_addr[j] =
					pip_conf->port[i].subnet_mask[j];
			}

			LOG_DBG("Obtained Subnet Mask: %u.%u.%u.%u",
				newaddr4.s4_addr[0], newaddr4.s4_addr[1],
				newaddr4.s4_addr[2], newaddr4.s4_addr[3]);

			/* Kernel mode API to set Subnet Mask */
			net_if_ipv4_set_netmask(iface, &newaddr4);

			/* Extract Gateway from IP Region structure */
			for (j = 0; j < 4; j++) {
				newaddr4.s4_addr[j] =
					pip_conf->port[i].gateway[j];
			}

			LOG_DBG("Obtained Gateway: %u.%u.%u.%u",
				newaddr4.s4_addr[0], newaddr4.s4_addr[1],
				newaddr4.s4_addr[2], newaddr4.s4_addr[3]);

			/* Kernel mode API to set Gateway */
			net_if_ipv4_set_gw(iface, &newaddr4);
#endif  /* CONFIG_NET_IPV4 */
		}

#ifdef CONFIG_DNS_RESOLVER
		const struct sockaddr *dns_servers[DNS_SERVER_ENTRY_MAX + 1];
		struct sockaddr_in dns_addr[DNS_SERVER_ENTRY_MAX];
		int ret;

		/* Extract DNS Servers from IP Region structure */
		for (k = 0; k < pip_conf->port[i].dns_srv_num &&
		     k < DNS_SERVER_ENTRY_MAX; k++) {
			dns_servers[k] = (struct sockaddr *)&dns_addr[k];
			dns_addr[k].sin_family = AF_INET;

			/* Extract DNS Server addr from IP Region structure */
			for (j = 0; j < 4; j++) {
				dns_addr[k].sin_addr.s4_addr[j] =
					pip_conf->port[i].dns_srv_addr[k][j];
			}

			LOG_DBG("Obtained DNS Server Address: %u.%u.%u.%u",
				dns_addr[k].sin_addr.s4_addr[0],
				dns_addr[k].sin_addr.s4_addr[1],
				dns_addr[k].sin_addr.s4_addr[2],
				dns_addr[k].sin_addr.s4_addr[3]);
		}

		/* Last entry of DNS Server array required set to NULL */
		dns_servers[k] = NULL;

		/* Kernel mode API to init DNS Servers */
		ret = dns_resolve_init(dns_resolve_get_default(),
				       NULL, dns_servers);
		if (ret < 0) {
			LOG_WRN("Cannot initialize DNS resolver (%d)", ret);
		}
#endif  /* CONFIG_DNS_RESOLVER */

		/* ToDo: Proxy Configuration initialization */

	}

	return;
}

static void mac_addr_init(struct mac_config_data *pmac_conf)
{
	struct ethernet_req_params params = { 0 };
	int iface_up, ret, i, j;
	struct net_if *iface;

	/* Any PSE GbE ports available */
	if (!pmac_conf->num_ports || pmac_conf->num_ports > PSE_MAX_GBE_PORTS) {
		LOG_DBG("No valid entry for MAC region");
		return;
	}

	for (i = 0; i < pmac_conf->num_ports; i++) {
		/* Obtain net_if from PCI port number */
		iface = pciport_2_iface(pmac_conf->port[i].bus_dev_fnc);
		if (!iface) {
			LOG_WRN("PCI port [0x%08X] GbE interface not available",
				pmac_conf->port[i].bus_dev_fnc);
			continue;
		}

		/* Ensure link down only perform MAC addr configuration */
		iface_up = net_if_is_up(iface);
		if (unlikely(iface_up)) {
			ret = net_if_down(iface);
			if (ret) {
				LOG_WRN("Cannot take interface index[%d] down",
					net_if_get_by_iface(iface));
				continue;
			}
		}

		/* Extract MAC address from MAC Region structure */
		for (j = 0; j < 6; j++) {
			params.mac_address.addr[j] =
				pmac_conf->port[i].mac_addr.u8_mac_addr[j];
		}

		LOG_DBG("Obtained MAC address: %02X:%02X:%02X:%02X:%02X:%02X",
			params.mac_address.addr[0], params.mac_address.addr[1],
			params.mac_address.addr[2], params.mac_address.addr[3],
			params.mac_address.addr[4], params.mac_address.addr[5]);

		/* Configure MAC address through Network Stack Kernel API */
		ret = net_mgmt(NET_REQUEST_ETHERNET_SET_MAC_ADDRESS, iface,
			       &params, sizeof(struct ethernet_req_params));
		if (ret) {
			LOG_ERR("Err[%d] to set interface index[%d] MAC addr",
				ret, net_if_get_by_iface(iface));
		} else {
			LOG_INF("Configured MAC addr to interface index[%d]",
				net_if_get_by_iface(iface));
		}

		/* Link up back the interface if previously link downed */
		if (unlikely(iface_up)) {
			ret = net_if_up(iface);
			if (ret) {
				LOG_WRN("Cannot take interface index[%d] up",
					net_if_get_by_iface(iface));
			}
		}
	}

	return;
}

static int tsn_net_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	struct mac_config_data *pmac_data = (void *)MAC_DATA_AONRF_ADDR;
	struct ip_config_data *pip_data = (void *)IP_DATA_AONRF_ADDR;
	struct tsn_config_data *ptsn_data = (void *)TSN_DATA_AONRF_ADDR;

	LOG_DBG("TSN Network Service start initialization");
	LOG_DBG("Pointer of MAC address region structure %p", pmac_data);
	LOG_DBG("Pointer of IP config region structure %p", pip_data);
	LOG_DBG("Pointer of TSN config region structure %p", ptsn_data);

	mac_addr_init(pmac_data);
	ip_config_init(pip_data);
	tsn_config_init(ptsn_data);

	return 0;
}
SYS_INIT(tsn_net_init, APPLICATION, TSN_NET_INIT_PRIO);
