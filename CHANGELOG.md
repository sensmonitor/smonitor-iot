# Changelog

All notable changes to this project will be documented in this file.

## v0.1.3 - LilyGO T-SIM7080G-S3 support

### Added

- Added LilyGO T-SIM7080G-S3 board support with SIM7080G Cat-M/NB-IoT modem.
- Added AXP2101 PMU initialization for modem and GNSS antenna power.
- Added ESP32-S3 target defaults for the current root development profile.
- Added explicit ESP32 target default for the LilyGO T-SIM7000G example.

### Changed

- Updated `sensmonitor/smonitor-modem` dependency to `0.1.5`.
- Switched default modem network selection to automatic for first bring-up.
- Updated T-SIM7080G-S3 UART and external I2C pin defaults.
- Documented separate ESP32 and ESP32-S3 build target expectations.

### Verified

- ESP-IDF 5.5.4 build with registry dependencies.
- Runtime telemetry flow on physical LilyGO T-SIM7080G-S3 hardware.
- Runtime regression check on physical LilyGO T-SIM7000G hardware.

### Notes

- T-SIM7080G-S3 battery telemetry is not implemented yet; firmware currently
  reports the fallback battery value when battery monitoring is disabled.
- Use automatic network mode for initial modem bring-up, then select NB-IoT or
  LTE-M explicitly if required by the SIM/operator/deployment.

## v0.1.1 - ESP Component Registry dependencies

### Changed

- Switched `smonitor-modem` and `smonitor-i2c` from Git URL dependencies to
  ESP Component Registry packages.
- Updated the dependency lock file to resolve `sensmonitor/smonitor-modem`
  `0.1.1` and `sensmonitor/smonitor-i2c` `0.1.1`.

### Verified

- ESP-IDF 5.5.4 default LilyGO build.
- ESP-IDF 5.5.4 generic ESP32 UART modem build.
- Runtime telemetry flow on physical LilyGO T-SIM7000G hardware.

### Notes

- No firmware API or hardware support changes.
- LilyGO T-SIM7000G remains the only hardware-tested board profile.

## v0.1.0 - Initial public release

### Added

- ESP-IDF 5.5.4 firmware for ESP32 cellular IoT devices.
- Hardware-tested LilyGO T-SIM7000G board profile with SIM7000G modem.
- Build-tested generic ESP32 UART modem board profile.
- SIM7000 modem profile with PPPoS connectivity over UART.
- Configurable NB-IoT, LTE-M and GPRS network mode support.
- GNSS support through the modem profile.
- BME280 sensor integration over I2C.
- Secure SensMonitor WebSocket telemetry client.
- Device serial derivation from the ESP32 factory eFuse MAC.
- Runtime device configuration request by serial number.
- Configurable sample interval, APN and modem authentication settings.
- Battery voltage monitoring for the LilyGO T-SIM7000G profile.
- Apache-2.0 license.
- GitHub Actions CI for ESP-IDF 5.5.4.

### Verified

- LilyGO T-SIM7000G full clean, build, flash and runtime test on physical
  hardware.
- ESP-IDF 5.5.4 default LilyGO build.
- ESP-IDF 5.5.4 generic ESP32 UART modem build.

### Notes

- LilyGO T-SIM7000G is currently the only hardware-tested board profile.
- The generic ESP32 UART modem profile is a configurable integration starting
  point. Its GPIO values are examples and must be adjusted for the target
  hardware.
- SIM7000 is the first supported modem profile. Additional modem profiles are
  planned but are not release-supported yet.
- A SensMonitor server account and registered device serial are required for
  the complete telemetry flow.
