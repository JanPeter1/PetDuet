# PetDuet ESP32 CYD - ESP-IDF Project
cmake_minimum_required(VERSION 3.16)

set(PROJECT_VER "2.1.4")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Ensure LVGL v8 picks up main/lv_conf.h cleanly across all components
idf_build_set_property(COMPILE_DEFINITIONS "-DLV_CONF_INCLUDE_SIMPLE=1" APPEND)
idf_build_set_property(INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/main" APPEND)

project(petduet)
