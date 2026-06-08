cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../smonitor-modem"
    "${CMAKE_CURRENT_LIST_DIR}/../smonitor-i2c-reader"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(smonitor_iot)
