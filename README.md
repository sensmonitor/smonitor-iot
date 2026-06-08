# smonitor-iot

Public ESP32 client for SensMonitor.

## Version 0.1 scope

- ESP-IDF 5.5.4
- LilyGO T-SIM7000G with SIM7000
- `smonitor-modem` for cellular PPP
- `smonitor-i2c-reader` for sensor acquisition
- SensMonitor WebSocket device configuration and sample delivery

The device serial is derived from the ESP32 factory eFuse MAC:

```text
SM-ESP32-A1B2C3D4E5F6
```

If that serial is unknown to the server, firmware will report
`DEVICE_NOT_REGISTERED` and instruct the user to register the device in the
SensMonitor application. The serial identifies a device but is not a secret.

## Local setup

Keep these repositories as siblings:

```text
Code/
  smonitor-iot/
  smonitor-modem/
  smonitor-i2c-reader/
```

Activate ESP-IDF 5.5.4, then:

```bash
idf.py set-target esp32
idf.py menuconfig
idf.py build
```

APN, username, password and device token are empty by default. The
`examples/lilygo_t_sim7000g_vipmobile/sdkconfig.defaults` file is a local
operator example, not a production default.

The initial scaffold establishes repository and configuration boundaries.
The working PPP, WebSocket, JSON, GPIO and device-configuration code from
`modem_console` will be migrated into those boundaries next.
