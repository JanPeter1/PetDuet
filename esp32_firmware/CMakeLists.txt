# PetDuet ESP32 CYD - ESP-IDF Project
cmake_minimum_required(VERSION 3.16)

set(PROJECT_VER "2.1.11")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Ensure LVGL v8 picks up main/lv_conf.h cleanly across all components
idf_build_set_property(COMPILE_DEFINITIONS "-DLV_CONF_INCLUDE_SIMPLE=1" APPEND)
idf_build_set_property(INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/main" APPEND)

project(petduet)

# ESP-IDF 6.x uses GCC 15 (default C23 + -Werror). Keep LVGL v8 / cJSON building cleanly without relaxing app strictness.
foreach(_third_party_comp IN ITEMS lvgl__lvgl lvgl espressif__cjson)
    if(TARGET __idf_${_third_party_comp})
        target_compile_options(__idf_${_third_party_comp} PRIVATE
            $<$<COMPILE_LANGUAGE:C>:-std=gnu17>
            -Wno-error
        )
    endif()
endforeach()
