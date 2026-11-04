// Kaiju Proximity: a spy gadget, with Kaiju as the host.
// Shows the phones (Bluetooth) and Wi-Fi networks around you: how many are
// close, near and far, and roughly how far away each one is.
// Distances are a rough guide from signal strength. Walls and bodies change
// them a lot, so read them as "about this far", not a measurement.
//
// Keys:
//   ; , . /   move the cursor up and down
//   m or Select  mark the highlighted target as yours (it leaves the list and
//                the counts). Marks are saved to /scripts/ghosts.txt
//   h         show or hide the targets you've marked
//   Esc       quit
//
// The top right shows scan counts and the last key read. If something goes
// wrong with a scan, the error shows at the bottom.
//
// Privacy: devices are kept in memory only, except the ones you mark as
// yours. Names show on screen so you can tell what's around you. Full
// addresses never appear. Many phones change their Bluetooth address from
// time to time, so a marked phone can reappear as a new entry. Mark it again
// if that happens.

var display = require('display');
var keyboard = require('keyboard');
var ble = require('ble');
var wifi = require('wifi');
var storage = require('storage');

var SW = display.width();
var SH = display.height();

// spy-dossier palette, with the 70s warmth kept in the Kaiju colors
var cNight = display.color(40, 22, 10);
var cGold = display.color(217, 162, 27);
var cFire = display.color(230, 120, 30);
var cHead = display.color(181, 83, 42);
var cTan = display.color(210, 180, 140);
var cCream = display.color(245, 230, 200);
var cDim = display.color(110, 84, 56);
var cHot = display.color(240, 70, 30);
var cBar = display.color(70, 46, 26);

var CLOSE_DBM = -55;    // at or above this: close
var NEAR_DBM = -70;     // at or above this (and below close): near, else far
var STALE_MS = 20000;   // forget a target not seen for 20 seconds
var BLE_SECS = 1;       // Bluetooth scan length
var WIFI_EVERY = 3;     // Wi-Fi scan every 3rd cycle, it's slower
var GHOST_FILE = '/scripts/ghosts.txt';
var LIST_TOP = 56;      // first row of the target list
var ROW_H = 9;
var LIST_ROWS = 7;

// Kaiju's head, host of the show. Each run is [x, y, width] in a 96x88 box.
var HEAD_X = 130;
var HEAD_Y = 14;
var HEAD_RUNS = [17,5,2,17,6,3,18,7,4,18,8,6,19,9,6,20,10,7,20,11,9,21,12,9,21,13,11,22,14,12,7,15,3,22,15,13,8,16,4,23,16,16,9,17,6,23,17,22,10,18,7,24,18,25,11,19,9,25,19,27,12,20,10,25,20,30,13,21,45,13,22,48,14,23,50,15,24,51,16,25,53,3,26,1,17,26,54,4,27,5,18,27,56,5,28,9,19,28,57,6,29,73,8,30,73,9,31,74,10,32,74,11,33,75,13,34,75,14,35,75,15,36,75,16,37,75,15,38,76,7,39,84,3,40,88,5,41,85,8,42,82,10,43,79,12,44,77,15,45,72,16,46,68,17,47,64,17,48,61,17,49,58,17,50,56,17,51,54,17,52,52,18,53,49,18,54,47,18,55,45,18,56,43,19,57,41,20,58,38,21,59,36,22,60,33,23,61,31,24,62,28,25,63,26,28,64,21,33,65,12,37,66,5];

// Memory only: counted targets (marked ones are never counted).
var devices = {};       // address -> { rssi, seen, name }
var nets = {};          // BSSID -> { rssi, seen, name }
// Memory only: everything seen, for the list.
var seenBle = {};
var seenNet = {};

var ghosts = loadGhosts();  // address -> label, from the SD card
var showHidden = false;
var cursor = 0;
var cycle = 0;
var lastBle = 0;
var lastWifi = 0;
var lastError = '';
var lastKeyName = '';
var lastKey = '';
var lastSel = false;

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
  try {
    storage.write(GHOST_FILE, out);
  } catch (e) {
    lastError = 'save failed';
  }
}

function bucket(rssi) {
  if (rssi >= CLOSE_DBM) return 0;
  if (rssi >= NEAR_DBM) return 1;
  return 2;
}

// Rough distance in metres from signal strength. Only a guide.
function metres(rssi) {
  return Math.pow(10, (-59 - rssi) / 25);
}

function rounded(m) {
  if (m < 10) return Math.round(m * 10) / 10;
  return Math.round(m);
}

function fmtM(rssi) {
  return '~' + rounded(metres(rssi)) + 'm';
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

function scanBle(t) {
  var found = [];
  try {
    found = ble.scan(BLE_SECS);
  } catch (e) {
    lastError = 'ble: ' + String(e).substring(0, 14);
    return;
  }
  lastBle = found.length;
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
  var list = [];
  try {
    list = wifi.scan();
  } catch (e) {
    lastError = 'wifi: ' + String(e).substring(0, 13);
    return;
  }
  lastWifi = list.length;
  for (var i = 0; i < list.length; i++) {
    var mac = String(list[i].MAC).toLowerCase();
    var ssid = String(list[i].SSID || '');
    remember(seenNet, mac, list[i].RSSI, t, ssid);
    if (ghosts[mac] === undefined) remember(nets, mac, list[i].RSSI, t, ssid);
  }
  prune(seenNet, t);
  prune(nets, t);
}

// Counts and the distance range for each band, over the targets that aren't marked.
function bandSummary() {
  var out = [
    { n: 0, lo: 0, hi: 0 },
    { n: 0, lo: 0, hi: 0 },
    { n: 0, lo: 0, hi: 0 }
  ];
  var maps = [devices, nets];
  for (var mi = 0; mi < maps.length; mi++) {
    for (var k in maps[mi]) {
      var r = maps[mi][k].rssi;
      var b = bucket(r);
      var d = metres(r);
      var s = out[b];
      if (s.n === 0) {
        s.lo = d;
        s.hi = d;
      } else {
        if (d < s.lo) s.lo = d;
        if (d > s.hi) s.hi = d;
      }
      s.n++;
    }
  }
  return out;
}

// The list: every target seen, strongest first.
function buildList() {
  var items = [];
  for (var a in seenBle) {
    items.push({ addr: a, kind: 'BT', name: seenBle[a].name, rssi: seenBle[a].rssi });
  }
  for (var m in seenNet) {
    items.push({ addr: m, kind: 'AP', name: seenNet[m].name, rssi: seenNet[m].rssi });
  }
  items.sort(function (x, y) { return y.rssi - x.rssi; });
  var out = [];
  for (var i = 0; i < items.length; i++) {
    var it = items[i];
    it.ghost = ghosts[it.addr] !== undefined;
    if (it.ghost && !showHidden) continue;
    it.label = it.name ? it.name : it.kind + ' ' + it.addr.substring(it.addr.length - 5);
    out.push(it);
  }
  return out;
}

function toggleMine(it) {
  if (ghosts[it.addr] !== undefined) {
    delete ghosts[it.addr];
  } else {
    ghosts[it.addr] = it.label;
    delete devices[it.addr];
    delete nets[it.addr];
  }
  saveGhosts();
}

function shorten(s, n) {
  return s.length > n ? s.substring(0, n) : s;
}

// signal -95..-30 maps to a bar of 0..20 pixels
function barLen(rssi) {
  var v = Math.round((rssi + 95) * 20 / 65);
  if (v < 1) v = 1;
  if (v > 20) v = 20;
  return v;
}

// 0 = something close, 1 = something near, 2 = only far things, -1 = nothing
function levelOf(sum) {
  if (sum[0].n > 0) return 0;
  if (sum[1].n > 0) return 1;
  if (sum[2].n > 0) return 2;
  return -1;
}

// The head, with an expression that follows the nearest target. Brows and
// the mouth are cut out in the night color so they show on the head.
function drawHead(level) {
  var hx = HEAD_X;
  var hy = HEAD_Y;
  for (var i = 0; i < HEAD_RUNS.length; i += 3) {
    display.drawFillRect(hx + HEAD_RUNS[i], hy + HEAD_RUNS[i + 1], HEAD_RUNS[i + 2], 1, cHead);
  }
  var label = 'asleep';
  if (level === 0) {
    // close: wide gold eyes, angry brows, mouth open with teeth and fire
    label = 'ROAR!';
    display.drawFillRect(hx + 46, hy + 26, 8, 7, cGold);
    display.drawFillRect(hx + 56, hy + 24, 8, 7, cGold);
    display.drawFillRect(hx + 49, hy + 28, 2, 4, cNight);
    display.drawFillRect(hx + 59, hy + 26, 2, 4, cNight);
    display.drawFillRect(hx + 44, hy + 20, 12, 2, cNight);
    display.drawFillRect(hx + 56, hy + 22, 12, 2, cNight);
    display.drawFillRect(hx + 60, hy + 45, 28, 9, cNight);
    display.drawFillRect(hx + 64, hy + 45, 2, 3, cGold);
    display.drawFillRect(hx + 72, hy + 45, 2, 3, cGold);
    display.drawFillRect(hx + 80, hy + 45, 2, 3, cGold);
    display.drawFillRect(hx + 88, hy + 42, 20, 6, cFire);
    display.drawFillRect(hx + 92, hy + 43, 12, 3, cGold);
  } else if (level === 1) {
    // near: open eyes, flat brows, mouth open a little
    label = 'alert';
    display.drawFillRect(hx + 48, hy + 27, 7, 6, cGold);
    display.drawFillRect(hx + 57, hy + 27, 7, 6, cGold);
    display.drawFillRect(hx + 50, hy + 29, 2, 3, cNight);
    display.drawFillRect(hx + 59, hy + 29, 2, 3, cNight);
    display.drawFillRect(hx + 46, hy + 23, 10, 2, cNight);
    display.drawFillRect(hx + 58, hy + 23, 10, 2, cNight);
    display.drawFillRect(hx + 62, hy + 48, 24, 3, cNight);
  } else if (level === 2) {
    // far: calm, half-open eyes, mouth closed
    label = 'calm';
    display.drawFillRect(hx + 48, hy + 29, 7, 3, cGold);
    display.drawFillRect(hx + 57, hy + 29, 7, 3, cGold);
    display.drawFillRect(hx + 62, hy + 49, 24, 2, cNight);
  } else {
    // nothing around: eyes shut, mouth closed
    display.drawFillRect(hx + 48, hy + 30, 7, 2, cNight);
    display.drawFillRect(hx + 57, hy + 30, 7, 2, cNight);
    display.drawFillRect(hx + 62, hy + 49, 24, 2, cNight);
  }
  display.setTextSize(1);
  display.setTextColor(cGold);
  display.drawString(label, hx + 14, hy + 90);
}

function briefing(level) {
  if (level === 0) return 'THREAT: HIGH';
  if (level === 1) return 'THREAT: MED';
  if (level === 2) return 'THREAT: LOW';
  return 'ALL CLEAR';
}

function bandLine(name, s) {
  if (s.n === 0) return name + ' 0';
  return name + ' ' + s.n + '  ' + rounded(s.lo) + '-' + rounded(s.hi) + 'm';
}

// A heat bar: 12 segments, lit from the left up to how close the signal is.
// The first segments are gold (far end), then orange, then red (close end).
function heatBar(x, y, rssi, selected, ghost) {
  var lit = Math.round((rssi + 95) * 12 / 65);
  if (lit < 1) lit = 1;
  if (lit > 12) lit = 12;
  for (var s = 0; s < 12; s++) {
    var on = s < lit;
    var col;
    if (selected) {
      col = on ? cNight : cTan;
    } else if (ghost) {
      col = on ? cDim : cBar;
    } else if (!on) {
      col = cBar;
    } else {
      col = s < 5 ? cGold : s < 9 ? cFire : cHot;
    }
    display.drawFillRect(x + s * 5, y, 4, 6, col);
  }
}

function draw(list, sum, scanning) {
  display.fill(cNight);
  display.setTextSize(1);

  // monitor frame and title
  display.drawRect(1, 1, SW - 2, SH - 2, cDim);
  display.setTextColor(cGold);
  display.drawString("CLASSIFIED // PROXIMITY", 6, 5);
  display.drawFillRect(4, 15, 114, 1, cGold);
  display.setTextColor(scanning ? cFire : cGold);
  display.drawString(scanning ? "LIVE" : "HOLD", 180, 5);

  // one-line summary: how many close, near and far
  display.setTextColor(cGold);
  display.drawString("close " + sum[0].n + "  near " + sum[1].n, 6, 18);
  display.setTextColor(cTan);
  display.drawString("far " + sum[2].n, 6, 27);

  // nearest target, as the lock
  display.setTextColor(cCream);
  if (list.length > 0 && !list[0].ghost) {
    display.drawString("LOCK " + shorten(list[0].label, 9) + " " + fmtM(list[0].rssi), 6, 36);
  } else {
    display.drawString("LOCK none", 6, 36);
  }
  display.setTextColor(cDim);
  display.drawString("TARGETS", 6, 45);

  // the target list: a heat bar per device, closest on top
  if (list.length === 0) {
    display.setTextColor(cTan);
    display.drawString(scanning ? "nothing in range yet" : "paused: nothing listed", 6, LIST_TOP);
  }
  var top = 0;
  if (cursor >= LIST_ROWS) top = cursor - LIST_ROWS + 1;
  for (var i = 0; i < LIST_ROWS && top + i < list.length; i++) {
    var idx = top + i;
    var it = list[idx];
    var y = LIST_TOP + i * ROW_H;
    var b = bucket(it.rssi);
    var rowColor = b === 0 ? cFire : b === 1 ? cGold : cTan;
    if (it.ghost) rowColor = cDim;
    if (idx === cursor) {
      display.drawFillRect(3, y - 1, 121, ROW_H, cGold);
      display.setTextColor(cNight);
    } else {
      display.setTextColor(rowColor);
    }
    var prefix = it.ghost ? "x " : "";
    display.drawString(prefix + shorten(it.label, 8), 6, y);
    heatBar(62, y + 1, it.rssi, idx === cursor, it.ghost);
  }

  // Kaiju the host: head, expression, briefing
  drawHead(levelOf(sum));
  display.setTextColor(cFire);
  display.drawString(briefing(levelOf(sum)), 128, 113);

  // footer
  display.setTextColor(cTan);
  if (lastError) {
    display.setTextColor(cFire);
    display.drawString(shorten(lastError, 26), 6, SH - 11);
  } else {
    display.drawString("M MARK  H SHOW  ESC QUIT", 6, SH - 11);
  }
  display.setTextColor(cDim);
  display.drawString("b" + lastBle + " w" + lastWifi + " k:" + lastKeyName, 150, SH - 11);
}

function readKey() {
  var k = keyboard.getKeysPressed();
  var key = k.length > 0 ? k[0] : '';
  var edge = key !== '' && key !== lastKey;
  lastKey = key;
  if (edge) lastKeyName = key;
  return edge ? key : '';
}

function handleInput(list) {
  var acted = false;
  var key = readKey();
  var sel = keyboard.getSelPress();
  var selEdge = sel && !lastSel;
  lastSel = sel;

  if (key === ';' || key === ',') {
    if (cursor > 0) cursor--;
    acted = true;
  } else if (key === '.' || key === '/') {
    if (cursor < list.length - 1) cursor++;
    acted = true;
  } else if (key === 'm' || key === 'M' || selEdge) {
    if (list.length > 0) toggleMine(list[cursor]);
    acted = true;
  } else if (key === 'h' || key === 'H') {
    showHidden = !showHidden;
    cursor = 0;
    acted = true;
  }
  return acted;
}

// Scanning pauses for a few seconds after each key, so the keys respond at once.
var HOLD_MS = 3000;
var lastInputAt = -100000;
var lastSig = '';

function signature(list, sum, level, scanning) {
  var s = cursor + '|' + showHidden + '|' + level + '|' + scanning + '|' + lastBle + '|' + lastWifi;
  s += '|' + lastKeyName + '|' + lastError + '|' + sum[0].n + sum[1].n + sum[2].n;
  for (var i = 0; i < list.length && i < LIST_ROWS; i++) {
    s += '|' + list[i].label + ':' + Math.round(list[i].rssi / 3) + (list[i].ghost ? 'g' : '');
  }
  return s;
}

function main() {
  // one scan at start, so there's something to look at straight away
  scanBle(now());
  scanWifi(now());
  var list = buildList();
  var sum = bandSummary();
  draw(list, sum, true);
  lastSig = signature(list, sum, levelOf(sum), true);
  while (true) {
    if (keyboard.getEscPress()) break;

    var t0 = now();
    if (handleInput(buildList())) lastInputAt = t0;
    var scanning = now() - lastInputAt > HOLD_MS;

    if (scanning) {
      scanBle(now());
      if (cycle % WIFI_EVERY === 0) scanWifi(now());
      cycle++;
      if (lastError && lastError.indexOf('ble') === 0 && lastBle > 0) lastError = '';
    }

    list = buildList();
    if (cursor >= list.length) cursor = Math.max(0, list.length - 1);
    sum = bandSummary();
    var level = levelOf(sum);
    var sig = signature(list, sum, level, scanning);
    if (sig !== lastSig) {
      draw(list, sum, scanning);
      lastSig = sig;
    }
    delay(30);
  }
}

main();
