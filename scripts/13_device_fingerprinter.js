// Device Fingerprinter - Identify device types from network behavior
// Analyzes WiFi and BLE signatures to guess device types

function deviceFingerprinter() {
    var devices = [];

    // WiFi scan for device patterns
    var networks = wifi.scan(3000);
    if (networks) {
        for (var i = 0; i < networks.length; i++) {
            var net = networks[i];

            var device_type = "Unknown";
            if (net.ssid.indexOf("Galaxy") >= 0) device_type = "Samsung Phone";
            if (net.ssid.indexOf("iPhone") >= 0) device_type = "Apple Phone";
            if (net.ssid.indexOf("Alexa") >= 0) device_type = "Amazon Alexa";
            if (net.ssid.indexOf("Google") >= 0) device_type = "Google Device";
            if (net.ssid.indexOf("Chromecast") >= 0) device_type = "Chromecast";
            if (net.ssid.indexOf("TP-LINK") >= 0) device_type = "TP-Link Router";

            devices.push({
                ssid: net.ssid,
                bssid: net.bssid,
                signal: net.rssi + " dBm",
                estimated_device: device_type
            });
        }
    }

    // BLE device analysis
    var ble_scan = ble.startScan(3000);
    if (ble_scan && ble_scan.devices) {
        for (var b = 0; b < ble_scan.devices.length; b++) {
            var ble_dev = ble_scan.devices[b];
            var ble_type = "Unknown BLE Device";

            // Fingerprint by manufacturer or service UUIDs
            if (ble_dev.manufacturer === 0x004C) ble_type = "Apple Device";
            if (ble_dev.manufacturer === 0x0059) ble_type = "Samsung Device";

            devices.push({
                address: ble_dev.address,
                name: ble_dev.name,
                type: "BLE",
                estimated_device: ble_type
            });
        }
    }

    return {
        scan_type: "Device Fingerprinting",
        total_devices: devices.length,
        devices: devices
    };
}

// Run fingerprinter
deviceFingerprinter();
