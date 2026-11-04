// Kaiju Inventory: see what's around, and keep your own lab devices in a list.
//
// This app only listens and saves labels. It sends nothing and calls no attack
// function. Kaijugotchi reads its own file (/BrucePCAP/owned_aps.txt). This app
// reads and writes /kaiju_inventory.txt, and nothing else uses that file.
//
// Keys:
//   ; , up     . / down     move
//   Select     SCAN: add the device to your inventory
//              INVENTORY: remove it from your inventory
//   L          label the highlighted device (this also adds it to your inventory)
//   I          switch between SCAN and INVENTORY
//   Esc        quit. Hold it for a second: each scan blocks the keys briefly.

var display = require('display');
var keyboard = require('keyboard');
var ble = require('ble');
var wifi = require('wifi');
var storage = require('storage');

var SW = display.width();
var SH = display.height();

var cNight = display.color(40, 22, 10);
var cGold = display.color(217, 162, 27);
var cFire = display.color(230, 120, 30);
var cTan = display.color(210, 180, 140);
var cCream = display.color(245, 230, 200);
var cDim = display.color(110, 84, 56);

var INV_FILE = '/kaiju_inventory.txt';
var STALE_MS = 20000;   // an item not seen for 20 seconds is no longer visible
var HOLD_MS = 3000;     // after a key press, pause scanning for 3 seconds
var SCAN_EVERY_MS = 4000;  // scan at most this often, so the keys stay responsive
var LIST_TOP = 28;
var ROW_H = 11;
var LIST_ROWS = 6;

var inv = loadInv();    // id -> label, saved on the card
var seen = {};          // id -> { kind, name, rssi, chan, enc, t }, what's visible now
var view = 0;           // 0 = SCAN, 1 = INVENTORY
var cursor = 0;
var lastError = '';   // from the last scan
var saveError = '';  // from the last save
var lastKey = '';
var lastSel = false;
var lastInputAt = -100000;
var lastScanAt = -100000;
var lastSig = '';

// ---- ids and labels ------------------------------------------------------

// An id looks like "ap:aa:bb:cc:dd:ee:ff" or "bt:aa:bb:cc:dd:ee:ff".
function makeId(kind, addr) {
  return kind + ':' + String(addr).toLowerCase();
}

function kindOf(id) {
  return id.substring(0, id.indexOf(':'));
}

function addrOf(id) {
  return id.substring(id.indexOf(':') + 1);
}

function shorten(s, n) {
  return s.length > n ? s.substring(0, n) : s;
}

// The saved label, or what the device calls itself, or a short fallback.
function labelFor(id) {
  if (inv[id] !== undefined) return inv[id];
  var s = seen[id];
  if (s && s.name) return s.name;
  return kindOf(id).toUpperCase() + ' ' + shorten(addrOf(id), 8);
}

// Labels are saved one per line, so they can't hold a bar or a line break.
function cleanLabel(text) {
  var out = String(text).split('|').join('-').split('\n').join(' ');
  return out.trim();
}

// ---- the saved inventory -------------------------------------------------

function loadInv() {
  var map = {};
  var text = '';
  try {
    text = storage.read(INV_FILE);
  } catch (e) {
    return map;  // no inventory yet
  }
  if (!text) return map;
  var lines = text.split('\n');
  for (var i = 0; i < lines.length; i++) {
    var line = lines[i].trim();
    if (!line || line.charAt(0) === '#') continue;
    var bar = line.indexOf('|');
    if (bar < 0) continue;
    map[line.substring(0, bar)] = line.substring(bar + 1);
  }
  return map;
}

// storage.write appends unless told otherwise, so pass 'write' to replace the file.
function saveInv() {
  var out = '# Lab inventory. Used only by Kaiju Inventory.\n';
  for (var k in inv) {
    out += k + '|' + inv[k] + '\n';
  }
  var ok = false;
  try {
    ok = storage.write(INV_FILE, out, 'write');
  } catch (e) {
    ok = false;
  }
  saveError = ok ? '' : 'save failed: check the SD card';
}

// ---- scanning (passive: Wi-Fi and Bluetooth listening only) ---------------

function remember(id, kind, name, rssi, chan, enc, t) {
  seen[id] = { kind: kind, name: name, rssi: rssi, chan: chan, enc: enc, t: t };
}

function scanOnce(t) {
  lastError = '';
  try {
    var bt = ble.scan(1);
    for (var i = 0; i < bt.length; i++) {
      var n = String(bt[i].name || '');
      if (n === '<unknown>') n = '';
      remember(makeId('bt', bt[i].address), 'BT', n, bt[i].rssi, 0, '', t);
    }
  } catch (bleErr) {
    lastError = 'bt: ' + String(bleErr).substring(0, 18);
  }
  try {
    var aps = wifi.scan();
    for (var j = 0; j < aps.length; j++) {
      var ssid = String(aps[j].SSID || '');
      remember(
        makeId('ap', aps[j].MAC), 'AP', ssid, aps[j].RSSI,
        aps[j].channel, String(aps[j].encryptionType), t
      );
    }
  } catch (wifiErr) {
    lastError = 'wifi: ' + String(wifiErr).substring(0, 16);
  }
  for (var k in seen) {
    if (t - seen[k].t > STALE_MS) delete seen[k];
  }
}

// ---- the two lists -------------------------------------------------------

// SCAN: everything visible right now, strongest first.
function scanItems() {
  var items = [];
  for (var id in seen) items.push({ id: id, rssi: seen[id].rssi });
  items.sort(function (a, b) { return b.rssi - a.rssi; });
  return items;
}

// INVENTORY: everything saved, visible or not, sorted by label.
function inventoryItems() {
  var items = [];
  for (var id in inv) {
    items.push({ id: id, rssi: seen[id] ? seen[id].rssi : -200 });
  }
  items.sort(function (a, b) {
    var la = labelFor(a.id).toLowerCase();
    var lb = labelFor(b.id).toLowerCase();
    return la < lb ? -1 : la > lb ? 1 : 0;
  });
  return items;
}

function currentItems() {
  return view === 0 ? scanItems() : inventoryItems();
}

// ---- actions: mark, label, unmark ---------------------------------------

function markItem(id) {
  if (inv[id] === undefined) {
    var s = seen[id];
    inv[id] = cleanLabel(s && s.name ? s.name : labelFor(id));
    saveInv();
  }
}

function unmarkItem(id) {
  if (inv[id] !== undefined) {
    delete inv[id];
    saveInv();
  }
}

function labelItem(id) {
  var current = inv[id] !== undefined ? inv[id] : labelFor(id);
  var text = '';
  try {
    // Firmware order: title, max length, message. The field starts empty.
    text = keyboard.keyboard('Label', 24, 'was: ' + shorten(current, 20));
  } catch (e) {
    lastError = 'no keyboard';
    return;
  }
  if (text && String(text).trim().length > 0) {
    inv[id] = cleanLabel(text);
    saveInv();
  }
}

// ---- drawing -------------------------------------------------------------

function infoText(id) {
  var s = seen[id];
  if (!s) return 'not visible';
  if (s.kind === 'AP') return 'ch' + s.chan + ' ' + s.rssi + ' ' + shorten(s.enc, 12);
  return 'ble ' + s.rssi;
}

function draw(items) {
  display.fill(cNight);
  display.setTextSize(1);
  display.drawRect(1, 1, SW - 2, SH - 2, cDim);

  display.setTextColor(cGold);
  display.drawString(view === 0 ? 'SCAN' : 'INVENTORY', 6, 5);
  display.setTextColor(cTan);
  display.drawString(
    view === 0 ? 'nearby devices, mark yours' : 'saved devices, not used by attacks', 6, 16
  );

  if (items.length === 0) {
    display.setTextColor(cTan);
    display.drawString(view === 0 ? 'nothing seen yet' : 'inventory is empty', 6, LIST_TOP);
  }

  var top = cursor >= LIST_ROWS ? cursor - LIST_ROWS + 1 : 0;
  for (var i = 0; i < LIST_ROWS && top + i < items.length; i++) {
    var idx = top + i;
    var it = items[idx];
    var y = LIST_TOP + i * ROW_H;
    var selected = idx === cursor;
    var visible = seen[it.id] !== undefined;
    var mark = inv[it.id] !== undefined ? '* ' : '  ';
    if (selected) {
      display.drawFillRect(3, y - 1, 234, ROW_H, cGold);
      display.setTextColor(cNight);
    } else {
      display.setTextColor(visible ? cTan : cDim);
    }
    display.drawString(mark + shorten(labelFor(it.id), 10), 6, y);
    display.drawString(visible ? infoText(it.id) : 'off', 100, y);
  }

  // details for the highlighted device
  if (items.length > 0) {
    var cur = items[cursor];
    display.setTextColor(cCream);
    var kind = kindOf(cur.id).toUpperCase();
    var detail = kind + '  ' + addrOf(cur.id);
    if (seen[cur.id] === undefined) detail += '  (not visible now)';
    display.drawString(detail, 6, 104);
  }

  var msg = saveError || lastError;
  display.setTextColor(msg ? cFire : cTan);
  if (msg) {
    display.drawString(shorten(msg, 36), 6, SH - 11);
  } else if (view === 0) {
    display.drawString('SEL mark  L label  I inv  ESC', 6, SH - 11);
  } else {
    display.drawString('SEL remove  L label  I scan  ESC', 6, SH - 11);
  }
}

// ---- keys ----------------------------------------------------------------

// Reads one key per press, not on every loop it's held.
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

// ---- main loop -----------------------------------------------------------

function signature(items) {
  var s = view + '|' + cursor + '|' + lastError + '|' + saveError;
  for (var i = 0; i < items.length && i < LIST_ROWS; i++) {
    var id = items[i].id;
    s += '|' + id + labelFor(id) + (seen[id] ? Math.round(seen[id].rssi / 3) : 'x');
  }
  return s;
}

function main() {
  scanOnce(now());
  lastScanAt = now();
  var items = currentItems();
  while (true) {
    if (keyboard.getEscPress()) break;

    var key = readKey();
    var sel = selEdge();
    if (key !== '') lastInputAt = now();

    items = currentItems();
    if (cursor >= items.length) cursor = Math.max(0, items.length - 1);

    if (key === ';' || key === ',') {
      if (cursor > 0) cursor--;
    } else if (key === '.' || key === '/') {
      if (cursor < items.length - 1) cursor++;
    } else if (key === 'i' || key === 'I') {
      view = 1 - view;
      cursor = 0;
    } else if (key === 'l' || key === 'L') {
      if (items.length > 0) labelItem(items[cursor].id);
    }

    if (sel && items.length > 0) {
      if (view === 0) markItem(items[cursor].id);
      else unmarkItem(items[cursor].id);
    }

    // scanning pauses for a few seconds after a key press, so keys respond at once
    if (now() - lastInputAt > HOLD_MS && now() - lastScanAt >= SCAN_EVERY_MS) {
      scanOnce(now());
      lastScanAt = now();
    }

    items = currentItems();
    if (cursor >= items.length) cursor = Math.max(0, items.length - 1);
    var sig = signature(items);
    if (sig !== lastSig) {
      draw(items);
      lastSig = sig;
    }
    delay(30);
  }
}

main();
