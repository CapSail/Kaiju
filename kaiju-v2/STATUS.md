# Kaiju v2 status

Build: `python -m platformio run -e cardputer-adv -t upload` from this folder.

## Have

| Step | Feature | Status |
|---|---|---|
| 1 | Splash, keyboard menu, back/enter | Verified on device |
| 2 | WiFi scan (SSID, dBm, channel) | Verified on device |
| 3 | WiFi tools menu, channel analyzer | Verified on device |
| 4 | Packet sniffer: live counts by frame type, hop or lock | Verified on device |
| 5 | Sniffer recording to SD as PCAP | Verified on device (records and saves). Wireshark open not yet confirmed |
| 6 | Beacon spam: funny, random, single + counter, custom SD list, Rick-roll names (our own lines). Random MAC per beacon, retries, channel hop | Built, awaiting device test |
| 7 | Deauth flood: list-wide check (16 s passive beacon sweep, channels 1-13) tags each network VULN (no PMF), PART (PMF optional), or PMF (required). Per-target 3 s check before running. Broadcast deauth via the AP interface | Built, awaiting device test. Test on your own AP with PMF off |
| 8 | Karma: hops channels 1-13 (H toggles to a fixed channel). Answers each probe on the channel it came from, plus a beacon for each name | Built, awaiting device test |


## Need

| Step | Feature | Notes |
|---|---|---|
| 10 | Handshake capture: pick an AP from the list, save its beacon and all EAPOL frames to /pcap/hs_*.pcap. Counts M1-M4 and shows when crackable. D toggles deauth to force reconnects | Built, awaiting device test |
| 11 | Deauth and beacon detection alerts on the sniffer | Defensive: flag a spike in deauth or unexpected beacons |
| 12 | BLE tools | Placeholder today |
| 13 | RF 433 MHz (CC1101) listen and spectrum | Port logic from Bruce (AGPL-3). Needs Hydra pin config |
| 14 | NRF24 2.4 GHz tools | Placeholder today. Hydra pin config |
| 15 | IR send and read | Placeholder today |
| 16 | RFID read | Placeholder today. Only tools that work with what's on the Hydra/Cardputer |
| 17 | Files (SD browser) | Placeholder today |
| 18 | Settings (brightness, theme) | Placeholder today |

## Reuse rules

- Check the license before copying code. Bruce is AGPL-3.0, so copied code stays AGPL-3.0. The repo is public, so that's satisfied.
- M5Cardputer is MIT.
- Marauder, Saturn, and Ghost ESP are not checked yet. Don't copy from them until their licenses are read.
- Verify every API against the 3.3.9 source or headers before flashing.
