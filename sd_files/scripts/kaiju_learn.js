// Kaiju Learn
// A short guide to each menu, read from /scripts/learn/*.txt on the SD card.
//   ; or , = up / previous page      . or / = down / next page
//   Select   = open a topic
//   Esc      = back to the topics, or quit from the topic list

var display = require('display');
var keyboard = require('keyboard');
var storage = require('storage');

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
    var y = 18 + i * 9;
    if (i === sel) {
      display.drawFillRect(4, y - 1, SW - 8, 9, cRust);
      display.setTextColor(cBg);
    } else {
      display.setTextColor(cDark);
    }
    display.drawString(TOPICS[i][0], 8, y);
  }
  display.setTextColor(cDark);
  display.drawString("Select = open   Esc = quit", 8, SH - 12);
}

function loadLines(key) {
  var text = storage.read('/scripts/learn/' + key + '.txt');
  if (!text) text = 'Missing file: /scripts/learn/' + key + '.txt';
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
