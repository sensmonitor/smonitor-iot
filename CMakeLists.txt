cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED SDKCONFIG_DEFAULTS)
    set(SDKCONFIG_DEFAULTS "sdkconfig.defaults")
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults.local")
        list(APPEND SDKCONFIG_DEFAULTS "sdkconfig.defaults.local")
    endif()
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(smonitor_iot)
