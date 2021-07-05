# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(can)

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/drivers/can)
target_sources(app PRIVATE src/main.c)
