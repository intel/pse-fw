# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
include_directories($ENV{ZEPHYR_BASE}/subsys/net/l2/)
include_directories($ENV{ZEPHYR_BASE}/subsys/net/ip/)
project(NONE)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/upd_arp.c)
target_sources(app PRIVATE src/sock_udp.c)
target_sources(app PRIVATE src/send_ping.c)
