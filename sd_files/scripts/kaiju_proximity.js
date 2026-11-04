// Kaiju Proximity
// Counts the phones (Bluetooth) and Wi-Fi networks around you, sorted into
// close, near and far by signal strength. Signal strength is a rough guide,
// not a distance. Walls and bodies change it a lot.
//
//   Select  = mark mine: open the list and tick your own devices (hidden
//             from the counts). Marked devices are saved to
//             /scripts/ghosts.txt on the SD card when you press Esc.
//   Esc     = quit (or save and close the mark list)
//
// Privacy: devices are kept in memory only, except the ones you mark as
// yours. Names are shown only in the mark list, so you can tell which are
// yours. Addresses are never drawn in full.
// Note: many phones change their Bluetooth address from time to time, so a
// marked phone can reappear. Mark it again if that happens.

var display = require('display');
var keyboard = require('keyboard');
var ble = require('ble');
var wifi = require('wifi');
var storage = require('storage');

var SW = display.width();
var SH = display.height();

// 70s palette
var cBg = display.color(210, 180, 140);
var cDark = display.color(40, 22, 10);
var cBrown = display.color(74, 44, 23);
var cMustard = display.color(217, 162, 27);
var cRust = display.color(181, 83, 42);

var CLOSE_DBM = -55;    // at or above this: close
var NEAR_DBM = -70;     // at or above this (and below close): near, else far
var STALE_MS = 20000;   // forget a device or network not seen for 20 seconds
var BLE_SECS = 1;       // Bluetooth scan length
var WIFI_EVERY = 3;     // Wi-Fi scan every 3rd cycle, it's slower
var GHOST_FILE = '/scripts/ghosts.txt';
var MARK_ROWS = 8;      // rows on the mark list

// Memory only: counted devices and networks (marked ones are never counted).
var devices = {};       // address -> { rssi, seen }
var nets = {};          // BSSID -> { rssi, seen }
// Memory only: everything seen, for the mark list.
var seenBle = {};       // address -> { rssi, seen, name }
var seenNet = {};       // BSSID -> { rssi, seen, name }

var ghosts = loadGhosts();  // address -> label, from the SD card
var cycle = 0;

function loadGhosts() {
  var map = {};
  var text = '';
  try {
    text = storage.read(GHOST_FILE);
  } catch (e) {
    return map;  // no file yet
  }
  if (!text) return map;
  var lines = text.split('\n');
  for (var i = 0; i < lines.length; i++) {
    var line = lines[i].trim();
    if (line.length === 0 || line.charAt(0) === '#') continue;
    var bar = line.indexOf('|');
    var addr = bar >= 0 ? line.substring(0, bar) : line;
    var label = bar >= 0 ? line.substring(bar + 1) : 'mine';
    map[addr.toLowerCase()] = label;
  }
  return map;
}

function saveGhosts() {
  var out = '# Devices marked as yours. Hidden from Kaiju Proximity.\n';
  for (var k in ghosts) {
    out += k + '|' + ghosts[k] + '\n';
  }
  storage.write(GHOST_FILE, out);
}

function bucket(rssi) {
  if (rssi >= CLOSE_DBM) return 0;
  if (rssi >= NEAR_DBM) return 1;
  return 2;
}

function remember(map, key, rssi, t, name) {
  var e = map[key];
  if (e === undefined) {
    map[key] = { rssi: rssi, seen: t, name: name };
  } else {
    // smooth so one loud or quiet reading doesn't jump the count
    e.rssi = e.rssi * 0.6 + rssi * 0.4;
    e.seen = t;
    if (name) e.name = name;
  }
}

function prune(map, t) {
  for (var k in map) {
    if (t - map[k].seen > STALE_MS) delete map[k];
  }
}

function countBuckets(map) {
  var c = [0, 0, 0];
  for (var k in map) c[bucket(map[k].rssi)]++;
  return c;
}

function scanBle(t) {
  var found = ble.scan(BLE_SECS);
  for (var i = 0; i < found.length; i++) {
    var addr = String(found[i].address).toLowerCase();
    var name = String(found[i].name || '');
    if (name === '<unknown>') name = '';
    remember(seenBle, addr, found[i].rssi, t, name);
    if (ghosts[addr] === undefined) remember(devices, addr, found[i].rssi, t, name);
  }
  prune(seenBle, t);
  prune(devices, t);
}

function scanWifi(t) {
  var list = wifi.scan();
  for (var i = 0; i < list.length; i++) {
    var mac = String(list[i].MAC).toLowerCase();
    var ssid = String(list[i].SSID || '');
    remember(seenNet, mac, list[i].RSSI, t, ssid);
    if (ghosts[mac] === undefined) remember(nets, mac, list[i].RSSI, t, ssid);
  }
  prune(seenNet, t);
  prune(nets, t);
}

function bar(x, y, n, color) {
  var w = Math.min(n, 10) * 6;
  if (w > 0) display.drawFillRect(x, y, w, 8, color);
}

// A small Kaiju silhouette, the same shape as the game's. Awake eye when awake.
function drawKaiju(x, y, awake) {
  display.drawFillRect(x, y + 6, 26, 12, cDark);
  display.drawFillRect(x + 22, y, 12, 9, cDark);
  display.drawFillRect(x - 6, y + 10, 8, 4, cDark);
  display.drawFillRect(x + 4, y + 2, 3, 4, cDark);
  display.drawFillRect(x + 12, y + 2, 3, 4, cDark);
  display.drawFillRect(x + 4, y + 18, 5, 4, cDark);
  display.drawFillRect(x + 16, y + 18, 5, 4, cDark);
  if (awake) {
    display.drawFillRect(x + 28, y + 2, 2, 2, cMustard);
  } else {
    display.drawFillRect(x + 27, y + 3, 4, 1, cMustard);
  }
}

function drawColumn(x, title, c) {
  display.setTextColor(cDark);
  display.drawString(title, x, 24);
  display.setTextColor(cRust);
  display.drawString("close", x, 40);
  bar(x + 40, 42, c[0], cRust);
  display.setTextColor(cDark);
  display.drawString("near", x, 56);
  bar(x + 40, 58, c[1], cMustard);
  display.drawString("far", x, 72);
  bar(x + 40, 74, c[2], cBrown);
}

function drawScreen(bleC, wifiC) {
  display.fill(cBg);
  display.setTextSize(1);
  display.setTextColor(cDark);
  display.drawString("PROXIMITY", 8, 6);
  drawColumn(8, "Phones", bleC);
  drawColumn(124, "Wi-Fi", wifiC);

  var nearby = bleC[0] + bleC[1] + wifiC[0] + wifiC[1];
  drawKaiju(SW - 60, SH - 30, nearby > 0);
  display.setTextColor(cDark);
  display.drawString(nearby > 0 ? "something is close" : "all quiet", 8, SH - 14);
  display.drawString("Select = mark mine", 8, SH - 26);
}

function shorten(s, n) {
  return s.length > n ? s.substring(0, n) : s;
}

function tail(addr) {
  return addr.substring(addr.length - 5);
}

// Mark list: every device and network seen, strongest first.
function buildMarkList() {
  var items = [];
  for (var a in seenBle) {
    items.push({ addr: a, kind: 'BT', name: seenBle[a].name, rssi: seenBle[a].rssi });
  }
  for (var m in seenNet) {
    items.push({ addr: m, kind: 'AP', name: seenNet[m].name, rssi: seenNet[m].rssi });
  }
  items.sort(function (x, y) { return y.rssi - x.rssi; });
  for (var i = 0; i < items.length; i++) {
    var it = items[i];
    it.ghost = ghosts[it.addr] !== undefined;
    it.label = it.name ? it.name : it.kind + ' ' + tail(it.addr);
  }
  return items;
}

function toggleGhost(it) {
  if (ghosts[it.addr] !== undefined) {
    delete ghosts[it.addr];
  } else {
    ghosts[it.addr] = it.label;
    // drop it from the counts right away
    delete devices[it.addr];
    delete nets[it.addr];
  }
  it.ghost = ghosts[it.addr] !== undefined;
}

function drawMarkList(items, sel, top) {
  display.fill(cBg);
  display.setTextSize(1);
  display.setTextColor(cRust);
  display.drawString("MARK MINE", 8, 4);
  display.setTextColor(cDark);
  if (items.length === 0) {
    display.drawString("Nothing seen yet. Scan first.", 8, 30);
  }
  for (var i = 0; i < MARK_ROWS && top + i < items.length; i++) {
    var it = items[top + i];
    var y = 18 + i * 12;
    if (top + i === sel) {
      display.drawFillRect(4, y - 2, SW - 8, 11, cRust);
      display.setTextColor(cBg);
    } else {
      display.setTextColor(cDark);
    }
    var mark = it.ghost ? "[x] " : "[ ] ";
    display.drawString(mark + shorten(it.label, 16) + "  " + it.rssi, 8, y);
  }
  display.setTextColor(cDark);
  display.drawString("Select=tick  Esc=save", 8, SH - 12);
}

function firstKey() {
  var k = keyboard.getKeysPressed();
  return k.length > 0 ? k[0] : '';
}

function markScreen() {
  var items = buildMarkList();
  var sel = 0;
  var top = 0;
  drawMarkList(items, sel, top);
  while (true) {
    if (keyboard.getEscPress()) break;
    if (keyboard.getSelPress() && items.length > 0) {
      toggleGhost(items[sel]);
      drawMarkList(items, sel, top);
    }
    var key = firstKey();
    if ((key === ';' || key === ',') && sel > 0) {
      sel--;
      if (sel < top) top = sel;
      drawMarkList(items, sel, top);
    } else if ((key === '.' || key === '/') && sel < items.length - 1) {
      sel++;
      if (sel >= top + MARK_ROWS) top = sel - MARK_ROWS + 1;
      drawMarkList(items, sel, top);
    }
    delay(100);
  }
  saveGhosts();
}

function main() {
  drawScreen([0, 0, 0], [0, 0, 0]);
  while (true) {
    if (keyboard.getEscPress()) break;
    if (keyboard.getSelPress()) {
      markScreen();
      drawScreen(countBuckets(devices), countBuckets(nets));
      continue;
    }
    var t = now();
    scanBle(t);
    if (cycle % WIFI_EVERY === 0) scanWifi(t);
    cycle++;
    drawScreen(countBuckets(devices), countBuckets(nets));
    delay(300);
  }
}

main();
