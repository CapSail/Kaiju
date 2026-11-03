#include <cctype>
#include <M5Cardputer.h>

namespace {

constexpr uint16_t COLOR_PRIMARY = 0xD69D;   // 70s beige
constexpr uint16_t COLOR_SECONDARY = 0xB5AD; // darker tan
constexpr uint16_t COLOR_BG = TFT_BLACK;

const char *const MENU_ITEMS[] = {"WiFi", "BLE", "RF 433MHz", "NRF24 2.4GHz", "IR", "RFID", "Files", "Settings"};
constexpr int MENU_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);

int selected = 0;
bool inItem = false;

void drawSplash() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    d.setTextSize(3);
    d.drawString("Kaiju", 60, 30);
    d.setTextSize(1);
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("Cappy's Play", 75, 90);
}

void drawMenu() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(2);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    d.drawString("KAIJU", 8, 4);
    d.setTextSize(1);
    for (int i = 0; i < MENU_COUNT; i++) {
        int y = 32 + i * 12;
        if (i == selected) {
            d.fillRect(0, y - 1, d.width(), 11, COLOR_PRIMARY);
            d.setTextColor(COLOR_BG, COLOR_PRIMARY);
        } else {
            d.setTextColor(COLOR_PRIMARY, COLOR_BG);
        }
        d.drawString(MENU_ITEMS[i], 10, y);
    }
}

void drawItem() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(2);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    d.drawString(MENU_ITEMS[selected], 8, 20);
    d.setTextSize(1);
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("Not built yet.", 8, 60);
    d.drawString("Backspace: back", 8, 110);
}

bool wordHas(const std::vector<char> &word, char c) {
    for (char w : word) {
        if (tolower(w) == c) return true;
    }
    return false;
}

}  // namespace

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);
    drawSplash();
    delay(2000);
    drawMenu();
}

void loop() {
    M5Cardputer.update();
    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return;

    auto &ks = M5Cardputer.Keyboard.keysState();
    bool up = ks.up || wordHas(ks.word, 'w');
    bool down = ks.down || wordHas(ks.word, 's');
    bool back = ks.backspace || ks.esc;

    if (inItem) {
        if (back) {
            inItem = false;
            drawMenu();
        }
        return;
    }

    if (up) {
        selected = (selected + MENU_COUNT - 1) % MENU_COUNT;
        drawMenu();
    } else if (down) {
        selected = (selected + 1) % MENU_COUNT;
        drawMenu();
    } else if (ks.enter) {
        inItem = true;
        drawItem();
    }
}
