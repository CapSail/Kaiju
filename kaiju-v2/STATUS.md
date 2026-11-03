# Kaiju v2 status

Build: `python -m platformio run -e cardputer-adv -t upload` from this folder.

## Have

| Step | Feature | Status |
|---|---|---|
| 1 | Splash, keyboard menu, back/enter | Verified on device |
| 2 | WiFi scan (SSID, dBm, channel) | Verified on device |
| 3 | WiFi tools menu, channel analyzer (passive, 13 channels, beacon counts) | Verified on device |
| 4 | Packet sniffer: live counts by frame type, hop or lock channel | Verified on device |
| 5 | Sniffer recording to SD as PCAP (256-byte snaplen) | Verified on device: records and saves. Wireshark open not yet confirmed |
| 6 | Beacon spam: funny names, random names, single name + counter, custom list from SD (/kaiju/ssids.txt). Random MAC per beacon, retries on full buffer, channel hop 1-11 option | Built, awaiting device test |

## Need

| Step | Feature | Notes |
|---|---|---|
| 7 | Deauth | The ESP-IDF doc for `esp_wifi_80211_tx` lists beacon, probe, action, and data frames only, so deauth is undocumented. Test on your own AP first, and watch it with the sniffer |
| 8 | Probe request flood | Probe frames are a documented type |
| 9 | Evil portal (captive portal: DNS and HTTP) | Larger: needs a DNS server and web server. Lab use only |
| 10 | Deauth and beacon detection alerts on the sniffer | Defensive: flag a spike in deauth or unexpected beacons |
| 11 | BLE tools | Placeholder today |
| 12 | RF 433 MHz (CC1101) listen and spectrum | Port logic from Bruce (AGPL-3). Needs Hydra pin config |
| 13 | NRF24 2.4 GHz tools | Placeholder today. Hydra pin config |
| 14 | IR send and read | Placeholder today |
| 15 | RFID read | Placeholder today. Only tools that work with what's on the Hydra/Cardputer |
| 16 | Files (SD browser) | Placeholder today |
| 17 | Settings (brightness, theme) | Placeholder today |

## Reuse rules

- Check the license before copying code. Bruce is AGPL-3.0, so copied code stays AGPL-3.0. The repo is public, so that's satisfied.
- M5Cardputer is MIT.
- Marauder, Saturn, and Ghost ESP are not checked yet. Don't copy from them until their licenses are read.
- Verify every API against the 3.3.9 source or headers before flashing.
