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
| 8 | Karma: listens on all channels. Only devices whose MAC is in /kaiju/karma_allow.txt (one MAC per line) trigger it. An allowed probe hosts the evil portal under the requested name for 90 s, then returns to listening. Each host logged to /kaiju/karma_log.txt. No allowlist means nothing hosts | Built, awaiting device test. Phones need MAC randomization off for the test network |
| 9 | Handshake capture: pick an AP from the list, save its beacon and all EAPOL frames to /pcap/hs_*.pcap. Counts M1-M4 and shows when crackable. D toggles deauth to force reconnects | Built, awaiting device test |
| 10 | Evil portal: open network with the chosen SSID, DNS catches all names, captive redirect. Template: first .html in SD /PortalTemplates, else a plain built-in sign-in page. Each capture is saved to /BruceEvilCreds/<ssid>_creds.csv with session, SSID, channel, uptime, client IP, user agent, and browser time. Viewer: WiFi > Portal creds. No password checking against the real AP | Built, awaiting device test |
| 11 | Sniffer alerts: deauth or disassoc frames above 20 in 10 s, and a second access point seen under an already-seen name (possible evil twin). Shown on the sniffer screen | Built, awaiting device test. Self-test: beacon spam from this device should trigger the twin alert |

## Need

| Step | Feature | Notes |
|---|---|---|
| 12 | BLE tools | Placeholder today |
| 13 | RF 433 MHz (CC1101) listen and spectrum | Port logic from Bruce (AGPL-3). Needs Hydra pin config |
| 14 | NRF24 2.4 GHz tools | Placeholder today. Hydra pin config |
| 15 | IR send and read | Placeholder today |
| 16 | RFID read | Placeholder today. Only tools that work with what's on the Hydra/Cardputer |
| 17 | Files (SD browser) | Placeholder today |
| 18 | Settings (brightness, theme) | Placeholder today |

## Limits

- Logs keep the newest 200 entries. Karma log and per-SSID creds files rotate (creds go to _creds_2.csv, _creds_3.csv, and so on) so nothing is dropped silently.
- Packet recording stops at 4 MB and says so on screen.
- Viewer lists up to 40 files and shows the newest 60 entries of a file.

## Reuse rules

- Check the license before copying code. Bruce is AGPL-3.0, so copied code stays AGPL-3.0. The repo is public, so that's satisfied.
- M5Cardputer is MIT.
- Marauder, Saturn, and Ghost ESP are not checked yet. Don't copy from them until their licenses are read.
- Verify every API against the 3.3.9 source or headers before flashing.
