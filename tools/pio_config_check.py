# PlatformIO pre-script: reminds users to configure the mobile operator APN.
#
# - Creates sdkconfig.defaults.local from the example on the first build.
# - Prints a warning after every build while the APN is empty.
# - Stops "upload" while the APN is empty, because the firmware cannot connect
#   without it. Set SMONITOR_ALLOW_EMPTY_APN=1 to upload anyway, for example to
#   read the device serial number before the SIM card is ready.
#
# Build-only runs (CI) are never blocked.

import os
import re
import shutil

Import("env")  # noqa: F821  (provided by PlatformIO/SCons)

PROJECT_DIR = env.subst("$PROJECT_DIR")  # noqa: F821
PIOENV = env.subst("$PIOENV")  # noqa: F821
LOCAL_DEFAULTS = os.path.join(PROJECT_DIR, "sdkconfig.defaults.local")
LOCAL_EXAMPLE = os.path.join(PROJECT_DIR, "sdkconfig.defaults.local.example")
SDKCONFIG = os.path.join(PROJECT_DIR, "sdkconfig.%s" % PIOENV)

APN_RE = re.compile(r'^CONFIG_SMONITOR_MODEM_APN="(.*)"\s*$', re.MULTILINE)


def banner(lines):
    width = max(len(line) for line in lines) + 4
    print("\n" + "*" * width)
    for line in lines:
        print("* " + line.ljust(width - 4) + " *")
    print("*" * width + "\n")


def configured_apn():
    """Return the APN from the active sdkconfig.<env>, or None if not generated yet."""
    if not os.path.isfile(SDKCONFIG):
        return None
    with open(SDKCONFIG, encoding="utf-8") as fp:
        match = APN_RE.search(fp.read())
    return match.group(1) if match else ""


def empty_apn_lines():
    return [
        "SensMonitor: the mobile operator APN is empty.",
        "The device will not connect to the cellular network without it.",
        "",
        "1. Set CONFIG_SMONITOR_MODEM_APN=\"your-apn\" in sdkconfig.defaults.local",
        "2. Delete sdkconfig.%s so it is regenerated from the defaults" % PIOENV,
        "3. Build and upload again",
        "",
        "Or set it directly: pio run -e %s -t menuconfig" % PIOENV,
        "(SensMonitor IoT > Mobile network APN)",
    ]


if not os.path.isfile(LOCAL_DEFAULTS) and os.path.isfile(LOCAL_EXAMPLE):
    shutil.copyfile(LOCAL_EXAMPLE, LOCAL_DEFAULTS)
    banner([
        "SensMonitor: created sdkconfig.defaults.local from the example.",
        "Set your mobile operator APN in it (CONFIG_SMONITOR_MODEM_APN).",
        "The board pins come from the selected PlatformIO environment.",
    ])


def warn_after_build(source, target, env):
    if configured_apn() == "":
        banner(empty_apn_lines())


def block_upload_without_apn(source, target, env):
    if configured_apn() != "":
        return
    if os.environ.get("SMONITOR_ALLOW_EMPTY_APN") == "1":
        print("SensMonitor: uploading with an empty APN (SMONITOR_ALLOW_EMPTY_APN=1).")
        return
    banner(empty_apn_lines() + [
        "",
        "Upload stopped. To upload anyway: SMONITOR_ALLOW_EMPTY_APN=1 pio run -t upload",
    ])
    env.Exit(1)


env.AddPostAction("buildprog", warn_after_build)  # noqa: F821
env.AddPreAction("upload", block_upload_without_apn)  # noqa: F821
