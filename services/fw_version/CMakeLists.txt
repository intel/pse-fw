# Copyright (c) 2021 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_MISC_FW_VERSION)
  zephyr_include_directories(${PROJECT_BINARY_DIR}/fw_version)

  add_custom_target(fw_version
    COMMAND ${CMAKE_COMMAND} -D PROJECT_BINARY_DIR=${PROJECT_BINARY_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/fw_version.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  add_dependencies(zephyr fw_version)

  zephyr_sources(fw_version.c)
endif()
