// Kaiju Proximity: a simple Bluetooth radar, with Kaiju as the host.
//
// Step 1, PICK: a short Bluetooth scan lists the devices around you.
//   ; or , and . or /    move up and down
//   Select               mark the device as yours (press again to unmark)
//   Select on START      go to the radar
//
// Step 2, RADAR: no buttons. It shows how many devices are close, near and
//   far, with a heat bar for each one. Devices you marked are left out.
//   Esc quits. Hold it for a second: each scan blocks the keys briefly.
//
// Distances are a rough guide from signal strength, not a measurement.
// Phones change their Bluetooth address from time to time, so a marked phone
// can come back as a new device. Mark it again if that happens.
// Marks are saved to /scripts/ghosts.txt. Full addresses are never shown.

var display = require('display');
var keyboard = require('keyboard');
var ble = require('ble');
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
var NEAR_DBM = -70;     // at or above this: near, below it: far
var STALE_MS = 20000;   // a device not heard for 20 seconds leaves the radar
var PICK_ROUNDS = 5;    // pick screen: five scans, about 10 seconds
var SCAN_SECS = 2;      // one Bluetooth scan
var GHOST_FILE = '/scripts/ghosts.txt';
var LIST_TOP = 38;      // first row of the list
var ROW_H = 11;
var LIST_ROWS = 7;

// Kaiju's head, host of the show. Each run is [x, y, width] in a 96x88 box.
var HEAD_X = 130;
var HEAD_Y = 14;
var HEAD_RUNS = [17,5,2,17,6,3,18,7,4,18,8,6,19,9,6,20,10,7,20,11,9,21,12,9,21,13,11,22,14,12,7,15,3,22,15,13,8,16,4,23,16,16,9,17,6,23,17,22,10,18,7,24,18,25,11,19,9,25,19,27,12,20,10,25,20,30,13,21,45,13,22,48,14,23,50,15,24,51,16,25,53,3,26,1,17,26,54,4,27,5,18,27,56,5,28,9,19,28,57,6,29,73,8,30,73,9,31,74,10,32,74,11,33,75,13,34,75,14,35,75,15,36,75,16,37,75,15,38,76,7,39,84,3,40,88,5,41,85,8,42,82,10,43,79,12,44,77,15,45,72,16,46,68,17,47,64,17,48,61,17,49,58,17,50,56,17,51,54,17,52,52,18,53,49,18,54,47,18,55,45,18,56,43,19,57,41,20,58,38,21,59,36,22,60,33,23,61,31,24,62,28,25,63,26,28,64,21,33,65,12,37,66,5];


var ghosts = loadGhosts();  // address -> label, the devices marked as yours
var devices = {};           // radar: address -> { rssi, seen, name }
var lastBle = 0;
var lastError = '';   // from the last scan
var saveError = '';  // from the last save
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
  // storage.write appends unless told otherwise, so pass 'write' to replace the file.
  var ok = false;
  try {
    ok = storage.write(GHOST_FILE, out, 'write');
  } catch (e) {
    ok = false;
  }
  saveError = ok ? '' : 'save failed: check the SD card';
}

function isMarked(addr) {
  return ghosts[addr] !== undefined;
}

function toggleMine(it) {
  if (isMarked(it.addr)) {
    delete ghosts[it.addr];
  } else {
    ghosts[it.addr] = it.label;
    delete devices[it.addr];
  }
  saveGhosts();
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

function remember(map, key, rssi, t, name) {
  var e = map[key];
  if (e === undefined) {
    map[key] = { rssi: rssi, seen: t, name: name };
  } else {
    // smooth so one loud or quiet reading doesn't make the bars jump
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

function labelOf(addr, name) {
  return name ? name : 'BT ' + addr.substring(addr.length - 5);
}

function shorten(s, n) {
  return s.length > n ? s.substring(0, n) : s;
}

// One Bluetooth scan. Adds what it hears to the map.
function scanInto(map, t, skipMarked) {
  var found = [];
  try {
    found = ble.scan(SCAN_SECS);
  } catch (e) {
    lastError = 'bt: ' + String(e).substring(0, 18);
    return;
  }
  lastError = '';
  lastBle = found.length;
  for (var i = 0; i < found.length; i++) {
    var addr = String(found[i].address).toLowerCase();
    if (skipMarked && isMarked(addr)) continue;
    var name = String(found[i].name || '');
    if (name === '<unknown>') name = '';
    remember(map, addr, found[i].rssi, t, name);
  }
}

// Everything found in the pick scan, strongest first.
function pickList(found) {
  var items = [];
  for (var a in found) {
    items.push({ addr: a, rssi: found[a].rssi, label: labelOf(a, found[a].name), mine: isMarked(a) });
  }
  items.sort(function (x, y) { return y.rssi - x.rssi; });
  return items;
}

// The radar: marked devices are never in the map, strongest first.
function radarList() {
  var items = [];
  for (var a in devices) {
    items.push({ addr: a, rssi: devices[a].rssi, label: labelOf(a, devices[a].name) });
  }
  items.sort(function (x, y) { return y.rssi - x.rssi; });
  return items;
}

// How many are close, near and far.
function countsOf(items) {
  var n = [0, 0, 0];
  for (var i = 0; i < items.length; i++) n[bucket(items[i].rssi)]++;
  return n;
}

// 0 = something close, 1 = near, 2 = only far, -1 = nothing
function levelOf(n) {
  if (n[0] > 0) return 0;
  if (n[1] > 0) return 1;
  if (n[2] > 0) return 2;
  return -1;
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

// The head, with an expression that follows the nearest target. Brows and
// the mouth are cut out in the night color so they show on the head.
function drawHead(level, override) {
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
  if (override) label = override;
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

function frame(title) {
  display.fill(cNight);
  display.setTextSize(1);
  display.drawRect(1, 1, SW - 2, SH - 2, cDim);
  display.setTextColor(cGold);
  display.drawString(title, 6, 5);
  display.drawFillRect(4, 15, 114, 1, cGold);
}

function footer(text) {
  var msg = saveError || lastError;
  display.setTextColor(msg ? cFire : cTan);
  display.drawString(msg ? shorten(msg, 36) : text, 6, SH - 11);
}

function drawScanning(round) {
  frame('CLASSIFIED // PROXIMITY');
  display.setTextColor(cFire);
  display.drawString('scanning ' + round + ' of ' + PICK_ROUNDS, 6, 30);
  display.setTextColor(cTan);
  display.drawString('about 10 seconds', 6, 42);
  drawHead(2, 'scanning');
  footer('ESC quit');
}

function drawPick(list, cursor) {
  frame('CLASSIFIED // PROXIMITY');
  display.setTextColor(cTan);
  display.drawString('Select marks yours', 6, 19);
  display.drawString('then START', 6, 28);
  var rows = list.length + 1;   // row 0 is START
  var top = cursor >= LIST_ROWS ? cursor - LIST_ROWS + 1 : 0;
  for (var i = 0; i < LIST_ROWS && top + i < rows; i++) {
    var idx = top + i;
    var y = LIST_TOP + i * ROW_H;
    var selected = idx === cursor;
    if (selected) {
      display.drawFillRect(3, y - 1, 121, ROW_H, cGold);
      display.setTextColor(cNight);
    } else {
      display.setTextColor(idx === 0 ? cFire : list[idx - 1].mine ? cDim : cTan);
    }
    if (idx === 0) {
      display.drawString('START >>', 6, y);
      continue;
    }
    var it = list[idx - 1];
    display.drawString((it.mine ? 'x ' : '') + shorten(it.label, 8), 6, y);
    heatBar(62, y + 1, it.rssi, selected, it.mine);
  }
  if (list.length === 0) {
    display.setTextColor(cTan);
    display.drawString('nothing heard', 6, LIST_TOP + ROW_H);
  }
  drawHead(2, 'choose');
  footer('ESC quit');
}

function drawRadar(list, n) {
  frame('KAIJU PROXIMITY');
  display.setTextColor(cFire);
  display.drawString('LIVE', 190, 5);
  display.setTextColor(cGold);
  display.drawString('close ' + n[0] + '  near ' + n[1], 6, 19);
  display.setTextColor(cTan);
  display.drawString('far ' + n[2], 6, 28);
  if (list.length === 0) {
    display.setTextColor(cTan);
    display.drawString('nothing in range yet', 6, LIST_TOP);
  }
  for (var i = 0; i < LIST_ROWS && i < list.length; i++) {
    var it = list[i];
    var y = LIST_TOP + i * ROW_H;
    var b = bucket(it.rssi);
    display.setTextColor(b === 0 ? cFire : b === 1 ? cGold : cTan);
    display.drawString(shorten(it.label, 8), 6, y);
    heatBar(62, y + 1, it.rssi, false, false);
  }
  var lvl = levelOf(n);
  drawHead(lvl);
  display.setTextColor(cFire);
  display.drawString(briefing(lvl), 128, 113);
  footer('hold ESC to quit');
}

// Reads one key press per press, not on every loop it's held.
function readKey() {
  var k = keyboard.getKeysPressed();
  var key = k.length > 0 ? k[0] : '';
  var edge = key !== '' && key !== lastKey;
  lastKey = key;
  return edge ? key : '';
}

function selEdge() {
  var sel = keyboard.getSelPress();
  var edge = sel && !lastSel;
  lastSel = sel;
  return edge;
}

// Step 1. Returns true when START is chosen, false on Esc.
function pick() {
  var found = {};
  for (var round = 1; round <= PICK_ROUNDS; round++) {
    drawScanning(round);
    scanInto(found, now(), false);
    if (keyboard.getEscPress()) return false;
  }
  var list = pickList(found);
  var cursor = list.length > 0 ? 1 : 0;
  var lastSig = '';
  while (true) {
    if (keyboard.getEscPress()) return false;
    var key = readKey();
    var sel = selEdge();
    if (key === ';' || key === ',') {
      if (cursor > 0) cursor--;
    } else if (key === '.' || key === '/') {
      if (cursor < list.length) cursor++;
    }
    if (sel) {
      if (cursor === 0) return true;
      toggleMine(list[cursor - 1]);
      list = pickList(found);
    }
    var sig = cursor + '|' + lastError + '|' + saveError;
    for (var i = 0; i < list.length; i++) sig += '|' + list[i].label + list[i].mine;
    if (sig !== lastSig) {
      drawPick(list, cursor);
      lastSig = sig;
    }
    delay(30);
  }
}

// Step 2. No buttons: scans until Esc.
function radar() {
  var lastSig = '';
  while (true) {
    if (keyboard.getEscPress()) return;
    scanInto(devices, now(), true);
    prune(devices, now());
    var list = radarList();
    var n = countsOf(list);
    var sig = n.join(',') + '|' + lastError + '|' + saveError;
    for (var i = 0; i < list.length && i < LIST_ROWS; i++) {
      sig += '|' + list[i].label + Math.round(list[i].rssi / 3);
    }
    if (sig !== lastSig) {
      drawRadar(list, n);
      lastSig = sig;
    }
    delay(20);
  }
}

function main() {
  try {
    if (!pick()) return;
    radar();
  } catch (e) {
    showError(String(e));
  }
}

// If the script crashes, show the message and save it to the card so it can be read.
function showError(msg) {
  try {
    storage.write('/kaiju_error.txt', msg + '\n');
  } catch (e) {
    // no card: the message still shows on screen
  }
  display.fill(cNight);
  display.setTextSize(1);
  display.setTextColor(cFire);
  display.drawString('KAIJU PROXIMITY ERROR', 6, 20);
  display.setTextColor(cCream);
  display.drawString(shorten(msg, 36), 6, 36);
  display.drawString(shorten(msg.substring(36), 36), 6, 46);
  display.setTextColor(cTan);
  display.drawString('saved to /kaiju_error.txt', 6, 66);
  display.drawString('press Esc', 6, 76);
  while (!keyboard.getEscPress()) {
    delay(50);
  }
}

main();
