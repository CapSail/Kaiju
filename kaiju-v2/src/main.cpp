#include <cctype>
#include <cstdio>
#include <M5Cardputer.h>
#include <WiFi.h>
#include "esp_wifi.h"

namespace {

constexpr uint16_t COLOR_PRIMARY = 0xD5B1;   // 70s tan #D2B48C
constexpr uint16_t COLOR_SECONDARY = 0x9BCA; // brown #9C7A52
constexpr uint16_t COLOR_BG = TFT_BLACK;

const char *const MENU_ITEMS[] = {"WiFi", "BLE", "RF 433MHz", "NRF24 2.4GHz", "IR", "RFID", "Files", "Settings"};
constexpr int MENU_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);
constexpr int WIFI_ITEM = 0;

const char *const WIFI_TOOLS[] = {"Scan networks", "Channel analyzer", "Packet sniffer"};
constexpr int WIFI_TOOL_COUNT = sizeof(WIFI_TOOLS) / sizeof(WIFI_TOOLS[0]);
constexpr int WIFI_TOOL_SCAN = 0;
constexpr int WIFI_TOOL_CHANNELS = 1;
constexpr int WIFI_TOOL_SNIFF = 2;

constexpr int WIFI_VISIBLE_ROWS = 8;
constexpr int CH_FIRST = 1;
constexpr int CH_LAST = 13;
constexpr uint32_t CH_DWELL_MS = 300;
constexpr int BAR_BASE_Y = 108;
constexpr int BAR_MAX_H = 72;

enum class Screen { Menu, Placeholder, WifiMenu, WifiScan, WifiChannels, WifiSniff };

Screen screen = Screen::Menu;
int selected = 0;
int wifiToolSel = 0;
int netCount = 0;
int netSel = 0;
int netTop = 0;

volatile uint32_t chFrames[CH_LAST + 1] = {};
volatile uint32_t chBeacons[CH_LAST + 1] = {};
uint32_t sweepFrames[CH_LAST + 1] = {};
uint32_t sweepBeacons[CH_LAST + 1] = {};
int chCurrent = CH_FIRST;
int sweepCount = 0;
uint32_t chDwellStart = 0;

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

void drawWifiMenu() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(2);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    d.drawString("WiFi", 8, 4);
    d.setTextSize(1);
    for (int i = 0; i < WIFI_TOOL_COUNT; i++) {
        int y = 32 + i * 12;
        if (i == wifiToolSel) {
            d.fillRect(0, y - 1, d.width(), 11, COLOR_PRIMARY);
            d.setTextColor(COLOR_BG, COLOR_PRIMARY);
        } else {
            d.setTextColor(COLOR_PRIMARY, COLOR_BG);
        }
        d.drawString(WIFI_TOOLS[i], 10, y);
    }
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("Backspace: back", 4, 124);
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
        if (idx == netSel) {
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

void onPromiscuous(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA) return;
    auto *pkt = static_cast<wifi_promiscuous_pkt_t *>(buf);
    int ch = pkt->rx_ctrl.channel;
    if (ch < CH_FIRST || ch > CH_LAST) return;
    chFrames[ch]++;
    if (type == WIFI_PKT_MGMT && pkt->rx_ctrl.sig_len > 0 && (pkt->payload[0] >> 4) == 0x8) {
        chBeacons[ch]++;
    }
}

void setAnalyzerChannel(int ch) {
    esp_wifi_set_channel(static_cast<uint8_t>(ch), WIFI_SECOND_CHAN_NONE);
}

void drawChannels() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(1);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    char header[40];
    snprintf(header, sizeof(header), "Channels  sweep %d  Bksp: back", sweepCount);
    d.drawString(header, 4, 4);

    if (sweepCount == 0) {
        d.drawString("Listening... first sweep in about 4s.", 4, 60);
        return;
    }

    uint32_t maxFrames = 1;
    int busiest = CH_FIRST;
    for (int ch = CH_FIRST; ch <= CH_LAST; ch++) {
        if (sweepFrames[ch] > maxFrames) maxFrames = sweepFrames[ch];
        if (sweepFrames[ch] > sweepFrames[busiest]) busiest = ch;
    }

    for (int ch = CH_FIRST; ch <= CH_LAST; ch++) {
        int x = 8 + (ch - CH_FIRST) * 17;
        int h = static_cast<int>(sweepFrames[ch] * BAR_MAX_H / maxFrames);
        if (h < 1 && sweepFrames[ch] > 0) h = 1;
        if (h > 0) d.fillRect(x, BAR_BASE_Y - h, 12, h, COLOR_PRIMARY);
        char label[4];
        snprintf(label, sizeof(label), "%d", ch);
        d.setTextColor(COLOR_SECONDARY, COLOR_BG);
        d.drawString(label, x + 2, BAR_BASE_Y + 4);
    }

    char footer[48];
    snprintf(footer, sizeof(footer), "Busiest ch%d: %lu frames, %lu beacons", busiest,
             (unsigned long)sweepFrames[busiest], (unsigned long)sweepBeacons[busiest]);
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString(footer, 4, 124);
}

void startChannelAnalyzer() {
    WiFi.mode(WIFI_STA);
    wifi_promiscuous_filter_t filter = {WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA};
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous_rx_cb(&onPromiscuous);
    for (int ch = 0; ch <= CH_LAST; ch++) {
        chFrames[ch] = 0;
        chBeacons[ch] = 0;
        sweepFrames[ch] = 0;
        sweepBeacons[ch] = 0;
    }
    sweepCount = 0;
    chCurrent = CH_FIRST;
    setAnalyzerChannel(chCurrent);
    esp_wifi_set_promiscuous(true);
    chDwellStart = millis();
    drawChannels();
}

void stopChannelAnalyzer() {
    esp_wifi_set_promiscuous(false);
}

void tickChannelAnalyzer() {
    if (millis() - chDwellStart < CH_DWELL_MS) return;
    chDwellStart = millis();
    chCurrent++;
    if (chCurrent > CH_LAST) {
        chCurrent = CH_FIRST;
        for (int ch = CH_FIRST; ch <= CH_LAST; ch++) {
            sweepFrames[ch] = chFrames[ch];
            sweepBeacons[ch] = chBeacons[ch];
            chFrames[ch] = 0;
            chBeacons[ch] = 0;
        }
        sweepCount++;
        drawChannels();
    }
    setAnalyzerChannel(chCurrent);
}

enum SniffCounter : uint8_t {
    SNIFF_TOTAL,
    SNIFF_MGMT,
    SNIFF_CTRL,
    SNIFF_DATA,
    SNIFF_BEACON,
    SNIFF_PROBE_REQ,
    SNIFF_PROBE_RESP,
    SNIFF_AUTH,
    SNIFF_DEAUTH,
    SNIFF_DISASSOC,
    SNIFF_COUNT
};

volatile uint32_t sniffLive[SNIFF_COUNT] = {};
uint32_t sniffShown[SNIFF_COUNT] = {};
bool sniffHop = true;
int sniffCh = CH_FIRST;
uint32_t sniffDwellStart = 0;
uint32_t sniffRateStart = 0;
uint32_t sniffLastTotal = 0;
uint32_t sniffRate = 0;

void onSniff(void *buf, wifi_promiscuous_pkt_type_t type) {
    sniffLive[SNIFF_TOTAL]++;
    if (type == WIFI_PKT_CTRL) {
        sniffLive[SNIFF_CTRL]++;
        return;
    }
    if (type == WIFI_PKT_DATA) {
        sniffLive[SNIFF_DATA]++;
        return;
    }
    if (type != WIFI_PKT_MGMT) return;
    sniffLive[SNIFF_MGMT]++;
    auto *pkt = static_cast<wifi_promiscuous_pkt_t *>(buf);
    if (pkt->rx_ctrl.sig_len < 1) return;
    switch ((pkt->payload[0] >> 4) & 0xF) {
    case 0x4: sniffLive[SNIFF_PROBE_REQ]++; break;
    case 0x5: sniffLive[SNIFF_PROBE_RESP]++; break;
    case 0x8: sniffLive[SNIFF_BEACON]++; break;
    case 0xA: sniffLive[SNIFF_DISASSOC]++; break;
    case 0xB: sniffLive[SNIFF_AUTH]++; break;
    case 0xC: sniffLive[SNIFF_DEAUTH]++; break;
    default: break;
    }
}

void drawSniffer() {
    for (int i = 0; i < SNIFF_COUNT; i++) sniffShown[i] = sniffLive[i];
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(1);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    char line[48];
    snprintf(line, sizeof(line), "Sniffer  ch%-2d  %s", sniffCh, sniffHop ? "hopping" : "locked");
    d.drawString(line, 4, 4);
    snprintf(line, sizeof(line), "Total %lu  (%lu/s)", (unsigned long)sniffShown[SNIFF_TOTAL],
             (unsigned long)sniffRate);
    d.drawString(line, 4, 18);
    snprintf(line, sizeof(line), "Mgmt %lu  Ctrl %lu  Data %lu", (unsigned long)sniffShown[SNIFF_MGMT],
             (unsigned long)sniffShown[SNIFF_CTRL], (unsigned long)sniffShown[SNIFF_DATA]);
    d.drawString(line, 4, 30);
    snprintf(line, sizeof(line), "Beacon %lu  ProbeReq %lu", (unsigned long)sniffShown[SNIFF_BEACON],
             (unsigned long)sniffShown[SNIFF_PROBE_REQ]);
    d.drawString(line, 4, 42);
    snprintf(line, sizeof(line), "ProbeResp %lu  Auth %lu", (unsigned long)sniffShown[SNIFF_PROBE_RESP],
             (unsigned long)sniffShown[SNIFF_AUTH]);
    d.drawString(line, 4, 54);
    snprintf(line, sizeof(line), "Deauth %lu  Disassoc %lu", (unsigned long)sniffShown[SNIFF_DEAUTH],
             (unsigned long)sniffShown[SNIFF_DISASSOC]);
    d.drawString(line, 4, 66);
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("Enter hop/lock  ;. ch  Bksp back", 4, 124);
}

void startSniffer() {
    WiFi.mode(WIFI_STA);
    wifi_promiscuous_filter_t filter = {WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_CTRL |
                                        WIFI_PROMIS_FILTER_MASK_DATA};
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous_rx_cb(&onSniff);
    for (int i = 0; i < SNIFF_COUNT; i++) {
        sniffLive[i] = 0;
        sniffShown[i] = 0;
    }
    sniffHop = true;
    sniffCh = CH_FIRST;
    setAnalyzerChannel(sniffCh);
    sniffLastTotal = 0;
    sniffRate = 0;
    sniffDwellStart = millis();
    sniffRateStart = sniffDwellStart;
    esp_wifi_set_promiscuous(true);
    drawSniffer();
}

void stopSniffer() {
    esp_wifi_set_promiscuous(false);
}

void tickSniffer() {
    uint32_t now = millis();
    if (sniffHop && now - sniffDwellStart >= CH_DWELL_MS) {
        sniffDwellStart = now;
        sniffCh = sniffCh >= CH_LAST ? CH_FIRST : sniffCh + 1;
        setAnalyzerChannel(sniffCh);
    }
    if (now - sniffRateStart >= 1000) {
        uint32_t total = sniffLive[SNIFF_TOTAL];
        sniffRate = total - sniffLastTotal;
        sniffLastTotal = total;
        sniffRateStart = now;
        drawSniffer();
    }
}

void moveSnifferChannel(int delta) {
    if (sniffHop) return;
    int next = sniffCh + delta;
    if (next < CH_FIRST) next = CH_LAST;
    if (next > CH_LAST) next = CH_FIRST;
    sniffCh = next;
    setAnalyzerChannel(sniffCh);
    drawSniffer();
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
    if (screen == Screen::WifiChannels) tickChannelAnalyzer();
    if (screen == Screen::WifiSniff) tickSniffer();
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
                screen = Screen::WifiMenu;
                wifiToolSel = 0;
                drawWifiMenu();
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

    case Screen::WifiMenu:
        if (back) {
            screen = Screen::Menu;
            drawMenu();
        } else if (up) {
            wifiToolSel = (wifiToolSel + WIFI_TOOL_COUNT - 1) % WIFI_TOOL_COUNT;
            drawWifiMenu();
        } else if (down) {
            wifiToolSel = (wifiToolSel + 1) % WIFI_TOOL_COUNT;
            drawWifiMenu();
        } else if (ks.enter) {
            if (wifiToolSel == WIFI_TOOL_SCAN) {
                screen = Screen::WifiScan;
                scanWifi();
            } else if (wifiToolSel == WIFI_TOOL_CHANNELS) {
                screen = Screen::WifiChannels;
                startChannelAnalyzer();
            } else if (wifiToolSel == WIFI_TOOL_SNIFF) {
                screen = Screen::WifiSniff;
                startSniffer();
            }
        }
        break;

    case Screen::WifiScan:
        if (back) {
            WiFi.scanDelete();
            screen = Screen::WifiMenu;
            drawWifiMenu();
        } else if (up) {
            moveWifiSelection(-1);
        } else if (down) {
            moveWifiSelection(1);
        } else if (ks.enter) {
            scanWifi();
        }
        break;

    case Screen::WifiChannels:
        if (back) {
            stopChannelAnalyzer();
            screen = Screen::WifiMenu;
            drawWifiMenu();
        }
        break;

    case Screen::WifiSniff:
        if (back) {
            stopSniffer();
            screen = Screen::WifiMenu;
            drawWifiMenu();
        } else if (ks.enter) {
            sniffHop = !sniffHop;
            sniffDwellStart = millis();
            drawSniffer();
        } else if (up) {
            moveSnifferChannel(-1);
        } else if (down) {
            moveSnifferChannel(1);
        }
        break;
    }
}
