# Kaiju handoff

Last updated: 2026-11-03

## Goal

Kaiju is an upgraded Bruce firmware for the M5Stack Cardputer ADV, paired with a Hydra RF 424 (CC1101 433 MHz and NRF24 2.4 GHz). It has two parts:

1. **Kaiju v1**: a fork of Bruce at the repo root. Trimmed menus, a 70s tan and beige theme, a boot splash reading "Kaiju / Cappy's Play", and only the working scripts.
2. **Kaiju v2**: a from-scratch firmware in `kaiju-v2/`. WiFi tools come first, and the rest of the Bruce feature set follows.

**Scope.** Active WiFi tools transmit only against networks the owner has marked as their own (`O` in the Deauth list, saved to `/kaiju/own_bssid.txt`). PMF-required networks are never attacked.

## Repo and access

- Remote: `https://github.com/CapSail/Kaiju.git`
- Branch: `ccr-797e4c88-p1o4x6`. The next session needs this repo attached with push access.
- Base: `origin/main`
- Bruce upstream sits at the repo root. Kaiju v2 lives in `kaiju-v2/`.
- `kaiju-v2/STATUS.md` is the feature table and device-test record.

## Toolchain

- The owner builds on Windows in PowerShell. Use `python -m platformio`, since `platformio` is not on PATH.
- PlatformIO Core 6.1.19 or newer. pioarduino needs it.
- Serial port: COM7.
- **Bruce (root):** `python -m platformio run -e m5stack-cardputer -t upload`. The root `platformio.ini` pins the upstream pioarduino platform and libs tarball, and FastLED 3.10.3. FastLED 3.10.4 and newer fail on `FP` because of `-DFP=1`. `patch.py` calls `python -m platformio pkg exec`.
- **Kaiju v2:** from `kaiju-v2/`, run `python -m platformio run -e cardputer-adv -t upload`. Its `platformio.ini` sets `boards_dir = ../boards/_boards_json` under `[platformio]`, `board_build.variants_dir = variants`, and uses its own `variants/m5stack-cardputer/pins_arduino.h` with no keyboard table and no `KEY_OPT`. Library: M5Cardputer at commit `bf64d3178ab43928fa02365fbe3937cec99716ad`.
- **Sandbox note:** in the cloud session, PlatformIO could not download framework packages. TLS verification fails through the proxy, and `REQUESTS_CA_BUNDLE` was not honored. Compile on the owner's PC.

## Hardware

- Keyboard via M5Cardputer. Up and down are `;` and `.` (or `W` and `S`). Enter selects. Backspace and Esc go back. The arrow keys only work with Fn.
- Display: 240x135, landscape.
- SD card: SCK 40, MISO 39, MOSI 14, CS 12. `SD.begin(12, SPI, 25000000)`.
- Hydra CC1101 and NRF24 pin configs are not written yet.

## SD card layout (Kaiju v2)

- `/kaiju/`: `own_bssid.txt`, `wardrive.csv`, `karma_allow.txt` (created by the owner, one MAC per line), `karma_log.txt`
- `/pcap/`: captures named `<prefix>_<seconds>.pcap`
- `/BruceEvilCreds/`: `<ssid>_creds.csv`, rotating to `_creds_2.csv` and so on at 200 lines
- `/PortalTemplates/`: optional HTML page for the evil portal (the first `.html` file is used)
- `/scripts/`: Bruce JavaScript files

Every log keeps the newest 200 lines.

## Kaiju v2: verified on device

Last device-tested build: commit `2159712e`.

- Splash and keyboard menu
- WiFi scan
- WiFi tools menu and channel analyzer
- Packet sniffer: live counts by frame type, and PCAP recording to SD (opening the PCAP in Wireshark is not confirmed)
- Wi-Fi beacon spam: funny, random, single, custom SD list, and Rick-roll names. Random MAC per beacon, channel hop. Basic run only.
- Deauth flood: the list-wide PMF check tags each network VULN, PART, or PMF. Per-target check before running. Basic run only, and it needs an own AP with PMF off.
- Karma: hosts the portal only for MACs in `karma_allow.txt`, for 90 seconds. Basic run only.
- Handshake capture: M1 to M4 counts and a crackable indicator. `D` toggles deauth. Basic run only.
- Evil portal: open network, DNS catch-all, captive redirect, SD template or built-in page, credentials saved to CSV. Viewer under WiFi > Portal creds. Basic run only.
- Sniffer twin-AP alert: verified. Deauth-flood alert: not tested.
- Kaijugotchi: the scan and the hunt-to-scan cycle are verified on device. The capture step is not tested.

## Built but not yet compiled or tested

- `974ab2bb`: Kaijugotchi review and test flow. It scans all networks for 60 seconds and shows a review list with each network's verdict. `O` marks the owner's networks, and Enter tests the marked ones, strongest first, with two tries each. PMF networks are skipped. Failed tests leave no PCAP and no log line.

## Still needs a test

- Kaijugotchi capture end to end. This needs a 2.4 GHz client that reconnects after a deauth. The owner's test phone was 5 GHz only.
- The review and test flow from `974ab2bb`.
- Failed-hunt cleanup (no PCAP left behind) and the two-tries limit.
- Manual handshake: the file is deleted on stop when the capture is not crackable.
- Karma log written only after a capture, and rotation at 200 lines.
- Creds rotation to `_creds_2.csv`.
- The 4 MB recording limit.
- The deauth-flood alert.
- Opening a sniffer PCAP in Wireshark.

## Kaiju v1 (Bruce fork): state

- Theme: `src/core/theme.h` sets `DEFAULT_PRICOLOR 0xD5B1` and `DEFAULT_SECCOLOR 0x9BCA` (tan and brown). Commit `9c1cd6ee`.
- Boot splash "Kaiju / Cappy's Play": `src/main.cpp`, boot screen. Commit `f3abac0d`.
- Menu trimmed to items that work. Commit `f3abac0d`.
- Scripts: 18 broken scripts removed. `20_status_report.js` and `23_kaiju_flasher.js` fixed to use only calls Bruce supports. Kept: `02_wifi_beacon_spam.js` and `03_ble_device_finder.js`. `19_social_engineering_generator.js` was removed by the owner (brand-impersonating SSID list, no device function). The kept scripts were not re-tested in this session.
- Build fixes committed: `patch.py` calls `python -m platformio`, and FastLED is pinned to 3.10.3.
- Device test status for v1 was not recorded in this session. Confirm it before relying on it.

## Next session: start here

1. Attach `CapSail/Kaiju` on branch `ccr-797e4c88-p1o4x6`.
2. Read `HANDOFF.md` and `kaiju-v2/STATUS.md`.
3. Compile and flash `kaiju-v2` at HEAD on the owner's PC. If the build fails, paste the first error.
4. Work through the "Still needs a test" list.

## Not started

- BLE: nothing in Kaiju v2. The scope has not been decided.
