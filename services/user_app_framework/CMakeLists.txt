zephyr_include_directories(include)
zephyr_sources_ifdef(CONFIG_USER_APP_FRAMEWORK user_app_framework.c)
zephyr_sources_ifndef(CONFIG_USER_APP_FRAMEWORK non_user_mode_framework.c)
