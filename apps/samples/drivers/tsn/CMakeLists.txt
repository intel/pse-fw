# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tsn)

zephyr_include_directories(
    ${ZEPHYR_BASE}/kernel/include/
    ${ARCH_DIR}/${ARCH}/include
)

target_sources(app PRIVATE src/main.c)
