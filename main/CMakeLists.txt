idf_component_register(
    SRCS
        "app_main.cpp"
        "app_controller.cpp"
        "device_identity.cpp"
        "sensor_runtime.cpp"
    INCLUDE_DIRS "."
    REQUIRES smonitor_client smonitor-modem smonitor-i2c-reader smonitor_battery
)
