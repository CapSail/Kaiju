// Kaiju Proximity
// Counts the phones (Bluetooth) and Wi-Fi networks around you, sorted by
// signal strength: close, near, far. Signal strength is a rough guide, not a
// distance. Walls and bodies change it a lot.
//
// Privacy: nothing is saved to the card, and names or addresses are never
// shown. The Kaiju sleeps when nothing is close and wakes when something is.
// Esc = quit.

var display = require('display');
var keyboard = require('keyboard');
var ble = require('ble');
var wifi = require('wifi');

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

// Your own devices' addresses, lowercase with colons, so they don't count.
// Example: var OWN = ['a4:cf:12:34:56:78'];
var OWN = [];

// Memory only: address -> { rssi (smoothed), seen (ms) }
var devices = {};
var nets = {};
var cycle = 0;

function bucket(rssi) {
  if (rssi >= CLOSE_DBM) return 0;
  if (rssi >= NEAR_DBM) return 1;
  return 2;
}

function isOwn(addr) {
  for (var i = 0; i < OWN.length; i++) {
    if (OWN[i] === addr) return true;
  }
  return false;
}

function remember(map, key, rssi, t) {
  var e = map[key];
  if (e === undefined) {
    map[key] = { rssi: rssi, seen: t };
  } else {
    // smooth so one loud or quiet reading doesn't jump the count
    e.rssi = e.rssi * 0.6 + rssi * 0.4;
    e.seen = t;
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
    if (isOwn(addr)) continue;
    remember(devices, addr, found[i].rssi, t);
  }
  prune(devices, t);
}

function scanWifi(t) {
  var list = wifi.scan();
  for (var i = 0; i < list.length; i++) {
    var mac = String(list[i].MAC).toLowerCase();
    if (isOwn(mac)) continue;
    remember(nets, mac, list[i].RSSI, t);
  }
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
}

function main() {
  drawScreen([0, 0, 0], [0, 0, 0]);
  while (true) {
    if (keyboard.getEscPress()) break;
    var t = now();
    scanBle(t);
    if (cycle % WIFI_EVERY === 0) scanWifi(t);
    cycle++;
    drawScreen(countBuckets(devices), countBuckets(nets));
    delay(300);
  }
}

main();
