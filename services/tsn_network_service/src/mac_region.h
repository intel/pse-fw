/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define MAC_ADDR_SUB_REGION_SIZE_MAX (4096)
#ifndef TSN_PORT_MAX
#define TSN_PORT_MAX (4)
#endif
#define MAC_ADDR_SUB_REGION_SIGNATURE_SIZE (256)  /* assume SHA256 */

/* This struct will be consumed by BIOS & PSE FW */
struct tsn_mac_addr {
	uint32_t bus_dev_fnc;  /* associated PCI(Bus:Dev:Fnc), ECAM format */

	union {
		uint64_t u64_mac_addr;
		uint32_t u32_mac_addr[2];
		uint8_t u8_mac_addr[6];
	} mac_addr;  /* MAC address associated with this port */
} __attribute__((packed));

union {
	/* ensures the data structure consumes SIZE_MAX */
	uint8_t u8_data[MAC_ADDR_SUB_REGION_SIZE_MAX];

	struct mac_config_data {
		/* data format version */
		uint32_t version;
		/* number of valid ia_tsn_mac_addrs in this structure */
		uint32_t num_ports;
		/* TSN MAC Address structures */
		struct tsn_mac_addr port[TSN_PORT_MAX];
		/* signature of the relevant data in this structure */
		uint8_t signature[MAC_ADDR_SUB_REGION_SIGNATURE_SIZE];
	} config;
} tsn_mac_addr_sub_region;
