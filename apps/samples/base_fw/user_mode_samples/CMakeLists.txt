# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(user_mode_samples)

include_directories(
user_mode_services
)

target_sources(app PRIVATE user_mode_apps/simple_app_example/app0_simple_app_example.c)
target_sources(app PRIVATE user_mode_apps/sub_task_example/app1_sub_task_example.c)
target_sources(app PRIVATE user_mode_services/inter_service_comm/serv0_service_example.c)
target_sources(app PRIVATE user_mode_services/inter_service_comm/serv1_service_example.c)
target_sources(app PRIVATE resource_table.c)
