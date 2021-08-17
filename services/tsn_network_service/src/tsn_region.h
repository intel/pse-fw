/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define TSN_SUB_REGION_SIZE_MAX (4096)
#ifndef TSN_PORT_MAX
#define TSN_PORT_MAX (4)
#endif
#define TSN_SUB_REGION_SIGNATURE_SIZE (256)  /* assume SHA256 */
#define TSN_CBS_QUEUE_MAX (8)
#define TSN_EST_GC_LIST_MAX (20)

struct tsn_cbs_queue {
	uint32_t bandwidth;
} __attribute__((packed));

struct tsn_cbs {
	uint32_t valid;  /* 0 = not valid, else = valid */
	uint32_t num_queues;
	struct tsn_cbs_queue queue[TSN_CBS_QUEUE_MAX];
} __attribute__((packed));

enum TSN_EST_GC_OP_MODES {
	TSN_EST_GC_HOLD = 0x00000000,
	TSN_EST_GC_RELEASE = 0x00000001,
	TSN_EST_GC_SET = 0x00000002,
	TSN_EST_GC_OP_MODE_MAX = 0xFFFFFFFF  /* ensure 4 bytes allocated */
};

struct tsn_est_gc {
	uint32_t gate_control;
	uint32_t time_interval_nsec;
	enum TSN_EST_GC_OP_MODES operation_mode;
} __attribute__((packed));

struct tsn_est {
	uint32_t valid;  /* 0 = not valid, else = valid */
	uint32_t base_time_nsec;
	uint32_t base_time_sec;
	uint32_t cycle_time_nsec;
	uint32_t cycle_time_sec;
	uint32_t time_extension_nsec;
	uint32_t num_gc_lists;
	struct tsn_est_gc gc_list[TSN_EST_GC_LIST_MAX];
} __attribute__((packed));

struct tsn_fpe {
	uint32_t valid;  /* 0 = not valid, else = valid */
	uint32_t hold_advance_nsec;
	uint32_t release_advance_nsec;
	uint32_t additional_fragment;
	uint32_t frame_preemption_status_table;
} __attribute__((packed));

struct tsn_tbs {
	uint32_t valid;  /* 0 = not valid, else = valid */
	/* corresponding bits refer to queue number and set/unset represent
	 * that queue use tbs or not
	 */
	uint32_t tbs_queues;
} __attribute__((packed));

/* This struct will be consumed by PSE FW only as tsn config only valid when
 * PSE owned
 */
struct pse_tsn_mac_config {
	uint32_t valid;  /* 0 = not valid, else = valid */

	struct tsn_cbs cbs;  /* CBS 802.1Qav configuration data */
	struct tsn_est est;  /* EST 802.1Qbv configuration data */
	struct tsn_fpe fpe;  /* FPE 802.1Qbu configuration data */
	struct tsn_tbs tbs;  /* TBS configuration data */
} __attribute__((packed));

/* This struct will be consumed by BIOS only as VC/TC config only valid when
 * host owned
 */
struct ehl_vc_tc_mac_config {
	uint32_t valid;  /* 0 = not valid, else = valid */

	/* Assumption: TC0 always map to VC0, TC1 always map to VC1
	 *
	 * Bit value 0 = VC0/TC0, Bit value 1 = VC1/TC1
	 * Bit[1, 3, 5, 7, 9, 11, 13, 15] map to TX DMA channel 0 - 7
	 * Bit[0, 2, 4, 6, 8, 10, 12, 14] map to RX DMA channel 0 - 7
	 *
	 * EHL default value = 0xFF00, TGL/ICL default value = 0x05E0
	 */
	uint32_t dma_vc_tc_map;
	/* The bit mapping is following MSI vector sequence */
	uint32_t msi_vc_tc_map;
} __attribute__((packed));

struct ehl_tsn_vc_tc_config {
	uint32_t bus_dev_fnc;  /* associated PCI(Bus:Dev:Fnc), ECAM format */

	/* TSN port configuration structures */
	struct pse_tsn_mac_config tsn_port;
	/* VC/TC port configuration structures */
	struct ehl_vc_tc_mac_config vc_tc_port;
} __attribute__((packed));

union {
	/* ensures the data structure consumes SIZE_MAX */
	uint8_t u8_data[TSN_SUB_REGION_SIZE_MAX];

	struct tsn_config_data {
		/* data format version */
		uint32_t version;
		/* number of valid pse_tsn_mac_configs in this structure */
		uint32_t num_ports;
		/* TSN or VC/TC port configuration structures */
		struct ehl_tsn_vc_tc_config port[TSN_PORT_MAX];
		/* signature of the relevant data in this structure */
		uint8_t signature[TSN_SUB_REGION_SIGNATURE_SIZE];
	} config;
} pse_tsn_config_sub_region;
