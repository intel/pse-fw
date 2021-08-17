FILE(GLOB_RECURSE tsn_net_service_source src/*.c)

zephyr_sources_ifdef(CONFIG_TSN_NET_SERVICE ${tsn_net_service_source})
