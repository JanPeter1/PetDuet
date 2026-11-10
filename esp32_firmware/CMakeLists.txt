cmake_minimum_required(VERSION 3.16)

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Project configuration
set(PROJECT_NAME "petduet")
set(PROJECT_VER "1.0.0")

# Component requirements
set(COMPONENT_REQUIRES
    "esp_https_ota"
    "esp_tls"
    "cjson"
    "driver"
    "spi_flash"
    "nvs_flash"
    "esp_wifi"
    "esp_netif"
    "esp_event"
    "protocol_examples_common"
    "json"
    "lvgl"
    "esp_timer"
    "freertos"
    "log"
)

project(${PROJECT_NAME} VERSION ${PROJECT_VER})