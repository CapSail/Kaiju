# Kaiju v2 status

Build: `python -m platformio run -e cardputer-adv -t upload` from this folder.

## Have

| Step | Feature | Status |
|---|---|---|
| 1 | Splash, keyboard menu, back/enter | Verified on device |
| 2 | WiFi scan (SSID, dBm, channel) | Verified on device |
| 3 | WiFi tools menu, channel analyzer (passive, 13 channels, beacon counts) | Built, awaiting device test |

## Need

| Step | Feature | Notes |
|---|---|---|
| 4 | Packet sniffer (frame counts by type, log to SD) | Passive. Needs SD pin setup |
| 5 | Active WiFi: deauth, beacon spam, probe, evil portal | Verify `esp_wifi_80211_tx` against 3.3.9 headers first. Lab use only |
| 6 | BLE tools | Placeholder today |
| 7 | RF 433 MHz (CC1101) listen and spectrum | Port logic from Bruce (AGPL-3). Needs Hydra pin config |
| 8 | NRF24 2.4 GHz tools | Placeholder today. Hydra pin config |
| 9 | IR send and read | Placeholder today |
| 10 | RFID read | Placeholder today. Only tools that work with what's on the Hydra/Cardputer |
| 11 | Files (SD browser) | Placeholder today |
| 12 | Settings (brightness, theme) | Placeholder today |

## Reuse rules

- Check the license before copying code. Bruce is AGPL-3.0, so copied code stays AGPL-3.0. The repo is public, so that's satisfied.
- M5Cardputer is MIT.
- Marauder, Saturn, and Ghost ESP are not checked yet. Don't copy from them until their licenses are read.
- Verify every API against the 3.3.9 source or headers before flashing.
