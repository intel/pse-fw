zephyr_sources(heci.c)
zephyr_sources_ifdef(CONFIG_SMHI            smhi_client.c)
zephyr_sources_ifdef(CONFIG_HECI_USE_DMA    heci_dma.c)
