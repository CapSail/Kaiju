# Kaiju v2 status

Build: `python -m platformio run -e cardputer-adv -t upload` from this folder.

## Have

| Step | Feature | Status |
|---|---|---|
| 1 | Splash, keyboard menu, back/enter | Verified on device |
| 2 | WiFi scan (SSID, dBm, channel) | Verified on device |
| 3 | WiFi tools menu, channel analyzer (passive, 13 channels, beacon counts) | Verified on device |
| 4 | Packet sniffer: live counts by frame type, hop or lock channel | Verified on device |
| 5 | Sniffer recording to SD as PCAP (Wireshark-readable, 256-byte snaplen) | Built, awaiting device test |

## Need

| Step | Feature | Notes |
|---|---|---|
| 6 | Active WiFi: deauth, beacon spam, probe, evil portal | Verify `esp_wifi_80211_tx` against 3.3.9 headers first. Lab use only |
| 7 | BLE tools | Placeholder today |
| 8 | RF 433 MHz (CC1101) listen and spectrum | Port logic from Bruce (AGPL-3). Needs Hydra pin config |
| 9 | NRF24 2.4 GHz tools | Placeholder today. Hydra pin config |
| 10 | IR send and read | Placeholder today |
| 11 | RFID read | Placeholder today. Only tools that work with what's on the Hydra/Cardputer |
| 12 | Files (SD browser) | Placeholder today |
| 13 | Settings (brightness, theme) | Placeholder today |

## Reuse rules

- Check the license before copying code. Bruce is AGPL-3.0, so copied code stays AGPL-3.0. The repo is public, so that's satisfied.
- M5Cardputer is MIT.
- Marauder, Saturn, and Ghost ESP are not checked yet. Don't copy from them until their licenses are read.
- Verify every API against the 3.3.9 source or headers before flashing.
