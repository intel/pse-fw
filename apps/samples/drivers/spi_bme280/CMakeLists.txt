# SPDX-License-Identifier: Apache-2.0


cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(spi_bme280)

target_sources(app PRIVATE src/main.c)
