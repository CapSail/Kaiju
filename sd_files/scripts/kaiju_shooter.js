// Kaiju Shooter
// Kaiju Edition of Space Shooter (orig. MSI, Bruce interpreter example).
// Select = breathe fire / menu. Prev/Next = move. Esc = quit.

var display = require('display');
var keyboard = require('keyboard');

var width = display.width;
var height = display.height;
var fillScreen = display.fill;
var drawFillRect = display.drawFillRect;
var drawString = display.drawString;
var setTextSize = display.setTextSize;
var color = display.color;
var setTextColor = display.setTextColor;

// 70s palette
var cNight = color(40, 22, 10);
var cBrown = color(74, 44, 23);
var cRust = color(181, 83, 42);
var cOrange = color(230, 120, 30);
var cMustard = color(217, 162, 27);
var cSky = color(210, 180, 140);
var cCream = color(245, 230, 200);

var getPrevPress = keyboard.getPrevPress;
var getNextPress = keyboard.getNextPress;
var getSelPress = keyboard.getSelPress;
var getEscPress = keyboard.getEscPress;

var playerX = 140;
var playerY = 120;
var playerWidth = 12;
var playerHeight = 8;
var playerSpeed = 5;
var playerLives = 3;
var playerScore = 0;

var bullets = [];
var enemies = [];
var enemyBullets = [];
var bosses = [];
var powerUps = [];

var level = 1;
var enemySpawnTimer = 0;
var bossSpawnTimer = 0;
var gameRunning = true;
var gameOver = false;
var menuOpen = false;
var menuSelection = 0;

var screenWidth = width();
var screenHeight = height();

var playerColor = cRust;
var bulletColor = cOrange;
var enemyColor = cRust;
var enemyBulletColor = cMustard;
var bossColor = cRust;
var bgColor = cNight;
var starColor = cMustard;

var stars = [];

function initStars() {
    stars = [];
    for (var i = 0; i < 20; i++) {
        stars.push({
            x: Math.random() * screenWidth,
            y: Math.random() * screenHeight,
            speed: 1 + Math.random() * 2
        });
    }
}

function updateStars() {
    for (var i = 0; i < stars.length; i++) {
        stars[i].y += stars[i].speed;
        if (stars[i].y > screenHeight) {
            stars[i].y = 0;
            stars[i].x = Math.random() * screenWidth;
        }
    }
}

function drawStars() {
    for (var i = 0; i < stars.length; i++) {
        drawFillRect(stars[i].x, stars[i].y, 1, 1, starColor);
    }
}

function createBullet(x, y, isPlayer) {
    if (isPlayer) {
        bullets.push({
            x: x,
            y: y,
            speed: 6,
            width: 4,
            height: 10
        });
    } else {
        enemyBullets.push({
            x: x,
            y: y,
            speed: 3,
            width: 4,
            height: 8
        });
    }
}

function createEnemy(type) {
    var enemy = {
        x: Math.random() * (screenWidth - 16),
        y: -10,
        width: 12,
        height: 8,
        speed: 1 + level * 0.3,
        health: 1,
        type: type,
        shootTimer: 0
    };

    if (type === 2) {
        enemy.width = 16;
        enemy.height = 12;
        enemy.health = 2;
        enemy.speed = 0.8 + level * 0.2;
    } else if (type === 3) {
        enemy.width = 10;
        enemy.height = 6;
        enemy.speed = 2 + level * 0.4;
    }

    enemies.push(enemy);
}

function createBoss(type) {
    var boss = {
        x: screenWidth / 2 - 20,
        y: -30,
        width: 40,
        height: 25,
        speed: 1,
        health: 10 + level * 3,
        maxHealth: 10 + level * 3,
        type: type,
        shootTimer: 0,
        movePattern: 0,
        moveTimer: 0
    };

    if (type === 2) {
        boss.health = 15 + level * 4;
        boss.maxHealth = 15 + level * 4;
        boss.width = 50;
        boss.height = 30;
    }

    bosses.push(boss);
}

function updatePlayer() {
    if (menuOpen) {
        if (getNextPress()) {
            menuSelection = menuSelection < 1 ? menuSelection + 1 : 0;
            delay(150);
        }
        if (getPrevPress()) {
            menuSelection = menuSelection > 0 ? menuSelection - 1 : 1;
            delay(150);
        }
        if (getSelPress()) {
            if (menuSelection === 0) {
                menuOpen = false;
                gameRunning = true;
            } else {
                playerScore = 0;
                playerLives = 3;
                level = 1;
                enemies = [];
                bullets = [];
                enemyBullets = [];
                bosses = [];
                powerUps = [];
                gameOver = false;
                gameRunning = true;
                menuOpen = false;
                playerX = screenWidth / 2 - playerWidth / 2;
                playerY = screenHeight - 30;
            }
            delay(300);
        }
        return;
    }

    if (getNextPress() && playerX + playerWidth < screenWidth) {
        playerX += playerSpeed;
        delay(8);
    }
    if (getPrevPress() && playerX > 0) {
        playerX -= playerSpeed;
        delay(8);
    }
    if (getSelPress()) {
        if (gameRunning) {
            createBullet(playerX + playerWidth / 2 - 1, playerY - 2, true);
            delay(80);
        } else if (gameOver) {
            playerScore = 0;
            playerLives = 3;
            level = 1;
            enemies = [];
            bullets = [];
            enemyBullets = [];
            bosses = [];
            powerUps = [];
            gameOver = false;
            gameRunning = true;
            playerX = screenWidth / 2 - playerWidth / 2;
            playerY = screenHeight - 30;
            delay(300);
        } else {
            menuOpen = true;
            gameRunning = false;
            menuSelection = 0;
            delay(300);
        }
    }
}

function updateBullets() {
    for (var i = bullets.length - 1; i >= 0; i--) {
        bullets[i].y -= bullets[i].speed;
        if (bullets[i].y < -10) {
            bullets.splice(i, 1);
            continue;
        }

        for (var j = enemyBullets.length - 1; j >= 0; j--) {
            if (bullets[i] && enemyBullets[j] &&
                bullets[i].x + bullets[i].width > enemyBullets[j].x &&
                bullets[i].x < enemyBullets[j].x + enemyBullets[j].width &&
                bullets[i].y + bullets[i].height > enemyBullets[j].y &&
                bullets[i].y < enemyBullets[j].y + enemyBullets[j].height) {

                bullets.splice(i, 1);
                enemyBullets.splice(j, 1);
                playerScore += 5;
                break;
            }
        }
    }

    for (var i = enemyBullets.length - 1; i >= 0; i--) {
        if (enemyBullets[i].speedX) {
            enemyBullets[i].x += enemyBullets[i].speedX;
            enemyBullets[i].y += enemyBullets[i].speed;
        } else {
            enemyBullets[i].y += enemyBullets[i].speed;
        }

        if (enemyBullets[i].y > screenHeight + 10 || enemyBullets[i].x < -10 || enemyBullets[i].x > screenWidth + 10) {
            enemyBullets.splice(i, 1);
            continue;
        }

        if (enemyBullets[i].x + enemyBullets[i].width > playerX &&
            enemyBullets[i].x < playerX + playerWidth &&
            enemyBullets[i].y + enemyBullets[i].height > playerY &&
            enemyBullets[i].y < playerY + playerHeight) {

            enemyBullets.splice(i, 1);
            playerLives--;
            if (playerLives <= 0) {
                gameOver = true;
                gameRunning = false;
            }
        }
    }
}

function updateEnemies() {
    enemySpawnTimer++;

    if (bosses.length === 0 && enemySpawnTimer > 50 - level * 3) {
        enemySpawnTimer = 0;
        var numEnemies = Math.min(2, 1 + Math.floor(level / 3));
        for (var e = 0; e < numEnemies; e++) {
            createEnemy(2);
        }
    }

    for (var i = enemies.length - 1; i >= 0; i--) {
        var enemy = enemies[i];
        enemy.y += enemy.speed;

        if (enemy.y > screenHeight + 10) {
            enemies.splice(i, 1);
            continue;
        }

        var playerCenterX = playerX + playerWidth / 2;
        var enemyCenterX = enemy.x + enemy.width / 2;

        if (enemy.type === 1 || enemy.type === 3) {
            if (enemyCenterX < playerCenterX - 2) {
                enemy.x += 0.5;
            } else if (enemyCenterX > playerCenterX + 2) {
                enemy.x -= 0.5;
            }
        }

        enemy.shootTimer++;
        if (enemy.shootTimer > 40 - level * 2) {
            enemy.shootTimer = 0;
            if (Math.random() < 0.8) {
                var bulletX = enemy.x + enemy.width / 2 - 1;
                var bulletY = enemy.y + enemy.height;

                if (enemy.type === 1 || enemy.type === 3) {
                    var dx = playerCenterX - bulletX;
                    var dy = playerY - bulletY;
                    var distance = Math.sqrt(dx * dx + dy * dy);
                    var bulletSpeedX = (dx / distance) * 2;
                    var bulletSpeedY = (dy / distance) * 3;

                    enemyBullets.push({
                        x: bulletX,
                        y: bulletY,
                        speed: bulletSpeedY,
                        speedX: bulletSpeedX,
                        width: 2,
                        height: 4
                    });
                } else {
                    createBullet(bulletX, bulletY, false);
                    if (enemy.type === 2 && Math.random() < 0.6) {
                        createBullet(enemy.x + 2, enemy.y + enemy.height, false);
                        createBullet(enemy.x + enemy.width - 2, enemy.y + enemy.height, false);
                    }
                }
            }
        }

        for (var j = bullets.length - 1; j >= 0; j--) {
            if (bullets[j].x + bullets[j].width > enemy.x &&
                bullets[j].x < enemy.x + enemy.width &&
                bullets[j].y + bullets[j].height > enemy.y &&
                bullets[j].y < enemy.y + enemy.height) {

                bullets.splice(j, 1);
                enemy.health--;

                if (enemy.health <= 0) {
                    enemies.splice(i, 1);
                    playerScore += enemy.type * 10;
                }
                break;
            }
        }

        if (enemy.x + enemy.width > playerX &&
            enemy.x < playerX + playerWidth &&
            enemy.y + enemy.height > playerY &&
            enemy.y < playerY + playerHeight) {

            enemies.splice(i, 1);
            playerLives--;
            if (playerLives <= 0) {
                gameOver = true;
                gameRunning = false;
            }
        }
    }
}

function updateBosses() {
    bossSpawnTimer++;

    if (bosses.length === 0 && bossSpawnTimer > 500 + level * 80) {
        bossSpawnTimer = 0;
        var bossCount = level > 4 ? 2 : 1;
        for (var i = 0; i < bossCount; i++) {
            createBoss(Math.floor(Math.random() * 2) + 1);
        }
    }

    for (var i = bosses.length - 1; i >= 0; i--) {
        var boss = bosses[i];
        boss.moveTimer++;

        if (boss.y < 20) {
            boss.y += boss.speed;
        } else {
            var playerCenterX = playerX + playerWidth / 2;
            var bossCenterX = boss.x + boss.width / 2;

            if (boss.moveTimer > 40) {
                boss.moveTimer = 0;
                boss.movePattern = Math.floor(Math.random() * 4);
            }

            if (boss.movePattern === 0 && boss.x > 5) {
                boss.x -= 1.5;
            } else if (boss.movePattern === 1 && boss.x + boss.width < screenWidth - 5) {
                boss.x += 1.5;
            } else if (boss.movePattern === 2) {
                if (bossCenterX < playerCenterX - 3) {
                    boss.x += 1;
                } else if (bossCenterX > playerCenterX + 3) {
                    boss.x -= 1;
                }
            }

            if (boss.x <= 0) boss.x = 1;
            if (boss.x >= screenWidth - boss.width) boss.x = screenWidth - boss.width - 1;
        }

        boss.shootTimer++;
        if (boss.shootTimer > 25 - level) {
            boss.shootTimer = 0;
            var bulletCount = boss.type === 2 ? 3 : 2;
            for (var k = 0; k < bulletCount; k++) {
                createBullet(boss.x + k * (boss.width / bulletCount), boss.y + boss.height, false);
            }

            if (Math.random() < 0.3) {
                createBullet(boss.x + boss.width / 2, boss.y + boss.height, false);
            }
        }

        for (var j = bullets.length - 1; j >= 0; j--) {
            if (bullets[j].x + bullets[j].width > boss.x &&
                bullets[j].x < boss.x + boss.width &&
                bullets[j].y + bullets[j].height > boss.y &&
                bullets[j].y < boss.y + boss.height) {

                bullets.splice(j, 1);
                boss.health--;

                if (boss.health <= 0) {
                    bosses.splice(i, 1);
                    playerScore += 100 * level;
                    level++;
                }
                break;
            }
        }

        if (boss.x + boss.width > playerX &&
            boss.x < playerX + playerWidth &&
            boss.y + boss.height > playerY &&
            boss.y < playerY + playerHeight) {

            playerLives--;
            if (playerLives <= 0) {
                gameOver = true;
                gameRunning = false;
            }
        }
    }
}

function drawPlayer(x, y) {
    drawFillRect(x + 2, y, 2, 2, cMustard);     // spikes
    drawFillRect(x + 8, y, 2, 2, cMustard);
    drawFillRect(x + 3, y + 2, 6, 4, cRust);    // head
    drawFillRect(x + 1, y + 4, 10, 3, cRust);   // jaw
    drawFillRect(x + 5, y + 3, 2, 1, cCream);   // eye
    drawFillRect(x, y + 7, 12, 1, cBrown);      // base
}

function drawEnemy(enemy) {
    var x = enemy.x;
    var y = enemy.y;

    if (enemy.type === 1) {                     // small saucer
        drawFillRect(x + 3, y, 6, 2, cMustard);
        drawFillRect(x, y + 2, 12, 2, cRust);
        drawFillRect(x + 1, y + 4, 10, 2, cBrown);
    } else if (enemy.type === 2) {              // big saucer
        drawFillRect(x + 5, y, 6, 3, cMustard);
        drawFillRect(x + 2, y + 3, 12, 3, cOrange);
        drawFillRect(x, y + 6, 16, 3, cRust);
        drawFillRect(x + 3, y + 9, 10, 3, cBrown);
    } else {                                    // fast dart saucer
        drawFillRect(x + 2, y, 6, 2, cOrange);
        drawFillRect(x, y + 2, 10, 2, cMustard);
        drawFillRect(x + 2, y + 4, 6, 2, cBrown);
    }
}

function drawBoss(boss) {
    var x = Math.round(boss.x);
    var y = Math.round(boss.y);
    var w = boss.width;
    var h = boss.height;

    drawFillRect(x + Math.round(w / 4), y, Math.round(w / 2), Math.round(h / 4), cMustard);   // cupola
    drawFillRect(x, y + Math.round(h / 4), w, Math.round(h / 3), cRust);                       // hull
    drawFillRect(x + 2, y + Math.round(h / 2), w - 4, Math.round(h / 4), cBrown);             // underside
    drawFillRect(x + 4, y + Math.round(h / 3), w - 8, 2, cOrange);                             // light band

    if (boss.type === 2) {
        drawFillRect(x - 3, y + Math.round(h / 4), 4, 4, cOrange);                             // wing lights
        drawFillRect(x + w - 1, y + Math.round(h / 4), 4, 4, cOrange);
    }
}

function drawGame() {
    fillScreen(bgColor);

    drawStars();
    setTextColor(cCream);

    drawPlayer(playerX, playerY);

    for (var i = 0; i < bullets.length; i++) {
        var bullet = bullets[i];
        drawFillRect(bullet.x, bullet.y, bullet.width, bullet.height, bulletColor);
        drawFillRect(bullet.x, bullet.y - 2, 2, 2, cCream);
    }

    for (var i = 0; i < enemyBullets.length; i++) {
        var bullet = enemyBullets[i];
        drawFillRect(bullet.x, bullet.y, bullet.width, bullet.height, enemyBulletColor);
        drawFillRect(bullet.x, bullet.y + 2, 2, 2, cBrown);
    }

    for (var i = 0; i < enemies.length; i++) {
        drawEnemy(enemies[i]);
    }

    for (var i = 0; i < bosses.length; i++) {
        var boss = bosses[i];
        drawBoss(boss);

        var healthBarWidth = (boss.health / boss.maxHealth) * boss.width;
        drawFillRect(boss.x, boss.y - 5, boss.width, 3, cBrown);
        drawFillRect(boss.x, boss.y - 5, healthBarWidth, 3, cMustard);
    }

    setTextSize(1);
    drawString("Score: " + playerScore, 5, 5);
    drawString("Lives: " + playerLives, 5, 15);
    drawString("Level: " + level, 5, 25);
    drawString("Kaiju Edition", screenWidth - 90, screenHeight - 15);

    if (gameOver && !menuOpen) {
        drawFillRect(30, 40, 200, 80, cBrown);
        drawFillRect(31, 41, 198, 78, cRust);

        setTextSize(2);
        drawString("GAME OVER", 60, 60);
        setTextSize(1);
        drawString("Score: " + playerScore, 80, 80);
        drawString("Level: " + level, 80, 95);
        drawString("Press Select to restart", 55, 110);
    }

    if (menuOpen) {
        drawFillRect(40, 50, 160, 80, cBrown);
        drawFillRect(41, 51, 158, 78, cRust);

        setTextSize(2);
        drawString("PAUSE", 90, 70);

        if (menuSelection === 0) {
            drawFillRect(60, 85, 120, 15, cOrange);
        }
        if (menuSelection === 1) {
            drawFillRect(60, 105, 120, 15, cOrange);
        }

        setTextSize(1);
        drawString("Continue", 80, 90);
        drawString("New Game", 80, 110);
    }
}

fillScreen(bgColor);
setTextColor(cMustard);
setTextSize(2);
drawString('KAIJU SHOOTER', 22, 30);
setTextSize(1);
setTextColor(cCream);
drawString('Scroll/Prev = Left/Right', 5, 62);
drawString('Select = Breath/Menu', 5, 77);
drawString('Press Select to Start', 5, 97);
setTextColor(cOrange);
drawString('Kaiju Edition - orig. MSI', 5, 118);
keyboard.setLongPress(true);

while (true) {
    if (getSelPress()) {
        delay(300);
        break;
    }
}

initStars();
playerX = screenWidth / 2 - playerWidth / 2;
playerY = screenHeight - 30;

while (true) {
    updatePlayer();

    if (gameRunning && !gameOver && !menuOpen) {
        updateStars();
        updateBullets();
        updateEnemies();
        updateBosses();
    }

    drawGame();

    if (getEscPress()) {
        break;
    }

    delay(16);
}
keyboard.setLongPress(false);
