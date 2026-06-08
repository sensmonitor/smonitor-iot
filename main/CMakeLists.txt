idf_component_register(
    SRCS
        "app_main.cpp"
        "device_identity.cpp"
    INCLUDE_DIRS "."
    REQUIRES smonitor-modem smonitor-i2c-reader esp_event nvs_flash
)
