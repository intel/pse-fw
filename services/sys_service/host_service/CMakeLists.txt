add_library(subsys__host_service INTERFACE)

zephyr_include_directories(include)
zephyr_sources_ifdef(CONFIG_HOST_SERVICE host_ipc_service.c)
zephyr_sources_ifdef(CONFIG_SYS_MNG sys_mng.c)
add_subdirectory_ifdef(CONFIG_HECI heci)
