cmake_minimum_required(VERSION 3.13.1)
if(CONFIG_BOARD_SAM_E70_XPLAINED)
	include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
	project(ehl_oob)
	target_sources(app PRIVATE adapter.c)
elseif(CONFIG_SOC_INTEL_PSE)
	zephyr_sources_ifdef(CONFIG_OOB_SERVICE telit.c adapter.c thingsboard.c
	azure_iot.c)
endif()
