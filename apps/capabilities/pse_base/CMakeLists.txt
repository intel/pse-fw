cmake_minimum_required(VERSION 3.8.2)

#set(DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/pse_sram.overlay)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(pse_base)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/utility.c)
zephyr_sources_ifdef(CONFIG_PM_SERVICE src/sleep.c)

