idf_component_register(
    SRCS
        "app_main.cpp"
        "app_controller.cpp"
        "boards/board_profile.c"
        "boards/common/axp2101_power.c"
        "boards/common/modem_gpio_power.c"
        "device_identity.cpp"
        "sensor_runtime.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver smonitor_client smonitor-modem smonitor-i2c smonitor_battery
)

if(CONFIG_SMONITOR_BOARD_GENERIC_ESP32_UART)
    target_sources(${COMPONENT_LIB} PRIVATE "boards/generic_esp32_uart.c")
elseif(CONFIG_SMONITOR_BOARD_LILYGO_T_SIM7080_S3)
    target_sources(${COMPONENT_LIB} PRIVATE "boards/lilygo_t_sim7080_s3.c")
elseif(CONFIG_SMONITOR_BOARD_LILYGO_T_SIM7670_S3)
    target_sources(${COMPONENT_LIB} PRIVATE "boards/lilygo_t_sim7670_s3.c")
else()
    target_sources(${COMPONENT_LIB} PRIVATE "boards/lilygo_t_sim7000g.c")
endif()
