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

const char *const WIFI_TOOLS[] = {"Scan networks", "Channel analyzer", "Packet sniffer", "Beacon spam"};
constexpr int WIFI_TOOL_COUNT = sizeof(WIFI_TOOLS) / sizeof(WIFI_TOOLS[0]);
constexpr int WIFI_TOOL_SCAN = 0;
constexpr int WIFI_TOOL_CHANNELS = 1;
constexpr int WIFI_TOOL_SNIFF = 2;
constexpr int WIFI_TOOL_BEACON = 3;

constexpr int WIFI_VISIBLE_ROWS = 8;
constexpr int CH_FIRST = 1;
constexpr int CH_LAST = 13;
constexpr uint32_t CH_DWELL_MS = 300;
constexpr int BAR_BASE_Y = 108;
constexpr int BAR_MAX_H = 72;

enum class Screen { Menu, Placeholder, WifiMenu, WifiScan, WifiChannels, WifiSniff, WifiBeacon };

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

void startPcap() {
    if (!sdReady) {
        SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
        sdReady = SD.begin(SD_CS_PIN, SPI, 25000000);
    }
    if (!sdReady) {
        snprintf(pcapMsg, sizeof(pcapMsg), "no SD card found");
        return;
    }
    if (!SD.exists("/pcap")) SD.mkdir("/pcap");
    snprintf(pcapName, sizeof(pcapName), "kaiju_%lu.pcap", (unsigned long)(millis() / 1000));
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
        startPcap();
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

const char *const BEACON_SSIDS[] = {"KaijuLab-1", "KaijuLab-2", "KaijuLab-3", "KaijuLab-4"};
constexpr int BEACON_SSID_COUNT = sizeof(BEACON_SSIDS) / sizeof(BEACON_SSIDS[0]);
constexpr uint32_t BEACON_PERIOD_MS = 100;
constexpr uint32_t BEACON_REDRAW_MS = 1000;
constexpr size_t BEACON_FRAME_MAX = 128;

bool beaconRunning = false;
int beaconCh = 6;
uint32_t beaconSent = 0;
uint32_t beaconErrors = 0;
uint32_t beaconLastSend = 0;
uint32_t beaconLastDraw = 0;

size_t buildBeacon(uint8_t *f, const char *ssid, uint8_t idx, uint8_t ch) {
    static const uint8_t RATES[] = {0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24};
    uint8_t hdr[24] = {0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                       0x02, 0x4b, 0x41, 0x49, 0x00, idx, 0x02, 0x4b, 0x41, 0x49, 0x00, idx,
                       0x00, 0x00};
    size_t p = 0;
    memcpy(f, hdr, sizeof(hdr));
    p = sizeof(hdr);
    memset(f + p, 0, 8);
    p += 8;
    f[p++] = 0x64;
    f[p++] = 0x00;
    f[p++] = 0x01;
    f[p++] = 0x04;
    size_t n = strlen(ssid);
    f[p++] = 0x00;
    f[p++] = static_cast<uint8_t>(n);
    memcpy(f + p, ssid, n);
    p += n;
    f[p++] = 0x01;
    f[p++] = sizeof(RATES);
    memcpy(f + p, RATES, sizeof(RATES));
    p += sizeof(RATES);
    f[p++] = 0x03;
    f[p++] = 0x01;
    f[p++] = ch;
    return p;
}

void sendBeacons() {
    uint8_t frame[BEACON_FRAME_MAX];
    for (int i = 0; i < BEACON_SSID_COUNT; i++) {
        size_t len = buildBeacon(frame, BEACON_SSIDS[i], static_cast<uint8_t>(i), static_cast<uint8_t>(beaconCh));
        if (esp_wifi_80211_tx(WIFI_IF_STA, frame, static_cast<int>(len), true) == ESP_OK) {
            beaconSent++;
        } else {
            beaconErrors++;
        }
    }
}

void drawBeacon() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(1);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    char line[48];
    snprintf(line, sizeof(line), "Beacon spam  ch%-2d  %s", beaconCh, beaconRunning ? "RUNNING" : "stopped");
    d.drawString(line, 4, 4);
    snprintf(line, sizeof(line), "Sent %lu  errors %lu", (unsigned long)beaconSent, (unsigned long)beaconErrors);
    d.drawString(line, 4, 20);
    for (int i = 0; i < BEACON_SSID_COUNT; i++) {
        d.drawString(BEACON_SSIDS[i], 12, 36 + i * 12);
    }
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("Enter start/stop  ;. ch  Bksp back", 4, 124);
}

void startBeacon() {
    WiFi.mode(WIFI_STA);
    setAnalyzerChannel(beaconCh);
    beaconSent = 0;
    beaconErrors = 0;
    beaconLastSend = 0;
    beaconLastDraw = 0;
    beaconRunning = true;
    drawBeacon();
}

void stopBeacon() {
    beaconRunning = false;
}

void tickBeacon() {
    if (!beaconRunning) return;
    uint32_t now = millis();
    if (now - beaconLastSend >= BEACON_PERIOD_MS) {
        beaconLastSend = now;
        sendBeacons();
    }
    if (now - beaconLastDraw >= BEACON_REDRAW_MS) {
        beaconLastDraw = now;
        drawBeacon();
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
    if (screen == Screen::WifiBeacon) tickBeacon();
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
                screen = Screen::WifiBeacon;
                drawBeacon();
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

    case Screen::WifiBeacon:
        if (back) {
            stopBeacon();
            screen = Screen::WifiMenu;
            drawWifiMenu();
        } else if (ks.enter) {
            if (beaconRunning) {
                stopBeacon();
            } else {
                startBeacon();
            }
            drawBeacon();
        } else if (up) {
            beaconCh = beaconCh <= CH_FIRST ? CH_LAST : beaconCh - 1;
            setAnalyzerChannel(beaconCh);
            drawBeacon();
        } else if (down) {
            beaconCh = beaconCh >= CH_LAST ? CH_FIRST : beaconCh + 1;
            setAnalyzerChannel(beaconCh);
            drawBeacon();
        }
        break;
    }
}
