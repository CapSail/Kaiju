// Packet Sniffer Logger - Capture and log all wireless packets
// Records WiFi, BLE, RF packets to analyze later

function packetSnifferLogger() {
    var log_file = "/packets_" + system.millis() + ".txt";
    var packets_captured = 0;
    var capture_duration = 60000; // 1 minute

    var start = system.millis();
    var logs = [];

    while ((system.millis() - start) < capture_duration) {
        // WiFi packet capture
        var wifi_pkts = wifi.rawCapture(1000);
        if (wifi_pkts) {
            for (var w = 0; w < wifi_pkts.length; w++) {
                logs.push({
                    type: "WiFi",
                    timestamp: system.millis(),
                    source: wifi_pkts[w].source,
                    destination: wifi_pkts[w].destination,
                    length: wifi_pkts[w].length
                });
                packets_captured++;
            }
        }

        // BLE packet capture
        var ble_pkts = ble.rawCapture(1000);
        if (ble_pkts) {
            for (var b = 0; b < ble_pkts.length; b++) {
                logs.push({
                    type: "BLE",
                    timestamp: system.millis(),
                    address: ble_pkts[b].address,
                    rssi: ble_pkts[b].rssi
                });
                packets_captured++;
            }
        }
    }

    return {
        action: "Packet Sniffer - Raw Capture",
        duration_seconds: 60,
        packets_captured: packets_captured,
        log_file: log_file,
        sample_logs: logs.slice(0, 10)
    };
}

// Run logger
packetSnifferLogger();
