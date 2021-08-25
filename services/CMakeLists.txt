#Placeholder for adding other services like eclite, oob, etc.

zephyr_include_directories(sys_service/include)

add_subdirectory(eclite_fw)

#add_subdirectory(tsn_network_service)
add_subdirectory(user_app_framework)
add_subdirectory(sys_service)
add_subdirectory(fw_version)
