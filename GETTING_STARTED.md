# 🌊 KAIJU Getting Started Guide

**Complete setup, flashing, and attack workflow for Cappy's personal pentesting device**

---

## Table of Contents
1. [Before You Start](#before-you-start)
2. [Hardware Setup](#hardware-setup)
3. [Flashing Kaiju](#flashing-kaiju)
4. [First Boot & Configuration](#first-boot--configuration)
5. [Loading Attack Scripts](#loading-attack-scripts)
6. [Attack Workflows](#attack-workflows)
7. [Troubleshooting](#troubleshooting)

---

## Before You Start

**⚖️ Legal Reminder:**
- Only attack networks/devices you own or have explicit permission to test
- Jamming, deauth attacks, and unauthorized RF transmission are **illegal** in most jurisdictions
- CTF/pentest? Get written authorization from the target organization
- Party tricks? Make sure everyone at the party consents (even if they don't know it's happening)
- When in doubt, ask a lawyer

**🔧 What You'll Need:**
- M5Stack Cardputer ADV
- Hydra RF 424 (optional but recommended)
- USB-C cable
- Linux/Mac/Windows computer
- Python 3.6+ with `platformio` installed
- ~30 minutes of your time

---

## Hardware Setup

### 1. Unbox Kaiju

You should have:
- ✓ M5Stack Cardputer ADV (the black card-sized device)
- ✓ Hydra RF 424 (the cap that plugs on top) — *optional but highly recommended*
- ✓ Two antennas (for the Hydra)
- ✓ USB-C cable

### 2. Install the Hydra RF 424 (Optional)

**If you have a Hydra:**

1. **Power off** the Cardputer ADV (hold G0 button for 3+ seconds until shutdown)
2. **Remove any existing cap** if one is installed
3. **Align** the Hydra's header pins with the ADV's SPI/GPIO header (rear top of device)
4. **Press firmly** until the Hydra clicks into place (no tools needed, 18-pin header)
5. **Verify** both antennas are connected to the RF module
6. **Power on** by pressing G0

**If no Hydra yet:** Don't worry, Kaiju still works for WiFi/BLE/IR attacks.

### 3. Charge the Device

- Connect USB-C cable to the Cardputer ADV
- Charging LED should illuminate
- Takes ~30 minutes to fully charge
- Battery should give you 4-6 hours of hacking

---

## Flashing Kaiju

### Option A: Build from Source (Recommended)

**Install PlatformIO:**
```bash
pip install platformio
```

**Clone Kaiju:**
```bash
git clone https://github.com/CapSail/Kaiju.git
cd Kaiju
```

**Connect your Cardputer ADV via USB**

**Build & Flash:**
```bash
# Build the firmware
platformio run -e m5stack-cardputer

# Flash to device
platformio run -e m5stack-cardputer --target upload
```

The build takes 2-5 minutes. When complete, you should see:
```
Building .pio/build/m5stack-cardputer/firmware.bin
Uploading .pio/build/m5stack-cardputer/firmware.bin to /dev/ttyUSB0 (or /dev/tty.*)
Hard resetting via RTS pin...
```

**Device will reboot and show the Kaiju splash screen** 🦑

### Option B: Use Web Flasher

Coming soon at [bruce.computer/flasher](https://bruce.computer/flasher) — select "Kaiju" build when available.

### Option C: Flash Pre-Built Binary

If you have a `.bin` file:
```bash
pip install esptool
esptool.py --port /dev/ttyUSB0 write_flash 0x0 kaiju.bin
```

(Replace `/dev/ttyUSB0` with your device's serial port: `/dev/tty.usbmodem*` on Mac, `COM3` on Windows)

---

## First Boot & Configuration

### Boot Sequence

1. **Splash Screen** — You'll see "Kaiju" with "Cappy's Play" (70's nautical theme)
2. **Version Display** — Shows "Cap-ADV"
3. **Menu Loads** — Displays the main menu with 9 options:
   - WiFi
   - BLE
   - RF
   - Wireless (NRF24)
   - IR
   - RFID
   - Files
   - Others
   - Settings

### Initial Configuration

**First thing: Configure RF Frequency**

1. Press **DOWN** arrow to highlight **Settings**
2. Press **ENTER** to open Settings menu
3. Navigate to **RF Configuration** (or **Radio Settings**)
4. Set frequency to **433.05 MHz** (if using Hydra RF 424)
5. **SAVE** and return to main menu

**Optional: Customize Colors**

1. Settings → **Theme** or **Display Settings**
2. Primary Color: Already set to 70's burnt orange (0xFB60)
3. Background: Black (0x0000)
4. You can adjust these live

**Optional: Set Startup App**

1. Settings → **Startup App**
2. Select an app to auto-launch when device boots
3. Good choice: **WiFi** or **RF Scanner**

### Keyboard Controls

- **UP/DOWN**: Navigate menu
- **LEFT/RIGHT**: Sub-menu or adjust values
- **ENTER/OK**: Select
- **G0**: Back/cancel
- **Fn + Key**: Wake keyboard from sleep

---

## Loading Attack Scripts

### Method 1: Via Files Menu (Easiest)

1. Go to **Files** menu from main screen
2. Navigate to **scripts/** folder (if present on device)
3. Select a `.js` file to run
4. Script executes immediately
5. View output and logs

### Method 2: SD Card Copy (For Custom Scripts)

1. Connect your Cardputer ADV via USB to your computer
2. Copy scripts from `kaiju/scripts/` to the device's SD card:
   ```
   /SD/scripts/
   ├── 01_rf_scanner.js
   ├── 02_wifi_beacon_spam.js
   └── ...
   ```
3. Eject safely and disconnect
4. On device, go to **Files** → **scripts/** and select any script

### Method 3: Built-in Interpreter

Some scripts run natively through Bruce's **JavaScript interpreter**:

1. Go to **Others** menu
2. Select **Interpreter** or **Scripts**
3. Paste script code directly
4. Hit run

---

## Attack Workflows

### Workflow 1: Scan Your Local WiFi

**Goal:** Find all networks in range and their signal strength

**Steps:**
1. Select **WiFi** from main menu
2. Choose **Scan Networks** or **Site Survey**
3. Wait 10-30 seconds
4. Review list: SSID, BSSID, Channel, Signal (dBm)
5. Note the strongest and weakest signals

**Script:** `01_rf_scanner.js` (for RF) or WiFi scan built-in

---

### Workflow 2: Confuse Local WiFi

**Goal:** Spam fake networks to confuse WiFi scanners and devices

**Steps:**
1. Go to **WiFi** menu
2. Select **Beacon Spam** or **Fake Network Broadcast**
3. Watch as fake networks appear on nearby devices:
   - "FreePublicWiFi"
   - "ATT_WiFi"
   - "Starbucks_WiFi"
   - etc.
4. Users will see these when scanning for networks
5. Press any key to stop

**Script:** `02_wifi_beacon_spam.js` runs for ~30 seconds

**Party Trick:** Have your friend scan for networks — they'll see hilarious fake ones and be confused

---

### Workflow 3: Deauth Attack (Against Your Own Network!)

**Goal:** Force a device off WiFi (test your router's resilience)

**⚠️ ONLY DO THIS ON YOUR OWN NETWORK⚠️**

**Steps:**
1. Go to **WiFi** menu
2. Select **Deauth** or **Disconnect Clients**
3. Choose target:
   - Specific device (requires BSSID)
   - All devices on network (broadcast deauth)
4. Set duration (e.g., 10 seconds)
5. Devices will disconnect
6. They'll auto-reconnect when attack stops

**Script:** `05_wifi_auto_deauth.js`

**Reality Check:** Modern routers have deauth protection. This often doesn't work. Evil Portal is better.

---

### Workflow 4: Find Bluetooth Devices

**Goal:** Locate all BLE devices in range

**Steps:**
1. Go to **BLE** menu
2. Select **Device Scan** or **BLE Discovery**
3. Wait 10-20 seconds for full scan
4. View results:
   - Device name / address
   - Signal strength (RSSI)
   - Services (if available)

**Script:** `03_ble_device_finder.js`

**What You'll Find:**
- iPhones, Androids, smartwatches
- Wireless headphones, speakers
- Fitness trackers, smart home devices
- Beacons and tags

---

### Workflow 5: RF 433 MHz Signal Hunt

**Goal:** Find active radio transmissions on ISM band

**Steps:**
1. Go to **RF** menu
2. Select **Scan** or **Frequency Sweep**
3. Device scans 433.05, 433.50, 434.79, etc.
4. Returns any detected signals:
   - Frequency
   - Signal strength (dBm)
   - Modulation (if detectable)
5. Note strongest frequency

**Script:** `08_rf_frequency_hop.js`

**What You'll Find:**
- Garage door openers
- Wireless doorbells
- RC car remotes
- Wireless weather stations
- Tire pressure monitors

**Next:** Use **RF Record** to capture the signal, then **RF Replay** to re-broadcast it

---

### Workflow 6: NRF24 Packet Capture

**Goal:** Sniff 2.4 GHz wireless keyboard/mouse traffic

**Steps:**
1. Go to **Wireless** (or **NRF24**) menu
2. Select **Packet Sniff** or **Packet Capture**
3. Choose channel (37-50 are common for keyboards)
4. Let it capture for 30-60 seconds
5. View captured packets:
   - Source/destination addresses
   - Packet length
   - Signal strength

**Script:** `04_nrf24_sniffer.js` or `11_nrf24_keyboard_monitor.js`

**What You're Capturing:**
- Logitech / Microsoft wireless keyboards
- Wireless mouse clicks
- Other 2.4 GHz ISM devices
- NOT Bluetooth (different protocol)

---

### Workflow 7: Environmental Full Scan

**Goal:** Get complete picture of all wireless activity around you

**Steps:**
1. Load script: `10_environmental_scanner.js`
2. Script runs and returns:
   - WiFi networks nearby
   - BLE devices nearby
   - RF (433 MHz) activity
   - NRF24 (2.4 GHz) activity
   - Signal strengths for all
3. Review the full report
4. Decide which vector to attack next

**Result:** A complete wireless threat assessment of your area

---

## Running Multiple Attack Scripts

Kaiju includes 20 attack scripts. Here's a sampling:

| Script | What It Does | Good For |
|--------|------------|----------|
| 01 | RF 433 MHz scanner | Find active signals |
| 02 | WiFi beacon spam | Confuse nearby devices |
| 03 | BLE device finder | Locate Bluetooth devices |
| 04 | NRF24 sniffer | Capture wireless keyboard packets |
| 05 | WiFi deauth | Force devices off network |
| 06 | BLE spam | Flood Bluetooth scanners |
| 07 | Signal mapper | Find strongest WiFi signals |
| 08 | RF frequency hopper | Auto-scan ISM band |
| 09 | Auto WiFi connect | Try default passwords |
| 10 | Environmental scan | Full spectrum overview |
| 11 | Keyboard monitor | Watch for wireless input |
| 12 | IR blaster | Send TV/AC remote codes |
| 13 | Device fingerprinter | Identify device types |
| 14 | Rogue AP builder | Create fake hotspots |
| 15 | Combined attack | Multi-vector simultaneous |
| 16 | Emergency stop | Kill all transmissions |
| 17 | Packet logger | Record raw traffic |
| 18 | Channel analyzer | Find least congested WiFi |
| 19 | Social engineering | Analyze fake network tactics |
| 20 | Status report | Device capability report |

---

## Troubleshooting

### Device Won't Turn On
- Check USB-C connection
- Leave plugged in for 5 minutes (battery may be dead)
- Try holding G0 for 5 seconds

### Keyboard Not Responding
- Hold **Fn** + any key to wake keyboard from sleep
- If still dead, restart device (G0 for 5 sec)

### Hydra RF 424 Not Detected
- **Power off** device
- **Physically re-seat** the Hydra cap (remove and press firmly back in)
- **Power on** and check Settings → RF Config
- If still missing, the header connection might be damaged

### WiFi Attacks Not Working
- Router may have deauth protection (common in 2023+)
- Try evil portal instead
- Ensure you're testing your own network

### Scripts Won't Load
- Check files are in correct folder: `/scripts/`
- Verify file names have `.js` extension
- Scripts must be ES5 JavaScript (no arrow functions or let/const)
- Check [Bruce Interpreter Wiki](https://github.com/pr3y/Bruce/wiki/Interpreter) for API

### Device Gets Hot
- Normal during sustained RF transmission
- Reduce scan duration if device overheats
- Ensure antennas are properly connected (loose antennas cause power waste)

### USB Connection Issues
- Try different USB cable (many are charge-only)
- Try different computer
- On Linux: `lsusb` should show device
- On Mac: `ls /dev/tty.usbmodem*` should show port
- On Windows: Check Device Manager for COM port

---

## Safety & Legal

### ⚖️ Before You Attack

1. **Verify you own/control the target**
   - Your home WiFi? ✅ OK
   - Friend's WiFi? ⚠️ Get written permission
   - Public WiFi? ❌ Illegal without written authorization
   - Corporate network? ❌ Need signed penetration testing agreement

2. **Check local regulations**
   - USA: FCC regulations prohibit intentional RF jamming
   - EU: RTTE Directive governs unlicensed RF devices
   - Other countries: Vary widely — research your jurisdiction

3. **Understand the laws**
   - WiFi deauth attacks: Often illegal (jamming)
   - Packet sniffing: Legal if on your network
   - RF replay: Illegal if target device not yours
   - Evil portal / credential theft: Illegal without consent

### 🚨 Emergency Stop

If something goes wrong:
1. **Go to Scripts menu**
2. **Run `16_emergency_stop.js`**
3. All transmissions stop immediately
4. Device safe-resets

Or simply **hold G0 button for 5 seconds** to power off.

---

## Next Steps

1. **Set up your WiFi attack lab**
   - Create a test network with old router
   - Run attacks against it safely
   - Log results and learn

2. **Write custom scripts**
   - Check Bruce's [Interpreter Wiki](https://github.com/pr3y/Bruce/wiki/Interpreter)
   - Create automation for repetitive tasks
   - Share back to community

3. **Join the community**
   - [Bruce Discord](https://discord.gg/WJ9XF9czVT)
   - [CapSail GitHub](https://github.com/CapSail/Kaiju)
   - Ask questions, share tips

4. **Study wireless security**
   - [OWASP WiFi Security](https://owasp.org/www-community/attacks/)
   - [RTL-SDR Learning](https://www.rtl-sdr.com/)
   - [Flipper Zero Documentation](https://docs.flipper.net/)

---

## Need Help?

- **Scripts won't run?** → Check Bruce Interpreter docs
- **Hydra not working?** → See [Hydra RF 424 GitHub](https://github.com/pingequalab/cardputer-adv-hydra-rf)
- **Build errors?** → Check [Bruce Issues](https://github.com/pr3y/Bruce/issues)
- **General questions?** → Ask on [Bruce Discord](https://discord.gg/WJ9XF9czVT)

---

## Happy Hacking 🌊

You now have Kaiju. Use it wisely, responsibly, and legally. Break things you own. Learn how the world works. And most importantly — have fun doing it.

**Remember:** With great RF power comes great legal responsibility. 💛

---

*Built for Cap Sails. Cappy's Play indeed. 🌺*
