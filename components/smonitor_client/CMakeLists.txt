idf_component_register(
    SRCS
        "websocket_client.cpp"
        "json_client.cpp"
        "device_config.cpp"
        "gpio_control.cpp"
        "time_sync.cpp"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "."
    REQUIRES smonitor-i2c
    PRIV_REQUIRES esp_event esp_websocket_client json driver lwip
    EMBED_TXTFILES "isrgrootx1.pem"
)
