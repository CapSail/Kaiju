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
| 6 | Beacon spam: funny, random, single + counter, custom SD list, Rick-roll names (our own lines). Random MAC per beacon, retries, channel hop | Verified on device (basic run) |
| 7 | Deauth flood: list-wide check (16 s passive beacon sweep, channels 1-13) tags each network VULN (no PMF), PART (PMF optional), or PMF (required). Per-target 3 s check before running. Broadcast deauth via the AP interface. Press O on a row to mark it as yours (saved to /kaiju/own_bssid.txt, shown with *) | Verified on device (basic run). Test on your own AP with PMF off |
| 8 | Karma: listens on all channels. Only devices whose MAC is in /kaiju/karma_allow.txt (one MAC per line) trigger it. An allowed probe hosts the evil portal under the requested name for 90 s, then returns to listening. Only hosts that captured credentials are logged to /kaiju/karma_log.txt. No allowlist means nothing hosts | Verified on device (basic run). Phones need MAC randomization off for the test network |
| 9 | Handshake capture: pick an AP from the list, save its beacon and all EAPOL frames to /pcap/hs_*.pcap. Counts M1-M4 and shows when crackable. D toggles deauth to force reconnects. On stop, the file is kept only if crackable (M2 plus M1 or M3); otherwise it is deleted | Verified on device (basic run). Delete-on-stop not yet tested |
| 10 | Evil portal: open network with the chosen SSID, DNS catches all names, captive redirect. Template: first .html in SD /PortalTemplates, else a plain built-in sign-in page. Each capture is saved to /BruceEvilCreds/<ssid>_creds.csv with session, SSID, channel, uptime, client IP, user agent, and browser time. Viewer: WiFi > Portal creds. No password checking against the real AP | Verified on device (basic run) |
| 11 | Sniffer alerts: deauth or disassoc frames, 20 or more in 10 s, and a second access point seen under an already-seen name (possible evil twin). Shown on the sniffer screen | Twin alert verified on device. Deauth alert not yet tested (needs a second device sending deauth) |
| 12 | Kaijugotchi: scans all networks for 60 s (logs new ones to /kaiju/wardrive.csv), then opens a review list with each network's verdict (VULN, PART, PMF). Press O to mark your own networks. Enter tests only the marked ones, strongest first, with up to two tries each. Saves a PCAP only when a crackable handshake is caught. Failed tests leave no file | Scan and hunt-to-scan cycle verified on device earlier. The review and test flow is not tested yet. Capture not tested: the test phone is 5 GHz only and the Cardputer is 2.4 GHz only |

## Removed (WiFi-only build)

Removed on purpose: BLE, RF 433 MHz, NRF24, IR, RFID, Files, and Settings. The menu now holds only WiFi. The code is in commit `1540fe3` if it's ever needed again.

## Limits

- Logs keep the newest 200 entries. Wardrive log and Karma log trim their oldest lines. Per-SSID creds files rotate at 200 lines (_creds_2.csv, _creds_3.csv, and so on). After 50 files, the oldest creds file is deleted to make room (first in, first out), so the newest captures are always kept.
- Only useful data is saved. Kaijugotchi writes one wardrive line per new network and keeps a PCAP only for a crackable handshake. Failed hunts leave no file and no log line. Karma logs only hosts that captured credentials. A manual handshake capture is deleted on stop unless it is crackable.
- Packet recording stops at 4 MB and says so on screen.
- Viewer lists up to 50 files and shows the newest 60 entries of a file.

## Reuse rules

- Check the license before copying code. Bruce is AGPL-3.0, so copied code stays AGPL-3.0. The repo is public, so that's satisfied.
- M5Cardputer is MIT.
- Marauder, Saturn, and Ghost ESP are not checked yet. Don't copy from them until their licenses are read.
- Verify every API against the 3.3.9 source or headers before flashing.

## Not yet tested on device

- Kaijugotchi end to end: scout, hunt, capture, failed-hunt cleanup, and the two-hunt limit per network.
- Manual handshake delete-on-stop when the capture is not crackable.
- Karma log written only after a capture, and Karma log rotation at 200 entries.
- Creds rotation to _creds_2.csv.
- The 4 MB recording limit.
- The deauth-flood alert.
- Kaijugotchi capture, which needs a client on 2.4 GHz.
