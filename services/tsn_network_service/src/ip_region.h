/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define IP_SUB_REGION_SIZE_MAX (4096)
#define PSE_GBE_PORT_MAX (4)
#define DNS_SERVER_ENTRY_MAX (4)
#define IP_SUB_REGION_SIGNATURE_SIZE (256)  /* assume SHA256 */

/* This struct will be consumed by PSE FW only */
struct pse_tsn_ip_config {
	uint32_t bus_dev_fnc;  /* associated PCI(Bus:Dev:Fnc), ECAM format */
	uint32_t valid;  /* 0 = not valid, else = valid */

	uint32_t static_ip;  /* 0 = Dynamic IP from DHCPv4, else = Static IP */
	/* 32bit ipv4 address associated with this port */
	uint8_t ipv4_addr[4];
	/* 128bit ipv6 address associated with this port */
	uint8_t ipv6_addr[16];
	/* 32bit ipv4 subnet address associated with this port */
	uint8_t subnet_mask[4];
	/* 32bit ipv4 gateway address associated with this port */
	uint8_t gateway[4];
	uint32_t dns_srv_num;
	/* 32bit ipv4 dns server address associated with this port */
	uint8_t dns_srv_addr[DNS_SERVER_ENTRY_MAX][4];
	/* 32bit ipv4 proxy server address associated with this port */
	uint8_t proxy_addr[4];
	uint32_t proxy_port;
} __attribute__((packed));

union pse_ip_config_sub_region_u {
	/* ensures the data structure consumes SIZE_MAX */
	uint8_t u8_data[IP_SUB_REGION_SIZE_MAX];

	struct ip_config_data {
		/* data format version */
		uint32_t version;
		/* number of valid pse_tsn_ip_addrs in this structure */
		uint32_t num_ports;
		/* IP Addresses structures */
		struct pse_tsn_ip_config port[PSE_GBE_PORT_MAX];
		/* signature of the relevant data in this structure */
		uint8_t signature[IP_SUB_REGION_SIGNATURE_SIZE];
	} config;
} pse_ip_config_sub_region;
