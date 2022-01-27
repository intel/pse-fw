# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

set(DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/pse_sram.overlay)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(code_relocation_ccm)


target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/code_l2sram.c)
target_sources(app PRIVATE src/code_iccm.c)
target_sources(app PRIVATE src/code_tx_bss_data_split.c)

#This example will use ICCM and DCCM for Base FW and hence
#can only use L2SRAM memory for relocating Apps code
zephyr_code_relocate(src/code_l2sram.c L2SRAM)

zephyr_code_relocate(src/code_tx_bss_data_split.c L2SRAM_TEXT)
zephyr_code_relocate(src/code_tx_bss_data_split.c L2SRAM_BSS)
zephyr_code_relocate(src/code_tx_bss_data_split.c L2SRAM_DATA)

