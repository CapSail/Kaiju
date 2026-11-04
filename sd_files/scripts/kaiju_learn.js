// Kaiju Learn
// A short guide to each menu. The pages are built in, so this is the only file needed.
//   ; or , = up / previous page      . or / = down / next page
//   Select   = open a topic
//   Esc      = back to the topics, or quit from the topic list

var display = require('display');
var keyboard = require('keyboard');

var SW = display.width();
var SH = display.height();

// 70s palette
var cBg = display.color(210, 180, 140);
var cDark = display.color(40, 22, 10);
var cRust = display.color(181, 83, 42);

var TOPICS = [
  ['Wi-Fi', 'wifi'],
  ['BLE', 'ble'],
  ['Sub-GHz', 'subghz'],
  ['2.4GHz', '2ghz'],
  ['Infrared', 'infrared'],
  ['Files', 'files'],
  ['Apps', 'apps'],
  ['Scripts', 'scripts'],
  ['Clock', 'clock'],
  ['Tools', 'tools'],
  ['Settings', 'settings'],
  ['Kaiju extras', 'kaiju']
];

var PAGE_LINES = 9;

function firstKey() {
  var k = keyboard.getKeysPressed();
  return k.length > 0 ? k[0] : '';
}

function drawIndex(sel) {
  display.fill(cBg);
  display.setTextSize(1);
  display.setTextColor(cRust);
  display.drawString("LEARN", 8, 4);
  for (var i = 0; i < TOPICS.length; i++) {
    var y = 15 + i * 9;
    if (i === sel) {
      display.drawFillRect(4, y - 1, SW - 8, 9, cRust);
      display.setTextColor(cBg);
    } else {
      display.setTextColor(cDark);
    }
    display.drawString(TOPICS[i][0], 8, y);
  }
  display.setTextColor(cDark);
  display.drawString("Select = open   Esc = quit", 8, SH - 10);
}

var TEXT = {
  "wifi": "WI-FI\nConnect, scan, and test networks.\n\nConnect to Wifi\n  Join a network with its password.\nStart WiFi AP\n  Turn this device into a hotspot.\nTurn Off WiFi\n  Stop all Wi-Fi.\nAP info\n  Details about the network you're on.\nScan WiFi Networks\n  List networks in range.\nScan Hosts\n  List devices on your network.\nWardriving\n  Log nearby Wi-Fi and Bluetooth.\nChannel Analyzer\n  See how busy each Wi-Fi channel is.\nSniffer\n  Capture Wi-Fi packets to a file.\nJam Detect\n  Check for Wi-Fi jamming nearby.\nKaijugotchi\n  A pet that watches nearby networks.\n  It deauths only networks listed in\n  /BrucePCAP/owned_aps.txt.\nEvil Portal\n  A fake login page. Your own tests\n  only.\nNetCut\n  Cut a device off a network you run.\nWifi Atks\n  Wi-Fi attacks. Only on your\n  networks.\nResponder, SSH, TelNET, SOCKS4 Proxy,\nListen TCP, Client TCP, Wireguard\n  Network tools for testing\n  connections.\nWiFi Pass Recovery\n  Recover saved Wi-Fi passwords here.\nConfig\n  Wi-Fi settings.",
  "ble": "BLUETOOTH (BLE)\nScan, send, and test Bluetooth.\n\nBLE Scan\n  List nearby Bluetooth devices.\nBLE Keyboard\n  Use the Cardputer as a Bluetooth\n  keyboard.\nBad BLE\n  Run keyboard scripts over Bluetooth.\nBLE Sniffer\n  Watch Bluetooth packets.\nBLE Spam\n  Fake device pop-ups. Test your own\n  devices only.\nBLE Suite\n  A collection of Bluetooth tools.\niBeacon\n  Broadcast as an iBeacon.\nMedia Cmds\n  Send play, pause, and volume keys.\nPresenter mode\n  Act as a slide clicker.\nNinebot\n  Ninebot scooter tools.\nDisconnect\n  Drop the current Bluetooth link.",
  "subghz": "SUB-GHZ RADIO\nNeeds the CC1101 radio on your Hydra\ncap.\n\nScan/copy\n  Find a signal and save it.\nListen\n  Hear signals on one frequency.\nRecord RAW\n  Record a raw signal to replay later.\nCustom SubGhz\n  Replay a saved signal.\nBruteforce\n  Try codes on a remote. Your own gear\n  only.\nJammer\n  Blocks a frequency. Illegal in most\n  places. Don't use it.\nRSSI Spectrum, Spectrum, Spectogram,\nSquareWave Spec\n  Graphs of signal strength.\nRF Frequency, RF Module, RF TX Pin,\nRF RX Pin\n  Radio settings.",
  "2ghz": "2.4GHz RADIO\nNeeds the NRF24 radio on your Hydra\ncap.\n\nMouseJack\n  Check wireless mice and keyboards\n  for\n  weak pairing. Your own gear only.\nNRF Jammer\n  Jams 2.4 GHz. Illegal in most\n  places.\n  Don't use it.\nSpectrum\n  Shows busy 2.4 GHz channels.\nNRF24 (legacy), NRF24 (shared SPI)\n  Two ways to talk to the radio. Pick\n  the one that works with your module.\nInformation\n  Radio details.\nConfig pins\n  Set the radio's pins.",
  "infrared": "INFRARED (IR)\nUses the IR light on the device.\n\nIR Read\n  Capture a remote's button press.\nCustom IR\n  Replay a saved IR code.\nTV-B-Gone\n  Runs a list of TV power-off codes.\nIR Jammer\n  Blocks IR signals. Don't use it on\n  things you don't own.\nIr TX Repeats\n  How many times a code is sent.\nIr TX Pin, Ir RX Pin\n  Pin settings.\nConfig\n  IR settings.",
  "files": "FILES\nBrowse, copy, and share files.\n\nSD Card\n  Files on the microSD card.\nLittleFS\n  Files in the built-in storage.\nSend File, Recv File\n  Copy files to or from a computer.\nMass Storage\n  The card appears on your computer.\nWebUI\n  Manage files from a browser on the\n  same Wi-Fi.\nSend Cmds, Recv Cmds\n  Send or receive command files.\nConnect\n  Connect to another device.\n\nCaptured handshakes are in\n/BrucePCAP/handshakes. The Kaijugotchi\nCaptures option opens that folder.",
  "apps": "APPS (JAVASCRIPT)\nSmall programs stored on the SD card.\n\nLoad...\n  Pick an app and run it.\n\nApps live in the /scripts folder. Copy\na .js file there to add one. Folders\nshow as [ name ]. Open one for more.\n\nKaiju apps:\n  Stomp, Shooter, Pong, Proximity,\n  and Learn. See Kaiju extras.\n\nRead each app's notes first.\nSome apps use the radios.",
  "scripts": "SCRIPTS FOLDERS\nApps shows subfolders as [ name ].\nOpen one to find its scripts.\n\nUse these only on your own networks\nand devices, or where you have written\npermission to test.\n\n[ wifi ]  wifi_brute\n  Tries a list of passwords on one\n  network. Load a text file, one\n  password per line. Each try waits\n  up to 3 seconds. Esc stops it.\n  Scan lists WPA2 and WEP networks\n  only, so WPA3 networks won't show.\n  The whole list loads into memory,\n  so start with a short list.\n\n[ rf ]  rf_brute\n  Sends 256 codes on 433.92 MHz,\n  one every 0.2 s, about a minute.\n  Only fixed-code remotes respond.\n  Rolling-code remotes ignore it.\n  Any key stops it.\n\n[ ir ]  ir_brute\n  Sends 65,536 NEC codes, one every\n  0.2 s, over 3.5 hours.\n  Any key stops it.",
  "clock": "CLOCK\n\nClock\n  Shows the time.\nTimer\n  Counts down a timer you set.\nExit\n  Goes back to the main menu.\n\nThe clock uses the time set in\nSettings.",
  "tools": "TOOLS\nExtra tools that work over USB or\nsound.\n\nQRCodes\n  Make a QR code from text.\nMegalodon\n  A Bruce tool. See its on-screen\n  notes.\nMicrophone\n  Record sound or view its spectrum.\nBadUSB & HID\n  Run keyboard scripts over USB.\nUSB Keyboard\n  Type text through the USB cable.\nUSB Clicker\n  Act as a USB clicker.\nUSB U2F\n  Act as a security key. Testing only.",
  "settings": "SETTINGS\nLook, sound, and device options.\n\nUI Theme\n  Pick a look. Kaiju files live in\n  /themes on the SD card.\nUI Color\n  Pick a color preset.\nBrightness, Dim Time, Sleep\n  Screen settings.\nSound Volume, Audio Config\n  Sound settings.\nStartup App\n  Choose what opens at power on.\nHide/Show Apps\n  Hide menu items you don't use.\nNetwork Creds\n  Saved Wi-Fi logins.\nFactory Reset\n  Wipes settings. Careful.\nRestart, Power Off\n  Power options.\nAbout\n  Version info.",
  "kaiju": "KAIJU EXTRAS\nAdded to this build.\n\nKaijugotchi (Wi-Fi menu)\n  A pet that watches nearby networks.\n  Deauths only networks in\n  /BrucePCAP/owned_aps.txt.\nCaptures (Kaijugotchi)\n  Select on the Kaijugotchi screen,\n  then Captures to open handshakes.\nKaiju Stomp (Apps)\n  Run the city. Jump with ; or Select.\n  Breathe fire with /.\nKaiju Shooter (Apps)\n  Fly and burn the saucers. Prev/Next\n  to move, Select to breathe fire.\nKaiju Pong (Apps)\n  Bat the fireball past the rival.\n  Prev/Next to move, Select for menu.\nKaiju Proximity (Apps)\n  Counts nearby phones and Wi-Fi by\n  signal strength. Full MAC addresses\n  are never shown. M marks your own.\nKaiju Learn (Apps)\n  This guide."
};

function loadLines(key) {
  var text = TEXT[key];
  if (!text) text = 'No page for ' + key + '.';
  return text.split('\n');
}

function drawPage(title, lines, page) {
  display.fill(cBg);
  display.setTextSize(1);
  display.setTextColor(cRust);
  display.drawString(title, 8, 4);
  display.setTextColor(cDark);
  var start = page * PAGE_LINES;
  for (var i = 0; i < PAGE_LINES && start + i < lines.length; i++) {
    display.drawString(lines[start + i], 8, 18 + i * 11);
  }
  var pages = Math.ceil(lines.length / PAGE_LINES);
  display.drawString((page + 1) + "/" + pages + "  ; . = pages  Esc = back", 8, SH - 12);
}

function viewTopic(sel) {
  var title = TOPICS[sel][0];
  var lines = loadLines(TOPICS[sel][1]);
  var pages = Math.ceil(lines.length / PAGE_LINES);
  var page = 0;
  drawPage(title, lines, page);
  while (true) {
    if (keyboard.getEscPress()) return;
    var key = firstKey();
    if ((key === '.' || key === '/') && page < pages - 1) {
      page++;
      drawPage(title, lines, page);
    } else if ((key === ';' || key === ',') && page > 0) {
      page--;
      drawPage(title, lines, page);
    }
    delay(120);
  }
}

function main() {
  var sel = 0;
  drawIndex(sel);
  while (true) {
    if (keyboard.getEscPress()) break;
    if (keyboard.getSelPress()) {
      viewTopic(sel);
      drawIndex(sel);
    } else {
      var key = firstKey();
      if (key === ';' || key === ',') {
        sel = (sel + TOPICS.length - 1) % TOPICS.length;
        drawIndex(sel);
      } else if (key === '.' || key === '/') {
        sel = (sel + 1) % TOPICS.length;
        drawIndex(sel);
      }
    }
    delay(120);
  }
}

main();
