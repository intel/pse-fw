# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

set(DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/pse_sram.overlay)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pse_network_proxy)

target_sources(app PRIVATE src/main.c)
