# Roadmap

This roadmap describes the planned direction for SensMonitor IoT after the
`v0.1.0` initial public release. Items listed here are planned work, not
release-supported features, until they are documented as supported in the
README and verified through CI and, where relevant, physical hardware tests.

## Current baseline

- ESP-IDF 5.5.4 is the supported build target.
- LilyGO T-SIM7000G with SIM7000G is the first hardware-tested board profile.
- Generic ESP32 UART modem is a build-tested configurable starting point.
- SIM7000 is the first supported modem profile.
- BME280 is the first supported I2C sensor profile.
- `smonitor-modem` and `smonitor-i2c` are consumed as tagged `v0.1.0`
  components.

## v0.1.x maintenance

- Keep ESP-IDF 5.5.x CI green for the LilyGO and generic example builds.
- Improve LilyGO T-SIM7000G setup and troubleshooting documentation.
- Add practical troubleshooting notes for APN, SIM registration, PPP startup
  and weak cellular signal cases.
- Document tested carrier/network mode notes when they are verified on real
  hardware.
- Keep dependency versions explicit and reproducible.

## v0.2.0 direction

- Add runtime provisioning for APN and device-specific settings.
- Prepare optional firmware binary release assets after provisioning is safe
  and documented.
- Improve generic ESP32 UART modem profile documentation and wiring examples.
- Add a clearer hardware test matrix for supported, build-tested and planned
  profiles.

## Planned modem profiles

These modem families are candidates for future support. They are not currently
release-supported:

- SIM7600
- SIM7080 / SIM7070
- A7670
- Quectel BG95 / BG96

New modem profiles should not be marked as supported until PPP connectivity,
basic network registration and telemetry flow are verified.

## Planned board profiles

- Additional LilyGO cellular boards.
- Generic ESP32 + SIMCom UART modem wiring examples.
- Generic ESP32 + Quectel UART modem wiring examples.

Board profiles should clearly state whether they are hardware-tested,
build-tested only, or planned.

## Community and release hygiene

- Add GitHub issues for the next modem and provisioning milestones.
- Add contribution guidelines before accepting larger external changes.
- Keep release notes explicit about hardware-tested versus build-tested
  support.
