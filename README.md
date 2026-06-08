# SensMonitor IoT Ciient (web app on https://app.sensmonitor.com/ )

Public ESP32 firmware for SensMonitor devices.

This repository contains the final user-facing ESP-IDF project. It boots an
ESP32 board, connects a cellular modem over PPP, reads the configured I2C
sensor, requests device configuration from SensMonitor, and sends samples to
the SensMonitor WebSocket endpoint.

The first supported hardware profile is:

- ESP32 target
- LilyGO T-SIM7000G board
- SIM7000 modem over UART
- BME280 I2C environmental sensor
- ESP-IDF 5.5.4

## What This Firmware Does

At runtime the application:

1. Derives a device serial from the ESP32 factory eFuse MAC.
2. Initializes the configured I2C sensor.
3. Powers and configures the SIM7000 modem.
4. Opens a PPP data connection.
5. Synchronizes system time using SNTP.
6. Connects to the SensMonitor WebSocket endpoint.
7. Requests device configuration by serial number.
8. Applies sensor and digital I/O configuration received from the server.
9. Reads sensor values and sends samples continuously.

The generated serial has this format:

```text
SM-ESP32-A1B2C3D4E5F6
```

The serial identifies the physical device in SensMonitor. It is not a secret.

If the serial is not registered on the server, firmware logs
`DEVICE_NOT_REGISTERED`. Register that serial in the SensMonitor application,
then restart the device so it requests configuration again.

## Dependencies

ESP-IDF Component Manager downloads these public Git dependencies
automatically:

- `https://github.com/sensmonitor/smonitor-modem`
- `https://github.com/sensmonitor/smonitor-i2c-reader`

They are declared in `main/idf_component.yml` and installed under
`managed_components/`. Users do not need sibling repository checkouts.

The current manifests follow the `main` branches, while `dependencies.lock`
records the exact resolved commits. Tagged releases should replace branch
references when the first stable component versions are published.

## Repository Layout

Current project layout:

```text
smonitor-iot/
  main/
    app_main.cpp
    app_controller.cpp
    device_identity.cpp
    sensor_runtime.cpp
    Kconfig.projbuild

  components/
    smonitor_client/
      include/smonitor_client.h
      websocket_client.cpp
      json_client.cpp
      device_config.cpp
      gpio_control.cpp
      time_sync.cpp
      Kconfig

  examples/
    lilygo_sim7000g_bme280/
      sdkconfig.defaults
```

`main` owns the application flow: device identity, modem startup, sensor
runtime and the sample loop.

`components/smonitor_client` owns the SensMonitor protocol: TLS WebSocket,
device configuration, JSON sample formatting, digital I/O updates and SNTP
time synchronization.

`smonitor-modem` owns modem power, SIM7000 AT setup and PPP connectivity.

`smonitor-i2c-reader` owns I2C bus access and sensor decoding.

## Requirements

- ESP-IDF 5.5.4
- ESP32 target
- LilyGO T-SIM7000G board
- SIM card with data enabled
- APN for the mobile operator
- BME280 connected over I2C

The default LilyGO/BME280 wiring used by this project is:

| Signal | GPIO |
| --- | ---: |
| Modem TX | 27 |
| Modem RX | 26 |
| Modem RTS | 25 |
| Modem CTS | 23 |
| Modem PWRKEY | 4 |
| I2C SDA | 21 |
| I2C SCL | 22 |

The default BME280 address is `0x76`.

## Build

Activate ESP-IDF 5.5.4, then build. Internet access is required during the
first build so Component Manager can download dependencies:

```bash
cd smonitor-iot
idf.py set-target esp32
idf.py build
```

Flash and monitor:

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

Use the serial port for your board if it differs.

## Configuration

Configuration is stored in ESP-IDF `sdkconfig`. You can edit it through
`idf.py menuconfig` or edit the generated `sdkconfig` file directly for local
testing.

For a clean user build, start from the project defaults and configure the
operator APN before building.

### Mobile Network

Menu path:

```text
SensMonitor IoT
```

Important options:

| Option | Meaning |
| --- | --- |
| `CONFIG_SMONITOR_MODEM_APN` | Mobile operator APN. Required. Empty by default. |
| `CONFIG_SMONITOR_MODEM_USERNAME` | PPP username. Often empty. |
| `CONFIG_SMONITOR_MODEM_PASSWORD` | PPP password. Often empty. |
| `CONFIG_SMONITOR_MODEM_AUTH_NONE` | Disable PPP authentication. Default for LilyGO/SIM7000G profile. |
| `CONFIG_SMONITOR_MODEM_AUTH_PAP` | Use PAP with the configured username/password. |
| `CONFIG_SMONITOR_MODEM_NETWORK_NB_IOT` | Prefer NB-IoT. Default for the current profile. |
| `CONFIG_SMONITOR_MODEM_NETWORK_LTE_M` | Prefer LTE-M. |
| `CONFIG_SMONITOR_MODEM_NETWORK_AUTO` | Let modem/operator choose automatically. |
| `CONFIG_SMONITOR_MODEM_CONNECT_TIMEOUT_MS` | PPP connection timeout. Default `180000`. |

Example operator configuration:

```text
CONFIG_SMONITOR_MODEM_APN="your-apn"
CONFIG_SMONITOR_MODEM_USERNAME=""
CONFIG_SMONITOR_MODEM_PASSWORD=""
CONFIG_SMONITOR_MODEM_AUTH_NONE=y
CONFIG_SMONITOR_MODEM_NETWORK_NB_IOT=y
CONFIG_SMONITOR_MODEM_LTE_BAND=20
```

Use the APN, authentication mode, network technology and radio band supplied
by your mobile operator. Enable PAP only if the operator requires it.

### I2C Sensor

Menu path:

```text
SensMonitor IoT > I2C sensor
```

Important options:

| Option | Meaning |
| --- | --- |
| `CONFIG_SMONITOR_I2C_SENSOR_BME280` | Select BME280. Currently the supported profile. |
| `CONFIG_SMONITOR_I2C_PORT` | ESP32 I2C controller. Default `0`. |
| `CONFIG_SMONITOR_I2C_SDA_PIN` | SDA GPIO. Default `21`. |
| `CONFIG_SMONITOR_I2C_SCL_PIN` | SCL GPIO. Default `22`. |
| `CONFIG_SMONITOR_I2C_FREQUENCY_HZ` | I2C frequency. Default `100000`. |
| `CONFIG_SMONITOR_I2C_INTERNAL_PULLUPS` | Enables internal pull-ups. Useful for development. |
| `CONFIG_SMONITOR_I2C_DEVICE_ADDRESS` | Sensor address. Default `0x76`. |

GPIO 21/22 are the current board profile, not universal ESP32 I2C pins. Change
them if your wiring differs.

### SensMonitor Client

Menu path:

```text
SensMonitor client
```

Important options:

| Option | Meaning |
| --- | --- |
| `CONFIG_SMONITOR_WEBSOCKET_URI` | SensMonitor WebSocket endpoint. |
| `CONFIG_SMONITOR_CLIENT_TIME_SYNC_RETRIES` | SNTP retries before continuing. |
| `CONFIG_SMONITOR_SAMPLE_INTERVAL_MS` | Sample loop period. Default `5000`. |

The default WebSocket URI points to the production SensMonitor endpoint.

## Example Profile

An example profile is provided in:

```text
examples/lilygo_sim7000g_bme280/sdkconfig.defaults
```

It describes the LilyGO T-SIM7000G + BME280 hardware profile. APN, username
and password remain empty so public builds do not accidentally ship operator
credentials.

To use it manually, copy or merge the relevant values into your local
`sdkconfig` or use it as a reference when running `idf.py menuconfig`.

## Expected Log Flow

A healthy boot should look like this at a high level:

```text
smonitor_iot: Device serial: SM-ESP32-...
smonitor_sensor: Initializing BME280 on I2C0, SDA=21, SCL=22, address=0x76
smonitor_sensor: BME280 initialized successfully
smonitor_modem: PPP authentication: none
smonitor_modem: Power on the modem
smonitor_modem: Initializing esp_modem for SIM7000
smonitor_modem: PPP event ... phase establish
smonitor_modem: PPP event ... phase network
smonitor_modem: PPP event ... phase running
smonitor_modem: Modem connected to PPP server
smonitor_modem: IP          : ...
time_sync: System time set.
smonitor_client: WebSocket connected
smonitor_client: Sent device-config-request for: SM-ESP32-...
smonitor_client: Received device config with ... active sensors.
smonitor_iot: environment/temperature = ...
smonitor_client: Sent data: ...
smonitor_client: Received device-sample-ack.
```

If the device is not registered:

```text
DEVICE_NOT_REGISTERED: Register SM-ESP32-... in the SensMonitor application.
```

## Troubleshooting

### APN Is Empty

Log:

```text
APN is empty. Configure SensMonitor IoT > Mobile network APN.
```

Set `CONFIG_SMONITOR_MODEM_APN` in `sdkconfig` or `idf.py menuconfig`.

### PPP Does Not Get an IP

Check:

- APN is correct.
- SIM card has data enabled.
- Antenna is connected.
- Selected network mode is supported by the SIM/operator.
- Band is correct for your region/operator.
- PPP authentication mode matches the operator.

For the tested LilyGO/SIM7000G profile use:

```text
CONFIG_SMONITOR_MODEM_AUTH_NONE=y
CONFIG_SMONITOR_MODEM_NETWORK_NB_IOT=y
CONFIG_SMONITOR_MODEM_LTE_BAND=20
```

PPP failures are logged by name, for example:

```text
PPP error 7: authentication failed
PPP error 9: peer timeout
```

### BME280 Does Not Initialize

Check:

- SDA/SCL wiring.
- Sensor power.
- I2C address: `0x76` or `0x77`.
- Pull-ups. External pull-ups are recommended for reliable hardware.

The Espressif `i2c_bus` component may log a probe message before creating the
bus. The important line is:

```text
BME280 initialized successfully
```

### WebSocket Does Not Connect

Check:

- PPP has an IP first.
- System time is synchronized.
- `CONFIG_SMONITOR_WEBSOCKET_URI` is correct.
- TLS certificate bundle in `smonitor_client` is present.

### Device Is Not Registered

The firmware derives the serial from ESP32 eFuse MAC. Register the logged
serial in the SensMonitor application:

```text
SM-ESP32-A1B2C3D4E5F6
```

## Current Limitations

- BME280 is the only configured sensor profile.
- LilyGO T-SIM7000G/SIM7000 is the first supported modem board profile.
- Runtime provisioning is not yet stored in NVS; configuration is build-time
  through `sdkconfig`.
- The SensMonitor client is currently built from source inside this repo. It
  can later be distributed as a precompiled component if needed.
