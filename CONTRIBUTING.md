# Contributing

Thanks for considering a contribution to SensMonitor IoT.

This project is firmware for cellular ESP32 IoT devices, so correctness is not
only about compiling. Hardware support claims should be conservative and
explicit.

## Development environment

Use ESP-IDF 5.5.4 for development and validation.

The recommended setup is:

- ESP-IDF 5.5.4
- ESP32 target
- Visual Studio Code with the Espressif IDF extension, or an equivalent
  command-line ESP-IDF setup

## Before opening a pull request

Run at least the default LilyGO build:

```sh
idf.py build
```

If the change affects board profiles, modem configuration, Kconfig defaults or
example configs, also run the generic ESP32 UART modem build:

```sh
idf.py -B /tmp/smonitor-iot-generic \
  -D SDKCONFIG=/tmp/smonitor-iot-generic-sdkconfig \
  -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;examples/generic_esp32_uart_modem/sdkconfig.defaults" \
  build
```

For documentation-only changes, a full firmware build is not required unless
the documentation changes example commands, configuration names or build
profiles.

## Hardware support terminology

Use these terms consistently:

- **Hardware-tested**: the full flow was tested on physical hardware, including
  build, flash, modem registration, PPP connectivity and telemetry.
- **Build-tested**: the firmware builds successfully for the profile, but the
  complete flow was not verified on physical hardware.
- **Planned**: listed as future work only.
- **Experimental**: implementation exists but is incomplete, unstable or not
  release-supported.

Do not mark a board, modem or sensor profile as supported unless its status is
documented in the README and backed by the appropriate test level.

## Modem profile changes

New modem profiles should include:

- model-specific AT initialization;
- PPP connection validation;
- network registration validation;
- GNSS behavior notes, if the modem supports GNSS;
- documentation updates;
- hardware test notes before the profile is marked as hardware-tested.

Placeholder modem profiles should not be exposed as selectable Kconfig options
unless they are clearly marked experimental and cannot be mistaken for
release-supported functionality.

## Board profile changes

New board profiles should document:

- modem UART pins;
- modem power/PWRKEY behavior;
- I2C pins and pull-up assumptions;
- battery measurement assumptions, if any;
- whether the board is hardware-tested or build-tested only.

Generic profiles should avoid implying universal ESP32 wiring. GPIO defaults
are examples and must be described as such.

## Dependency changes

Keep dependencies explicit and reproducible.

When updating `smonitor-modem` or `smonitor-i2c`, prefer tagged component
versions over raw commit hashes when a suitable tag exists. After changing
`main/idf_component.yml`, run:

```sh
idf.py update-dependencies
```

Commit the resulting `dependencies.lock` update together with the manifest
change.

## Release assets

Do not add firmware binary release assets until runtime provisioning for
operator and device-specific settings is available and documented. Source-only
releases are preferred while APN and device configuration require a local
firmware build.

## Documentation

Update README, CHANGELOG or ROADMAP when a change affects:

- supported hardware status;
- build or installation steps;
- public configuration options;
- release behavior;
- modem, board or sensor support claims.
