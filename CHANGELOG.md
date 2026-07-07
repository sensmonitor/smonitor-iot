# Changelog

All notable changes to this project will be documented in this file.

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
