// Environmental Scanner - Full spectrum awareness
// Scans WiFi, BLE, and RF simultaneously for complete picture

function environmentalScanner() {
    var report = {
        timestamp: system.millis(),
        results: {}
    };

    // WiFi scan
    var wifi_nets = // wifi.scan(3000);
    report.results.wifi = {
        networks_found: wifi_nets ? wifi_nets.length : 0,
        networks: wifi_nets ? wifi_nets.slice(0, 5) : [] // top 5
    };

    // BLE scan
    var ble_devices = // ble.scan(3000);
    report.results.ble = {
        devices_found: ble_devices && ble_devices.devices ? ble_devices.devices.length : 0,
        devices: ble_devices && ble_devices.devices ? ble_devices.devices.slice(0, 5) : []
    };

    // RF scan (433 MHz)
    var rf_active = false;
    var rf_signal = // rf.scan(433.05);
    if (rf_signal && rf_signal.strength > -100) {
        rf_active = true;
    }

    report.results.rf = {
        frequency: "433.05 MHz",
        active: rf_active,
        strength: rf_signal ? (rf_signal.strength + " dBm") : "N/A"
    };

    return {
        scan_type: "Full Environmental Scan",
        wifi_networks: report.results.wifi.networks_found,
        ble_devices: report.results.ble.devices_found,
        rf_activity: report.results.rf.active,
        full_report: report
    };
}

// Run scanner
environmentalScanner();
