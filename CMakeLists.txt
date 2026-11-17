cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED SDKCONFIG_DEFAULTS)
    set(SDKCONFIG_DEFAULTS "sdkconfig.defaults")
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults.local")
        list(APPEND SDKCONFIG_DEFAULTS "sdkconfig.defaults.local")
    endif()
    # Optional board profile defaults, e.g. examples/lilygo_sim7000g/sdkconfig.defaults.
    # Used by platformio.ini; also usable with idf.py -D SMONITOR_BOARD_DEFAULTS=...
    # Loaded after sdkconfig.defaults.local so the selected board's pins, target and
    # flash settings win; operator and sensor settings from the local file still apply.
    if(DEFINED SMONITOR_BOARD_DEFAULTS)
        list(APPEND SDKCONFIG_DEFAULTS "${SMONITOR_BOARD_DEFAULTS}")
    endif()
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Optional per-build lock file. PlatformIO environments use one under .pio/ so
# building different board profiles does not rewrite the committed dependencies.lock.
if(DEFINED SMONITOR_DEPENDENCIES_LOCK)
    idf_build_set_property(DEPENDENCIES_LOCK "${CMAKE_CURRENT_LIST_DIR}/${SMONITOR_DEPENDENCIES_LOCK}")
endif()

project(smonitor_iot)
