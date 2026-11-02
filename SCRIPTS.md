# 🦑 KAIJU Attack Scripts

**20 ready-to-run JavaScript attack scripts for WiFi, BLE, RF, and NRF24 hacking**

All scripts are in the `scripts/` folder and run through Bruce's built-in **MQjs JavaScript interpreter**.

---

## Quick Reference

| # | Script Name | Category | Purpose | Duration |
|---|---|---|---|---|
| 01 | `rf_scanner.js` | RF | Scan 433 MHz for active signals | 30 sec |
| 02 | `wifi_beacon_spam.js` | WiFi | Create 8 fake networks | Continuous |
| 03 | `ble_device_finder.js` | BLE | Find Bluetooth devices | 10 sec |
| 04 | `nrf24_sniffer.js` | NRF24 | Capture 2.4 GHz packets | 35 sec |
| 05 | `wifi_auto_deauth.js` | WiFi | Force clients off networks | 10 sec |
| 06 | `ble_spam.js` | BLE | Flood with fake devices | 30 sec |
| 07 | `signal_strength_mapper.js` | WiFi | Map WiFi signal across channels | 30 sec |
| 08 | `rf_frequency_hop.js` | RF | Auto-scan ISM band | 30 sec |
| 09 | `quick_wifi_connect.js` | WiFi | Try default credentials | 15 sec |
| 10 | `environmental_scanner.js` | Multi | Full spectrum scan | 15 sec |
| 11 | `nrf24_keyboard_monitor.js` | NRF24 | Watch wireless keyboard packets | 30 sec |
| 12 | `ir_blaster_helper.js` | IR | Broadcast common IR codes | Instant |
| 13 | `device_fingerprinter.js` | Multi | Identify device types | 20 sec |
| 14 | `rogue_ap_builder.js` | WiFi | Create fake hotspots | Instant |
| 15 | `combined_attack_suite.js` | Multi | Multi-vector simultaneous | 30 sec |
| 16 | `emergency_stop.js` | Safety | Kill all transmissions | Instant |
| 17 | `packet_sniffer_logger.js` | Multi | Record all wireless traffic | 60 sec |
| 18 | `channel_analyzer.js` | WiFi | Find best WiFi channels | 30 sec |
| 19 | `social_engineering_generator.js` | Analysis | Analyze fake network tactics | Instant |
| 20 | `status_report.js` | Info | Show device capabilities | Instant |
| 21 | `motion_radar.js` | Radar | WiFi-based motion detection | 60 sec |
| 22 | `enhanced_motion_radar.js` | Radar | Multi-band motion (WiFi+RF+BLE) | 90 sec |
| 23 | `kaiju_flasher.js` | Utility | Build & flash firmware | ~5 min |

---

## How to Run Scripts

### Method 1: Via Files Menu
```
Files → scripts/ → [select script.js] → ENTER
```

### Method 2: Via Scripts Menu (if available)
```
Main Menu → Scripts → [select script] → ENTER
```

### Method 3: Direct Interpreter (Advanced)
```
Others → Interpreter → [paste code] → Run
```

---

## Script Details

### 01_rf_scanner.js
**Category:** RF (433 MHz)  
**Purpose:** Scan for active 433 MHz signals

```javascript
// Scans: 433.05, 433.50, 434.79, 434.00, 434.40, 434.45 MHz
// Returns: Frequencies with signal strength > -100 dBm
// Duration: ~30 seconds
```

**Output Example:**
```
RF 433 MHz Scanner
Frequencies checked: 6
Signals found: 2
Results:
  433.05 MHz: -67 dBm (FOUND)
  433.50 MHz: -55 dBm (FOUND)
```

**Use Case:** Detecting garage door openers, doorbells, RC toys, weather stations

**Next Step:** Use RF → Record to capture the signal, then RF → Replay to re-broadcast

---

### 02_wifi_beacon_spam.js
**Category:** WiFi  
**Purpose:** Broadcast fake network SSIDs

```javascript
// Creates fake networks:
// - FreePublicWiFi
// - NETGEAR_Guest
// - Linksys
// - TP-LINK_2G
// - ATT_WiFi
// - VerizonGuest
// - xfinitywifi
// - CableWiFi
// Duration: Runs until stopped (press any key)
```

**Output Example:**
```
WiFi Beacon Spam
Networks created: 8
Duration: continuous
Note: Press any key to stop
```

**Use Case:** 
- Prank friends — they'll see hilarious fake networks
- Test WiFi security tools
- Confuse WiFi network analyzers

**Party Trick:** Run this at a gathering and watch people's faces when they see "FBI_Surveillance_Van" appear in their network list

---

### 03_ble_device_finder.js
**Category:** BLE  
**Purpose:** Find all Bluetooth devices in range

```javascript
// 10-second BLE scan
// Returns: Device address, name, RSSI, number of services
```

**Output Example:**
```
BLE Scan
Duration: 10,000 ms
Devices found: 7
Results:
  FF:FF:AA:BB:CC:DD - "iPhone 15" (-45 dBm) - 5 services
  AA:BB:CC:DD:EE:FF - "AirPods Max" (-62 dBm) - 2 services
  ...
```

**Use Case:** 
- Inventory of Bluetooth devices around you
- Signal strength mapping
- Identify vulnerable devices (older BLE protocols)

---

### 04_nrf24_sniffer.js
**Category:** NRF24 (2.4 GHz)  
**Purpose:** Capture wireless keyboard/mouse packets

```javascript
// Scans channels: 0, 12, 25, 37, 50, 63, 76
// Captures non-Bluetooth 2.4 GHz traffic
// Duration: 35 seconds total (5 sec per channel)
```

**Output Example:**
```
NRF24 Packet Capture
Channels scanned: 7
Packets captured: 42
Results:
  Channel 37: FF:AA:BB:CC:DD - 8 bytes (-45 dBm)
  Channel 37: FF:AA:BB:CC:DD - 8 bytes (-48 dBm)
  ...
```

**Use Case:** 
- Identify wireless keyboards/mice on networks
- Analyze unencrypted wireless protocols
- Detect connected IoT devices using NRF24

**Advanced:** Captured packets can be analyzed with RF tools to detect vulnerabilities

---

### 05_wifi_auto_deauth.js
**Category:** WiFi  
**Purpose:** Force devices off WiFi networks

```javascript
// Sends deauth frames to target BSSID
// Kicks all connected clients
// Duration: 10 seconds
```

**Output Example:**
```
WiFi Deauth Attack
Target BSSID: FF:FF:FF:FF:FF:FF (broadcast all networks)
Clients deauthed: 3
Duration: 10,000 ms
Status: Complete
```

**⚠️ WARNING:**
- Only use on networks you own/control
- Deauth is a form of DoS/jamming in most countries — may be illegal
- Modern routers have protection — this often doesn't work

**Use Case:** 
- Test your own router's resilience
- Educational demonstration of WiFi vulnerability
- Authorized penetration testing (with written permission)

---

### 06_ble_spam.js
**Category:** BLE  
**Purpose:** Flood Bluetooth with fake advertisements

```javascript
// Broadcasts 5 fake BLE devices
// Sends advertisements for 30 seconds
// Interval: 100 ms between ads
```

**Output Example:**
```
BLE Advertisement Spam
Fake devices: 5
Packets sent: 1,500
Duration: 30 seconds
Status: Complete
```

**Fake Names:**
- Unknown_Device_1
- Pairing_Request
- Update_Available
- Connect_Now
- New_Device

**Use Case:** 
- Confuse Bluetooth scanners
- Test BLE jitter resistance
- Demonstrate BLE vulnerability
- Party trick: Watch nearby devices try to connect to fake ones

---

### 07_signal_strength_mapper.js
**Category:** WiFi  
**Purpose:** Map WiFi signal strength across channels

```javascript
// Tests: Channels 1, 6, 11, 13
// 10 samples per channel
// Calculates average RSSI
```

**Output Example:**
```
WiFi Signal Strength Map
Channels tested: 4
Results:
  Channel 1: -72 dBm (Medium)
  Channel 6: -55 dBm (Strong)
  Channel 11: -88 dBm (Weak)
  Channel 13: -65 dBm (Medium)
```

**Use Case:** 
- Find strongest WiFi channels for AP spoofing
- Identify signal propagation patterns
- Plan RF jammer placement (for authorized testing)

---

### 08_rf_frequency_hop.js
**Category:** RF  
**Purpose:** Auto-scan multiple ISM frequencies

```javascript
// Scans: 315.00, 433.05, 433.50, 434.00, 868.00, 915.00 MHz
// Duration: ~30 seconds
```

**Output Example:**
```
RF Frequency Hop
Frequencies checked: 6
Signals detected: 3
Strongest signal: 433.05 MHz
Results:
  315.00 MHz: -89 dBm (active)
  433.05 MHz: -45 dBm (active) ← STRONGEST
  434.00 MHz: -102 dBm (active)
```

**Use Case:** 
- Quick search for active RF transmitters
- Identify which frequencies are in use
- Prepare for deep RF analysis on active frequencies

---

### 09_quick_wifi_connect.js
**Category:** WiFi  
**Purpose:** Auto-connect with common default credentials

```javascript
// Tests default passwords against found networks:
// - password, 12345678, admin123, 123456
// - NETGEAR default, Linksys default
```

**Output Example:**
```
Default Credential Attempts
Networks scanned: 12
Successful connections: 2
Results:
  TP-LINK_2G / 12345678 (Connected)
  NETGEAR_Guest / (no password) (Connected)
```

**Use Case:** 
- Quick test of default credentials
- Identify poorly configured networks
- Educational demo of password reuse

---

### 10_environmental_scanner.js
**Category:** Multi-vector  
**Purpose:** Full spectrum environmental assessment

```javascript
// One-shot complete scan of all attack vectors
// WiFi + BLE + RF (433 MHz)
// Duration: ~15 seconds
```

**Output Example:**
```
Full Environmental Scan
WiFi networks: 12
BLE devices: 7
RF activity: Yes (433.05 MHz)
Complete report: [full data...]
```

**Use Case:** 
- Get complete picture before deciding which attack to use
- Situational awareness on arrival at new location
- Red team initial reconnaissance

---

### 11_nrf24_keyboard_monitor.js
**Category:** NRF24  
**Purpose:** Specifically target wireless keyboard traffic

```javascript
// Monitors common keyboard channels: 37, 38, 39
// Filters for keyboard-like packet patterns
// Duration: 30 seconds
```

**Output Example:**
```
NRF24 Keyboard Packet Capture
Channels monitored: 3
Keyboard packets: 24
Packets:
  Channel 37: [raw keyboard data]
  Channel 37: [raw keyboard data]
  ...
```

**Use Case:** 
- Target wireless keyboards specifically
- Analyze unencrypted keyboard protocols
- Identify vulnerable wireless input devices

---

### 12_ir_blaster_helper.js
**Category:** IR  
**Purpose:** Broadcast common IR codes

```javascript
// Sends IR codes for:
// TVs: Power, Volume, Mute, Channels
// AC units: Power, Temp Up/Down
// Lights: On/Off
```

**Output Example:**
```
IR Blaster Broadcast
Codes available: 11
Codes sent: 11
Targets: tv_power, tv_volume_up, tv_volume_down, ...
Status: Broadcast complete
```

**Use Case:** 
- Party trick: Turn off TV at gatherings
- Control devices around you
- Test IR receiver security

---

### 13_device_fingerprinter.js
**Category:** Multi-vector Analysis  
**Purpose:** Identify device types from network signatures

```javascript
// Analyzes WiFi SSIDs and BLE manufacturers
// Returns educated guesses on device types
```

**Output Example:**
```
Device Fingerprinting
Total devices: 15
Devices:
  "Galaxy_S24" → Samsung Phone
  "iPhone_WiFi" → Apple Phone
  "Alexa_Guest" → Amazon Alexa
  Unknown BLE device → Apple Device (manufacturer 0x004C)
  ...
```

**Use Case:** 
- Inventory what devices are nearby
- Identify attack surface by device type
- Plan targeted attacks

---

### 14_rogue_ap_builder.js
**Category:** WiFi  
**Purpose:** Create convincing fake WiFi networks

```javascript
// Builds 3 fake networks:
// - FreeAirport (open, channel 6)
// - Starbucks_WiFi (open, channel 11)
// - Public_Library (open, channel 1)
```

**Output Example:**
```
Rogue Access Point Creation
Networks created: 3
Networks:
  FreeAirport (Channel 6, Open)
  Starbucks_WiFi (Channel 11, Open)
  Public_Library (Channel 1, Open)
Status: Broadcasting
Warning: Educational only - verify legality
```

**Use Case:** 
- Create test environment
- Educational demonstration
- Authorized pentest setup

---

### 15_combined_attack_suite.js
**Category:** Multi-vector  
**Purpose:** Run multiple attacks simultaneously

```javascript
// Starts:
// 1. WiFi beacon spam
// 2. BLE advertisement spam
// 3. RF noise generation
// 4. Environmental scan
```

**Output Example:**
```
Combined Attack Suite
Attacks executed: 4
Log:
  WiFi Beacon Spam (Starting)
  BLE Advertisement Spam (Starting)
  RF Noise Generation (Starting)
  Environmental Scan (Complete)
Total duration: 30 seconds
```

**Use Case:** 
- Maximum chaos/interference
- Multi-vector red team exercise
- Stress-test WiFi/BLE/RF infrastructure

---

### 16_emergency_stop.js
**Category:** Safety  
**Purpose:** Disable all transmissions immediately

```javascript
// Stops: WiFi, BLE, RF, NRF24, IR
// Duration: Instant
```

**Output Example:**
```
EMERGENCY STOP
Transmitters stopped: 5
Stopped: WiFi, BLE, RF, NRF24, IR
Status: All transmissions disabled
Message: Device is now in safe mode
```

**Use Case:** 
- **ALWAYS RUN THIS if something goes wrong**
- Kill all attacks instantly
- Safe shutdown

---

### 17_packet_sniffer_logger.js
**Category:** Multi-vector Logging  
**Purpose:** Record all wireless packets for analysis

```javascript
// Captures WiFi + BLE packets simultaneously
// Logs to device storage
// Duration: 60 seconds
```

**Output Example:**
```
Packet Sniffer - Raw Capture
Duration: 60 seconds
Packets captured: 342
Log file: /packets_1635291847.txt
Sample logs: [first 10 packets...]
```

**Use Case:** 
- Save traffic for offline analysis
- Identify suspicious network behavior
- Forensic logging

---

### 18_channel_analyzer.js
**Category:** WiFi Analysis  
**Purpose:** Find least congested WiFi channels

```javascript
// Analyzes: Channels 1, 6, 11, 13
// Counts networks per channel
// Rates congestion level
```

**Output Example:**
```
WiFi Channel Analysis
Channels analyzed: 4
Results:
  Channel 1: 7 networks (High congestion)
  Channel 6: 2 networks (Low congestion) ← RECOMMENDED
  Channel 11: 5 networks (Medium congestion)
  Channel 13: 1 network (Low congestion) ← RECOMMENDED
```

**Use Case:** 
- Select best channels for your rogue AP
- Avoid crowded frequencies
- Optimize signal strength

---

### 19_social_engineering_generator.js
**Category:** Analysis  
**Purpose:** Analyze social engineering tactics

```javascript
// Examines fake network names
// Rates effectiveness against different users
```

**Output Example:**
```
Social Engineering Network Analysis
Fake networks: 5
Networks:
  AWS_Redacted → High effectiveness (corporate)
  Google-Guest → High effectiveness (Apple users)
  Hotel_Lobby_WiFi → High effectiveness (travelers)
  Apple_Network → High effectiveness (brand trust)
  Microsoft_Update_Center → High effectiveness (fake updates)
```

**Use Case:** 
- Understand user psychology
- Plan social engineering attacks
- Educational awareness training

---

### 20_status_report.js
**Category:** Device Info  
**Purpose:** Show Kaiju's complete capabilities

```javascript
// Returns: Device info, hardware specs, radio modules
// All supported attacks and features
```

**Output Example:**
```
Device: Kaiju
Firmware: Cap-ADV
Theme: 70s Nautical
Hardware: ESP32-S3 (8MB Flash, 8MB PSRAM)
Radio Modules:
  WiFi: 802.11b/g/n 2.4 GHz (Scan, Spam, Deauth, Sniffing)
  BLE: Bluetooth 5.0 (Scan, Spam, Attacks)
  RF: CC1101 433.05 MHz (Scan, Record, Replay)
  NRF24: 2.4 GHz (Sniffing, Keyboard Monitoring)
  IR: Record, Replay, Blasting
Available scripts: 20
Party trick readiness: Maximum
```

**Use Case:** 
- Verify all features working
- Show off Kaiju's capabilities
- Troubleshoot missing modules

---

## Custom Script Writing

Want to create your own? Check Bruce's [Interpreter Wiki](https://github.com/pr3y/Bruce/wiki/Interpreter).

**API Reference:**
- `wifi.scan(timeout)` — WiFi networks
- `ble.startScan(timeout)` — Bluetooth devices
- `rf.scan(frequency)` — RF signal strength
- `nrf24.sniff(channel, duration)` — NRF24 packets
- `ir.send(code)` — IR blasting
- `system.millis()` — Current time (ms)

**Example Custom Script:**
```javascript
// Find strongest WiFi signal
function findStrongest() {
    var nets = wifi.scan(5000);
    var strongest = null;
    var best_rssi = -200;
    
    for (var i = 0; i < nets.length; i++) {
        if (nets[i].rssi > best_rssi) {
            best_rssi = nets[i].rssi;
            strongest = nets[i];
        }
    }
    
    return strongest;
}

// Run it
findStrongest();
```

---

## Tips & Tricks

1. **Combine attacks** — Run beacon spam while sniffing packets to see how devices react
2. **Log everything** — Use packet sniffer during attacks for later analysis
3. **Test on your own setup first** — All attacks should be tested against your own networks
4. **Read the output carefully** — Scripts return JSON with detailed results
5. **Use emergency stop if needed** — `16_emergency_stop.js` kills everything

---

## Legal Reminder

All of these scripts are **legal to use on networks and devices you own or have explicit written permission to test**. Unauthorized attacks are illegal in most jurisdictions. Use responsibly. 🌊

---

---

## 21_motion_radar.js
**Category:** Motion Detection Radar  
**Purpose:** Detect movement through walls via WiFi signal variation

```javascript
// Monitors WiFi signal strength (RSSI) for motion
// How it works: People moving cause signal reflections to change
// Works through walls - signals bounce differently when obstacles move
// Duration: 60 seconds continuous scan
```

**Output Example:**
```
=== MOTION RADAR ===
Scanning for movement via WiFi signal variation...
Threshold: 5 dBm

[MOTION] 7.3 dBm change detected!
[MOTION] 8.1 dBm change detected!
[MOTION] 6.9 dBm change detected!

Results:
WiFi-Based Motion Radar
Duration: 60 seconds
Motion events: 23
Average intensity: 7.2 dBm
Proximity estimate: CLOSE
Baseline RSSI: -52 dBm
```

**Proximity Estimates:**
- **VERY CLOSE** (>20 dBm variation) = < 1 meter, direct line of sight
- **CLOSE** (10-20 dBm) = 1-3 meters, same room
- **NEARBY** (5-10 dBm) = 3-5 meters, adjacent room
- **DISTANT** (2-5 dBm) = 5+ meters, through walls
- **VERY DISTANT** (<2 dBm) = No clear motion

**Use Case:**
- Detect movement in adjacent rooms (through walls)
- Identify if someone is home
- Motion-triggered alerts
- Passive surveillance (authorized only)
- Ghost hunting (for fun)

**How It Works:**
1. Monitors WiFi signal strength from nearby router
2. Movement causes multipath reflections to change
3. RSSI variations indicate motion
4. No special hardware needed — uses WiFi already present

---

## 22_enhanced_motion_radar.js
**Category:** Advanced Motion Detection  
**Purpose:** Multi-band motion detection (WiFi + RF + BLE)

```javascript
// Uses THREE frequency bands simultaneously:
// - WiFi (2.4 GHz)
// - RF 433 MHz (Hydra CC1101)
// - BLE (2.4 GHz, separate from WiFi)
// More accurate through walls, higher confidence
```

**Output Example:**
```
=== ENHANCED MOTION RADAR (Multi-Band) ===
WiFi (2.4 GHz) + RF (433 MHz) + BLE Scanning...

[WiFi] Motion at 2.3s: 6.2 dBm
[RF 433] Motion at 2.5s: 7.1 dBm
[BLE] Motion at 2.7s: 5.8 dBm
[WiFi] Motion at 5.1s: 8.3 dBm

Results:
Enhanced Multi-Band Motion Radar
Duration: 90 seconds
Detection confidence: HIGH
Proximity estimate: CLOSE (1-3 meters)

Motion events:
  WiFi 2.4 GHz: 28 events
  RF 433 MHz: 15 events
  BLE 2.4 GHz: 12 events
  Total: 55 events

Intensity averages:
  WiFi: 6.8 dBm
  RF: 7.1 dBm
  BLE: 6.2 dBm
  Overall: 6.7 dBm
```

**Confidence Levels:**
- **HIGH** (>20 events) = Definite motion
- **MEDIUM** (10-20 events) = Probable motion
- **MEDIUM-LOW** (5-10 events) = Weak/distant motion
- **LOW** (<5 events) = No clear motion

**Use Case:**
- More accurate motion detection (3 bands = higher confidence)
- Better through-wall detection
- Distinguish motion from noise
- Proximity estimation (close/far/distant)
- Hydra RF 424 required for best results

**Advantage over Standard Radar:**
- Uses multiple frequency bands for cross-confirmation
- RF 433 MHz penetrates walls differently than WiFi
- BLE as independent 2.4 GHz source
- More reliable motion detection

---

## 23_kaiju_flasher.js
**Category:** Utility / Firmware Management  
**Purpose:** One-button build and flash Kaiju firmware

```javascript
// Automated firmware build and flash sequence
// Requires: platformio, USB connection to device
// Handles all steps automatically
```

**Output Example:**
```
╔════════════════════════════════════════╗
║     KAIJU FIRMWARE FLASHER            ║
║     Cappy's Play Edition              ║
╚════════════════════════════════════════╝

[1/5] Pre-flight checks...
      Checking requirements

[2/5] Building Kaiju firmware...
      Running: platformio run -e m5stack-cardputer
  Linking environment m5stack-cardputer
  Compiling src/main.cpp
  Compiling src/core/theme.h
  ✓ Build successful!

[3/5] Detecting Cardputer ADV...
  ✓ Device found!
  USB Port: /dev/ttyUSB0
  Board: m5stack-cardputer
  Chip: ESP32-S3

[4/5] Flashing Kaiju to device...
  Writing firmware.bin (0x0)...
  50% complete...
  100% complete!
  ✓ Verification successful!

[5/5] Rebooting device...
  Hard resetting via RTS pin...
  ✓ Device rebooting

╔════════════════════════════════════════╗
║        FLASH SUCCESSFUL! 🎉           ║
╚════════════════════════════════════════╝

Device is now running Kaiju firmware!
Splash screen: Kaiju / Cappy's Play
Theme: 70s Nautical Orange
```

**What It Does:**
1. **Pre-flight checks** — Verifies platformio & USB device
2. **Build** — Compiles firmware (2-5 min)
3. **Device detection** — Finds connected Cardputer ADV
4. **Flash** — Writes binary to device
5. **Reboot** — Restarts with new firmware

**Use Case:**
- One-button flash from Kaiju itself
- No command line needed
- Ideal for updating firmware on the device
- Perfect for testing modified builds

**Requirements:**
- platformio installed: `pip install platformio`
- USB cable connected (data-capable, not charge-only)
- Cardputer ADV powered on
- ~5 minutes

**Common Issues:**
- "Device not found" → Check USB cable, try different port
- "Build fails" → Check platformio installation
- "Permission denied" → May need `sudo` on Linux/Mac

---

**Built for Cap Sails. Happy hacking. 💛**
