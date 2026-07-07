# SensMonitor IoT — ESP32 Cellular Firmware

[![ESP-IDF](https://github.com/sensmonitor/smonitor-iot/actions/workflows/esp-idf.yml/badge.svg)](https://github.com/sensmonitor/smonitor-iot/actions/workflows/esp-idf.yml)

Open-source ESP-IDF firmware for ESP32 cellular IoT devices. The first
hardware-tested profile supports the LilyGO T-SIM7000G with a SIM7000 modem,
PPPoS connectivity over UART, NB-IoT/LTE-M/GPRS, GNSS, BME280 and secure
WebSocket telemetry.

The firmware boots the board, connects the cellular modem over PPP, reads the
configured I2C sensor, requests device configuration from SensMonitor and
sends samples to the SensMonitor WebSocket endpoint.

[SensMonitor Web App](https://app.sensmonitor.com/) |
[LilyGO T-SIM7000G integration guide](https://sensmonitor.com/lilygo-iot-integrations/lilygo-t-sim7000g/) |
[Roadmap](ROADMAP.md) |
[Contributing](CONTRIBUTING.md)

## Hardware Support

| Board | Modem | Connection | Sensor | Status |
| --- | --- | --- | --- | --- |
| LilyGO T-SIM7000G | SIM7000G | UART/PPPoS | BME280 | Hardware-tested |
| Generic ESP32 + UART | SIM7000 | UART/PPPoS | Configurable I2C | Build-tested |

The tested build uses ESP-IDF 5.5.4. Additional board and modem profiles are
planned; they are not considered supported until their complete PPP and
telemetry flow has been verified on physical hardware.

The generic profile exposes UART, optional PWRKEY and I2C wiring through
Kconfig. It is a configurable integration starting point, not a claim that
every ESP32/SIM7000 board combination has been physically verified.

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
- `https://github.com/sensmonitor/smonitor-i2c`

They are declared in `main/idf_component.yml` and installed under
`managed_components/`. Users do not need sibling repository checkouts.

The manifests pin exact component commits, and `dependencies.lock` records the
complete resolved dependency graph. Tagged component versions will replace
commit pins for stable releases.

## Requirements

- Microsoft Visual Studio Code with the Espressif IDF extension, or a command
  line ESP-IDF 5.5.4 installation
- Git
- LilyGO T-SIM7000G board
- SIM card with data enabled
- APN for the mobile operator
- LTE antenna connected before powering the modem
- BME280 connected over I2C
- USB data cable

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

## Quick Start With Visual Studio Code

Visual Studio Code with the official Espressif IDF extension is the
recommended setup for the first build.

### 1. Install ESP-IDF

1. Install [Microsoft Visual Studio Code](https://code.visualstudio.com/).
2. Open **Extensions** with `Ctrl+Shift+X`.
3. Search for and install the official **Espressif IDF** extension.
4. Open the Command Palette with `Ctrl+Shift+P`.
5. Run `ESP-IDF: Open ESP-IDF Installation Manager`.
6. Install ESP-IDF 5.5.4 and its tools.
7. Run `ESP-IDF: Select Current ESP-IDF Version` and select the installed
   ESP-IDF 5.5.4 setup.

Use `ESP-IDF: Doctor Command` if the extension reports a tool or environment
problem. See the official
[ESP-IDF extension installation guide](https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/installation.html)
for platform-specific setup details.

### 2. Clone And Open The Project

Clone the repository:

```bash
git clone https://github.com/sensmonitor/smonitor-iot.git
cd smonitor-iot
```

In Visual Studio Code, select **File > Open Folder** and open the cloned
`smonitor-iot` directory. The extension detects this repository as a standard
ESP-IDF project.

### 3. Select The Target And Serial Port

Open the Command Palette and run:

```text
ESP-IDF: Set Espressif Device Target
```

Select `esp32`. Then run:

```text
ESP-IDF: Select Port to Use
```

Select the serial port for the board, such as `COM5` on Windows or
`/dev/ttyUSB0` on Linux. The exact port depends on the host and board
revision.

### 4. Configure The Firmware

Copy the local configuration template:

```bash
cp sdkconfig.defaults.local.example sdkconfig.defaults.local
```

On Windows PowerShell, use:

```powershell
Copy-Item sdkconfig.defaults.local.example sdkconfig.defaults.local
```

The local file is ignored by Git. Enter the APN and other operator-specific
settings there, then run:

```text
ESP-IDF: SDK Configuration Editor
```

Review the options under **SensMonitor IoT**, especially the APN, PPP
authentication, preferred mobile network and LPWA band. Do not commit mobile
operator credentials.

### 5. Build, Flash And Monitor

Run these commands from the Command Palette in order:

```text
ESP-IDF: Build your Project
ESP-IDF: Flash your Project
ESP-IDF: Monitor your Device
```

The first build requires internet access so ESP-IDF Component Manager can
download the pinned dependencies. A successful boot follows the sequence
shown in [Expected Log Flow](#expected-log-flow).

## Command-Line Build

Activate ESP-IDF 5.5.4, then build. Internet access is required during the
first build so Component Manager can download dependencies:

```bash
cd smonitor-iot
idf.py set-target esp32
cp sdkconfig.defaults.local.example sdkconfig.defaults.local
```

Fill in the mobile network settings in `sdkconfig.defaults.local`. The local
file is ignored by Git and is loaded automatically after the shared
`sdkconfig.defaults`.

```bash
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
CONFIG_SMONITOR_MODEM_LPWA_BAND=20
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

Board profile examples are provided in:

```text
examples/lilygo_sim7000g/sdkconfig.defaults
examples/generic_esp32_uart_modem/sdkconfig.defaults
```

The LilyGO profile defines its tested modem UART, PWRKEY, I2C and battery
wiring. The generic ESP32 UART modem profile uses SIM7000 as its current
default modem profile, but the folder name describes the reusable board
architecture: ESP32 plus a cellular modem over UART. It defaults to UART
GPIO17/GPIO16, no hardware flow control, an externally powered modem,
disabled battery monitoring and disabled LilyGO-specific active GPS antenna
power.

To use GPIO PWRKEY control with the generic profile, select:

```text
SensMonitor IoT > Cellular modem hardware > Modem power control > GPIO PWRKEY
```

Then configure the PWRKEY GPIO, active level, pulse duration and startup
delay for the exact modem board. Verify all generic pin and power settings
against the target board schematic before powering the hardware.

APN, sensor choice and mobile network settings remain outside the board
profiles.

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
CONFIG_SMONITOR_MODEM_LPWA_BAND=20
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

## Repository Layout

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
    generic_esp32_uart_modem/
      sdkconfig.defaults
    lilygo_sim7000g/
      sdkconfig.defaults
```

`main` owns the application flow, board pin assignments and modem power
sequencing. `components/smonitor_client` owns the SensMonitor TLS WebSocket
protocol, device configuration, JSON sample formatting, digital I/O updates
and SNTP synchronization.

The external `smonitor-modem` component owns SIM7000 AT setup and PPP
connectivity. The external `smonitor-i2c` component owns I2C bus access,
sensor profiles and decoding.

## Current Limitations

- BME280 is the only configured sensor profile.
- LilyGO T-SIM7000G/SIM7000 is the only hardware-tested modem board profile.
- The generic ESP32/UART SIM7000 profile is build-tested only.
- Runtime provisioning is not yet stored in NVS; configuration is build-time
  through `sdkconfig`.
- The SensMonitor client is currently built from source inside this repo. It
  can later be distributed as a precompiled component if needed.

## License

Licensed under the [Apache License 2.0](LICENSE).
