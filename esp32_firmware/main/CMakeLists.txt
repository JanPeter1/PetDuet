idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES
                           rule_engine
                           pet_state
                           cyd_display
                           wifi_manager
                           ota_client
                           cjson
                           lvgl
                           esp_https_ota
                           esp_tls
                           nvs_flash
                           esp_wifi
                           esp_netif
                           esp_event
                           esp_timer
                           driver
                           freertos
                           log)