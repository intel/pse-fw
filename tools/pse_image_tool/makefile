#
# Copyright (c) 2019-2021 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

OUT_PATH := ./output
FRAGMENTS_PATH := ./fragments
BASE_DIR := ./

XML_OUTPUT_PATH="$(OUT_PATH)/PSE_FW.bin"
ifneq ("$(wildcard $(FRAGMENTS_PATH)/iccm.bin)","")
XML_ICCM_PATH="$(FRAGMENTS_PATH)/iccm.bin"
endif
ifneq ("$(wildcard $(FRAGMENTS_PATH)/aon_task.bin)","")
XML_AON_PATH="$(FRAGMENTS_PATH)/aon_task.bin"
endif
ifneq ("$(wildcard $(FRAGMENTS_PATH)/dccm.bin)","")
XML_DCCM_PATH="$(FRAGMENTS_PATH)/dccm.bin"
endif
ifneq ("$(wildcard $(FRAGMENTS_PATH)/bringup.bin)","")
XML_BRINGUP_PATH="$(FRAGMENTS_PATH)/bringup.bin"
endif
ifneq ("$(wildcard $(FRAGMENTS_PATH)/l2sram_ro.bin)","")
XML_L2SRAM_RO_PATH="$(FRAGMENTS_PATH)/l2sram_ro.bin"
endif
ifneq ("$(wildcard $(FRAGMENTS_PATH)/l2sram_rw.bin)","")
XML_L2SRAM_RW_PATH="$(FRAGMENTS_PATH)/l2sram_rw.bin"
endif

BUILD_H_PATH := $(BASE_DIR)/build.h
TARGET_XML :=$(OUT_PATH)/image_tool_setting.xml

TEMPLATE_XML := $(BASE_DIR)/pse_template.xml

CROSS_COMPILE := ${ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin/arm-zephyr-eabi-
CC      := $(CROSS_COMPILE)gcc

C_FLAGS=
C_FLAGS += -D__XML_OUTPUT_PATH=$(XML_OUTPUT_PATH)

ifneq ($(XML_ICCM_PATH),)
C_FLAGS += -D__XML_ICCM_PATH=$(XML_ICCM_PATH)
endif

ifneq ($(XML_DCCM_PATH),)
C_FLAGS += -D__XML_DCCM_PATH=$(XML_DCCM_PATH)
endif

ifneq ($(XML_L2SRAM_RO_PATH),)
C_FLAGS += -D__XML_L2SRAM_RO_PATH=$(XML_L2SRAM_RO_PATH)
endif

ifneq ($(XML_L2SRAM_RW_PATH),)
C_FLAGS += -D__XML_L2SRAM_RW_PATH=$(XML_L2SRAM_RW_PATH)
endif

ifneq ($(XML_AON_PATH),)
C_FLAGS += -D__XML_AON_PATH=$(XML_AON_PATH)
endif

ifneq ($(XML_BRINGUP_PATH),)
C_FLAGS += -D__XML_BRINGUP_PATH=$(XML_BRINGUP_PATH)
endif

all: $(XML_OUTPUT_PATH)

$(XML_OUTPUT_PATH): $(TARGET_XML)
	@echo ----------------------------------------------------------------
	@echo Generating final image
	@echo -------------------- Stitching PSE firmware --------------------
	@/usr/bin/env python3 pse_image_tool.py
FORCE:

.PHONY:xml_only
xml_only:$(TARGET_XML)

$(TARGET_XML): FORCE
	mkdir -p $(OUT_PATH)
	@echo Generating $(TARGET_XML)
	@$(CC) -x c -E -P "$(TEMPLATE_XML)" $(C_FLAGS) > "$(TARGET_XML)"

.PHONY: clean
clean:
	rm -rf $(OUT_PATH)
	rm -r $(FRAGMENTS_PATH)/*.*
