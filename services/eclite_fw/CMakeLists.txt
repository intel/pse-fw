# SPDX-License-Identifier: Apache-2.0

if (CONFIG_SOC_INTEL_PSE)

FILE(GLOB_RECURSE app_sources src/*.c)

zephyr_include_directories(
src/main/include
src/agents/include
src/frameworks/charger_framework/include
src/frameworks/device_framework/include
src/frameworks/thermal_framework/include
src/frameworks/usbc_framework/include
src/ext/pal/include
src/drivers/battery_driver
src/drivers/charger_driver
src/drivers/thermal_driver
src/drivers/fan_driver
src/drivers/usbc_pd_driver
src/drivers/include
)

zephyr_sources_ifdef(CONFIG_ECLITE_SERVICE ${app_sources})

else()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(NONE)

FILE(GLOB_RECURSE app_sources src/*.c)

include_directories(
src/main/include
src/agents/include
src/frameworks/charger_framework/include
src/frameworks/device_framework/include
src/frameworks/thermal_framework/include
src/frameworks/usbc_framework/include
src/ext/pal/include
src/drivers/battery_driver
src/drivers/charger_driver
src/drivers/thermal_driver
src/drivers/fan_driver
src/drivers/usbc_pd_driver
src/drivers/include
)

target_sources(app PRIVATE ${app_sources})

target_compile_definitions(app PRIVATE CONFIG_EHL_IPHFPGA=1)
target_compile_definitions(app PRIVATE CONFIG_FG=1 CONFIG_CHG=1)
target_compile_definitions(app PRIVATE CONFIG_THERM=1 CONFIG_FAN=1)
#target_compile_definitions(app PRIVATE CONFIG_THERMAL_ENABLE=1 )

endif()
