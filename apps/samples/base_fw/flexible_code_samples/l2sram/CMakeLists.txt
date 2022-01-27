# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

set(DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/pse_sram.overlay)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(code_relocation_l2sram)


target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/code_l2sram.c)
target_sources(app PRIVATE src/code_iccm.c)
target_sources(app PRIVATE src/code_tx_bss_data_split.c)

#This example will use L2SRAM for Base FW and hence
#can only use CCM memory for relocating Apps code
zephyr_code_relocate(src/code_iccm.c ICCM_TEXT)
zephyr_code_relocate(src/code_iccm.c DCCM_DATA)
zephyr_code_relocate(src/code_iccm.c DCCM_BSS)

zephyr_code_relocate(src/code_tx_bss_data_split.c ICCM_TEXT)
zephyr_code_relocate(src/code_tx_bss_data_split.c DCCM_DATA)
zephyr_code_relocate(src/code_tx_bss_data_split.c DCCM_BSS)

