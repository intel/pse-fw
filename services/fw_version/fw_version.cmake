# Copyright (c) 2021 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.2)
set(fwver_in_file ${CMAKE_CURRENT_SOURCE_DIR}/../../FW_VERSION)
set(fwver_out_file ${PROJECT_BINARY_DIR}/fw_version/fw_version.h)
set(fwver_env_file ${PROJECT_BINARY_DIR}/fw_version/fw_version_env)
unset(fwver_str_list)
file(REMOVE ${fwver_out_file} ${fwver_env_file})

if(EXISTS ${fwver_in_file})
  file(STRINGS ${fwver_in_file} fwver_in)
  foreach(line ${fwver_in})
    string(REGEX MATCH "([A-Z_]*) = ([0-9A-Za-z_]*)" _ ${line})
    set(FWVER_${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
    set(fwver_str_list ${fwver_str_list} FWVER_${CMAKE_MATCH_1})
  endforeach()
else()
  message(FATAL_ERROR "FW_VERSION file is missing!")
endif()

string(TIMESTAMP FWVER_BUILD_DATE %m%d%Y)
set(fwver_str_list ${fwver_str_list} FWVER_BUILD_DATE)
string(TIMESTAMP FWVER_BUILD_TIME %H%M%S)
set(fwver_str_list ${fwver_str_list} FWVER_BUILD_TIME)

execute_process(COMMAND hostname
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  OUTPUT_VARIABLE FWVER_HOST_NAME)
string(STRIP ${FWVER_HOST_NAME} FWVER_HOST_NAME)
set(fwver_str_list ${fwver_str_list} FWVER_HOST_NAME)

execute_process(COMMAND whoami
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  OUTPUT_VARIABLE FWVER_WHOAMI)
string(STRIP ${FWVER_WHOAMI} FWVER_WHOAMI)
set(fwver_str_list ${fwver_str_list} FWVER_WHOAMI)

macro(gen_git_ver git_dir)
  execute_process(COMMAND git diff-index --name-only HEAD
    WORKING_DIRECTORY ${git_dir}
    OUTPUT_VARIABLE git_dirty)

  execute_process(COMMAND git rev-parse --verify --short HEAD
    WORKING_DIRECTORY ${git_dir}
    OUTPUT_VARIABLE _git_ver)
  if(NOT ${_git_ver} STREQUAL "")
    string(STRIP ${_git_ver} git_ver)
  endif()

  if(NOT "${git_dirty}" STREQUAL "")
    string(APPEND git_ver "-dirty")
  endif()
endmacro()


configure_file(include/fw_version.h.in ${fwver_out_file})


file(APPEND ${fwver_out_file} "\n\n/* user defined */\n")
file(READ ${fwver_out_file} fwver_out)
foreach(ver_str ${fwver_str_list})
  file(APPEND ${fwver_env_file} "${ver_str}=${${ver_str}}\n")
  string(REGEX MATCH "#define ${ver_str} " out_matched ${fwver_out})
  if(NOT "${out_matched}" STREQUAL "")
    continue()
  endif()

  file(APPEND ${fwver_out_file} "#define ${ver_str} \"${${ver_str}}\"\n")
endforeach()
