#include <cctype>
#include <cstdio>
#include <M5Cardputer.h>
#include <WiFi.h>

namespace {

constexpr uint16_t COLOR_PRIMARY = 0xD5B1;   // 70s tan #D2B48C
constexpr uint16_t COLOR_SECONDARY = 0x9BCA; // brown #9C7A52
constexpr uint16_t COLOR_BG = TFT_BLACK;

const char *const MENU_ITEMS[] = {"WiFi", "BLE", "RF 433MHz", "NRF24 2.4GHz", "IR", "RFID", "Files", "Settings"};
constexpr int MENU_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);
constexpr int WIFI_ITEM = 0;
constexpr int WIFI_VISIBLE_ROWS = 8;

enum class Screen { Menu, Placeholder, WifiScan };

Screen screen = Screen::Menu;
int selected = 0;
int netCount = 0;
int netSel = 0;
int netTop = 0;

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

void drawPlaceholder() {
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

void drawWifiMessage(const char *msg) {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(1);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    d.drawString(msg, 8, 60);
}

void drawWifiScan() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(1);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    char header[32];
    snprintf(header, sizeof(header), "WiFi: %d networks", netCount);
    d.drawString(header, 4, 4);

    for (int row = 0; row < WIFI_VISIBLE_ROWS; row++) {
        int idx = netTop + row;
        if (idx >= netCount) break;
        int y = 18 + row * 12;
        bool hl = idx == netSel;
        if (hl) {
            d.fillRect(0, y - 1, d.width(), 11, COLOR_PRIMARY);
            d.setTextColor(COLOR_BG, COLOR_PRIMARY);
        } else {
            d.setTextColor(COLOR_PRIMARY, COLOR_BG);
        }
        String ssid = WiFi.SSID(idx);
        if (ssid.length() == 0) ssid = "<hidden>";
        char line[48];
        snprintf(line, sizeof(line), "%-16.16s %4ld dBm ch%-2ld", ssid.c_str(), (long)WiFi.RSSI(idx),
                 (long)WiFi.channel(idx));
        d.drawString(line, 4, y);
    }

    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("Enter: rescan  Bksp: back", 4, 124);
}

void scanWifi() {
    drawWifiMessage("Scanning...");
    WiFi.mode(WIFI_STA);
    WiFi.scanDelete();
    int n = WiFi.scanNetworks();
    netCount = n > 0 ? n : 0;
    netSel = 0;
    netTop = 0;
    drawWifiScan();
}

void moveWifiSelection(int delta) {
    int next = netSel + delta;
    if (next < 0 || next >= netCount) return;
    netSel = next;
    if (netSel < netTop) netTop = netSel;
    if (netSel >= netTop + WIFI_VISIBLE_ROWS) netTop = netSel - WIFI_VISIBLE_ROWS + 1;
    drawWifiScan();
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
    bool up = ks.up || wordHas(ks.word, ';') || wordHas(ks.word, 'w');
    bool down = ks.down || wordHas(ks.word, '.') || wordHas(ks.word, 's');
    bool back = ks.backspace || ks.esc;

    switch (screen) {
    case Screen::Menu:
        if (up) {
            selected = (selected + MENU_COUNT - 1) % MENU_COUNT;
            drawMenu();
        } else if (down) {
            selected = (selected + 1) % MENU_COUNT;
            drawMenu();
        } else if (ks.enter) {
            if (selected == WIFI_ITEM) {
                screen = Screen::WifiScan;
                scanWifi();
            } else {
                screen = Screen::Placeholder;
                drawPlaceholder();
            }
        }
        break;

    case Screen::Placeholder:
        if (back) {
            screen = Screen::Menu;
            drawMenu();
        }
        break;

    case Screen::WifiScan:
        if (back) {
            WiFi.scanDelete();
            screen = Screen::Menu;
            drawMenu();
        } else if (up) {
            moveWifiSelection(-1);
        } else if (down) {
            moveWifiSelection(1);
        } else if (ks.enter) {
            scanWifi();
        }
        break;
    }
}
