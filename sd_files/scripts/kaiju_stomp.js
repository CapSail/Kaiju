// Kaiju Stomp
// Run the Kaiju through the city and hop over the buildings.
//   ;  or  Select  = jump
//   /               = breathe fire (burns the next building in range)
//   Esc             = quit
// On game over: Select = play again, Esc = quit.

var display = require('display');
var keyboard = require('keyboard');

var SW = display.width();
var SH = display.height();

// 70s palette
var cSky = display.color(210, 180, 140);
var cDark = display.color(40, 22, 10);
var cBrown = display.color(74, 44, 23);
var cMustard = display.color(217, 162, 27);
var cOrange = display.color(230, 120, 30);
var cRust = display.color(181, 83, 42);
var cCream = display.color(245, 230, 200);

var GROUND = 112;      // y of the ground line, the Kaiju's feet rest here
var KX = 24;           // Kaiju left edge
var KW = 34;           // Kaiju width
var KH = 22;           // Kaiju height
var JUMP_V = -11;      // jump speed, peaks about 55px above the ground
var GRAV = 1;
var FIRE_RANGE = 70;   // how far the breath reaches
var FIRE_COOL = 30;    // frames between breaths
var FIRE_FRAMES = 4;   // frames the breath stays on screen

var st;                // game state, reset by newGame()

function newGame() {
  st = {
    kY: GROUND - KH,
    lastKY: GROUND - KH,
    kVy: 0,
    onGround: true,
    blds: [],
    speed: 4,
    score: 0,
    cleared: 0,
    fireCool: 0,
    fireLeft: 0,
    lastFireLen: 0,
    nextGap: 80,
    over: false
  };
  display.fill(cSky);
  display.drawFillRect(0, GROUND, SW, SH - GROUND, cDark);
  spawnBuilding(SW + 10);
}

function spawnBuilding(x) {
  var h = 8 + Math.floor(Math.random() * 7);    // 8..14 tall, all clearable
  var w = 12 + Math.floor(Math.random() * 13);  // 12..24 wide
  st.blds.push({ x: x, w: w, h: h, counted: false, burned: false });
}

function eraseBuilding(b) {
  display.drawFillRect(b.x, GROUND - b.h, b.w, b.h, cSky);
}

function drawBuilding(b) {
  display.drawFillRect(b.x, GROUND - b.h, b.w, b.h, cBrown);
  for (var wx = b.x + 3; wx < b.x + b.w - 3; wx += 5) {
    display.drawFillRect(wx, GROUND - b.h + 3, 2, 2, cMustard);
    if (b.h > 14) display.drawFillRect(wx, GROUND - b.h + 9, 2, 2, cMustard);
  }
}

function eraseKaiju(y) {
  display.drawFillRect(KX - 6, y, KW + 6, KH, cSky);
}

function drawKaiju(y) {
  display.drawFillRect(KX, y + 6, 26, 12, cDark);        // body
  display.drawFillRect(KX + 22, y, 12, 9, cDark);        // head
  display.drawFillRect(KX - 6, y + 10, 8, 4, cDark);     // tail
  display.drawFillRect(KX + 4, y + 2, 3, 4, cDark);      // back spikes
  display.drawFillRect(KX + 12, y + 2, 3, 4, cDark);
  display.drawFillRect(KX + 4, y + 18, 5, 4, cDark);     // legs
  display.drawFillRect(KX + 16, y + 18, 5, 4, cDark);
  display.drawFillRect(KX + 28, y + 2, 2, 2, cMustard);  // eye
}

function step() {
  var s = st;
  var keys = keyboard.getKeysPressed();
  var jump = keyboard.getSelPress() || (keys.length > 0 && keys[0] === ';');
  var fire = keys.length > 0 && keys[0] === '/';

  // 1. erase last frame: Kaiju, breath, and every building
  eraseKaiju(s.lastKY);
  if (s.lastFireLen > 0) {
    display.drawFillRect(KX + 34, s.lastKY + 2, s.lastFireLen + 4, 10, cSky);
  }
  for (var i = 0; i < s.blds.length; i++) {
    eraseBuilding(s.blds[i]);
  }

  // 2. jump and gravity
  if (jump && s.onGround) {
    s.kVy = JUMP_V;
    s.onGround = false;
  }
  s.kVy += GRAV;
  s.kY += s.kVy;
  if (s.kY >= GROUND - KH) {
    s.kY = GROUND - KH;
    s.kVy = 0;
    s.onGround = true;
  }

  // 3. breath: burns the nearest building in range, once per cooldown
  if (s.fireCool > 0) s.fireCool--;
  if (fire && s.fireCool === 0) {
    s.fireCool = FIRE_COOL;
    s.fireLeft = FIRE_FRAMES;
    var best = -1;
    for (var j = 0; j < s.blds.length; j++) {
      var gap = s.blds[j].x - (KX + KW);
      if (!s.blds[j].burned && gap >= 0 && gap < FIRE_RANGE) {
        if (best < 0 || gap < s.blds[best].x - (KX + KW)) best = j;
      }
    }
    if (best >= 0) {
      s.blds[best].burned = true;
      s.score++;
    }
  }
  var fireLen = 0;
  if (s.fireLeft > 0) {
    fireLen = FIRE_RANGE;
    s.fireLeft--;
  }

  // 4. move buildings, drop burned or off-screen ones, score passed ones
  var kept = [];
  for (var k = 0; k < s.blds.length; k++) {
    var b = s.blds[k];
    if (b.burned) continue;
    b.x -= s.speed;
    if (b.x + b.w > -2) kept.push(b);
    if (!b.counted && b.x + b.w < KX) {
      b.counted = true;
      s.score++;
      s.cleared++;
      if (s.cleared % 5 === 0 && s.speed < 7) s.speed++;
    }
  }
  s.blds = kept;

  var lastB = s.blds.length > 0 ? s.blds[s.blds.length - 1] : null;
  if (!lastB || lastB.x + lastB.w < SW - s.nextGap) {
    spawnBuilding(SW);
    s.nextGap = 60 + Math.floor(Math.random() * 40);  // room to land: the Kaiju is 34 px wide
  }

  // 5. collision: a building overlaps the Kaiju and its feet are below the roof
  var hit = false;
  for (var m = 0; m < s.blds.length; m++) {
    var c = s.blds[m];
    if (c.x < KX + KW && c.x + c.w > KX && s.kY + KH > GROUND - c.h) hit = true;
  }

  // 6. draw: buildings, breath, then Kaiju on top
  for (var n = 0; n < s.blds.length; n++) {
    drawBuilding(s.blds[n]);
  }
  if (fireLen > 0) {
    display.drawFillRect(KX + 34, s.kY + 4, fireLen, 6, cOrange);
    display.drawFillRect(KX + 34, s.kY + 5, fireLen, 4, cMustard);
  }
  drawKaiju(s.kY);

  // score, in the empty sky at the top left
  display.drawFillRect(0, 0, 90, 12, cSky);
  display.setTextSize(1);
  display.setTextColor(cDark);
  display.drawString("Score " + s.score, 2, 2);

  s.lastKY = s.kY;
  s.lastFireLen = fireLen;
  if (hit) s.over = true;
}

function gameOver() {
  display.drawFillRect(30, 40, SW - 60, 46, cRust);
  display.setTextColor(cCream);
  display.setTextSize(2);
  display.drawString("KAIJU DOWN", 46, 46);
  display.setTextSize(1);
  display.drawString("Score " + st.score + "  Select = again", 46, 72);
  display.drawString("Esc = quit", 46, 82);
}

function main() {
  newGame();
  while (true) {
    if (keyboard.getEscPress()) break;
    if (st.over) {
      gameOver();
      while (true) {
        if (keyboard.getEscPress()) return;
        if (keyboard.getSelPress()) break;
        delay(60);
      }
      newGame();
      continue;
    }
    step();
    delay(45);
  }
}

main();
