// Emergency Stop - Kill all ongoing transmissions
// Safety script to disable all attacks immediately

function emergencyStop() {
    var stopped = [];

    // Stop WiFi operations
    if (wifi.stop) {
        wifi.stop();
        stopped.push("WiFi");
    }

    // Stop BLE broadcasting
    if (ble.stop) {
        ble.stop();
        stopped.push("BLE");
    }

    // Stop RF transmission
    if (rf.stop) {
        rf.stop();
        stopped.push("RF");
    }

    // Stop NRF24
    if (nrf24.stop) {
        nrf24.stop();
        stopped.push("NRF24");
    }

    // Stop IR blasting
    if (ir.stop) {
        ir.stop();
        stopped.push("IR");
    }

    return {
        action: "EMERGENCY STOP",
        timestamp: system.millis(),
        transmitters_stopped: stopped.length,
        stopped_list: stopped,
        status: "All transmissions disabled",
        message: "Device is now in safe mode"
    };
}

// Run emergency stop
emergencyStop();
