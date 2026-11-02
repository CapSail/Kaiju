// BLE Device Finder - Locate nearby Bluetooth devices
// Scans and logs all BLE peripherals in range

function bleDeviceFinder() {
    var timeout = 10000; // 10 second scan
    var devices = [];

    // ble.startScan(timeout)
    var scan_result = ble.startScan(timeout);

    if (scan_result && scan_result.devices) {
        for (var i = 0; i < scan_result.devices.length; i++) {
            var device = scan_result.devices[i];
            devices.push({
                address: device.address,
                name: device.name || "[Unknown]",
                rssi: device.rssi + " dBm",
                services: device.services ? device.services.length : 0
            });
        }
    }

    return {
        scan_type: "BLE Scan",
        duration_ms: timeout,
        devices_found: devices.length,
        results: devices
    };
}

// Run finder
bleDeviceFinder();
