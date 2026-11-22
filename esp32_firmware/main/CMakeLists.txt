# NOTE: Omitting REQUIRES/PRIV_REQUIRES allows ESP-IDF 'main' to auto-link all core
# components (including IDF 6 split drivers esp_driver_* and mbedtls) as well as
# managed components from idf_component.yml (lvgl__lvgl, espressif__cjson).
idf_component_register(
    SRCS
        "app_main.cpp"
        "cyd_display.cpp"
        "cyd_hw.c"
        "cyd_lvgl_ui.cpp"
        "cyd_net.cpp"
        "cyd_storage.cpp"
        "cyd_touch.c"
    INCLUDE_DIRS
        "."
        ".."
)
