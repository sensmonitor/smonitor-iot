idf_component_register(
    SRCS
        "app_main.cpp"
        "app_controller.cpp"
        "boards/common/modem_gpio_power.c"
        "boards/lilygo_t_sim7000g.c"
        "device_identity.cpp"
        "sensor_runtime.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver smonitor_client smonitor-modem smonitor-i2c-reader smonitor_battery
)
