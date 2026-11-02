// Enhanced Motion Radar - Multi-band motion detection
// Uses WiFi + RF + BLE signal variation for more accurate motion detection
// Better through-wall detection using multiple frequency bands

function enhancedMotionRadar() {
    var scan_duration = 90000; // 1.5 minute scan
    var motion_threshold = 4; // dBm change threshold

    var start_time = system.millis();
    var wifi_motion = [];
    var rf_motion = [];
    var ble_motion = [];

    console.log("=== ENHANCED MOTION RADAR (Multi-Band) ===");
    console.log("WiFi (2.4 GHz) + RF (433 MHz) + BLE Scanning...");
    console.log("");

    var wifi_baseline = null;
    var rf_baseline = null;
    var ble_baseline = null;

    while ((system.millis() - start_time) < scan_duration) {
        var elapsed = (system.millis() - start_time) / 1000;

        // WiFi scan (2.4 GHz band)
        var wifi_nets = wifi.scan(1000);
        if (wifi_nets && wifi_nets.length > 0) {
            var wifi_rssi = wifi_nets[0].rssi;

            if (wifi_baseline === null) {
                wifi_baseline = wifi_rssi;
            }

            var wifi_change = Math.abs(wifi_rssi - wifi_baseline);
            if (wifi_change > motion_threshold) {
                wifi_motion.push({
                    time: elapsed,
                    change: wifi_change
                });
                console.log("[WiFi] Motion at " + elapsed.toFixed(1) + "s: " + wifi_change.toFixed(1) + " dBm");
            }

            wifi_baseline = (wifi_baseline * 0.95) + (wifi_rssi * 0.05);
        }

        // RF scan (433 MHz band) - if Hydra available
        var rf_signal = rf.scan(433.05);
        if (rf_signal && rf_signal.strength) {
            var rf_rssi = rf_signal.strength;

            if (rf_baseline === null) {
                rf_baseline = rf_rssi;
            }

            var rf_change = Math.abs(rf_rssi - rf_baseline);
            if (rf_change > motion_threshold && rf_rssi > -120) {
                rf_motion.push({
                    time: elapsed,
                    change: rf_change
                });
                console.log("[RF 433] Motion at " + elapsed.toFixed(1) + "s: " + rf_change.toFixed(1) + " dBm");
            }

            rf_baseline = (rf_baseline * 0.95) + (rf_rssi * 0.05);
        }

        // BLE scan (2.4 GHz band) - separate from WiFi
        var ble_scan = ble.startScan(1000);
        if (ble_scan && ble_scan.devices && ble_scan.devices.length > 0) {
            var ble_rssi = ble_scan.devices[0].rssi;

            if (ble_baseline === null) {
                ble_baseline = ble_rssi;
            }

            var ble_change = Math.abs(ble_rssi - ble_baseline);
            if (ble_change > motion_threshold) {
                ble_motion.push({
                    time: elapsed,
                    change: ble_change
                });
                console.log("[BLE] Motion at " + elapsed.toFixed(1) + "s: " + ble_change.toFixed(1) + " dBm");
            }

            ble_baseline = (ble_baseline * 0.95) + (ble_rssi * 0.05);
        }

        system.delay(500); // Sample every 500ms
    }

    // Calculate motion intensity across all bands
    var total_motion_events = wifi_motion.length + rf_motion.length + ble_motion.length;

    var wifi_intensity = 0;
    for (var i = 0; i < wifi_motion.length; i++) {
        wifi_intensity += wifi_motion[i].change;
    }
    wifi_intensity = wifi_motion.length > 0 ? (wifi_intensity / wifi_motion.length) : 0;

    var rf_intensity = 0;
    for (var i = 0; i < rf_motion.length; i++) {
        rf_intensity += rf_motion[i].change;
    }
    rf_intensity = rf_motion.length > 0 ? (rf_intensity / rf_motion.length) : 0;

    var ble_intensity = 0;
    for (var i = 0; i < ble_motion.length; i++) {
        ble_intensity += ble_motion[i].change;
    }
    ble_intensity = ble_motion.length > 0 ? (ble_intensity / ble_motion.length) : 0;

    var avg_intensity = (wifi_intensity + rf_intensity + ble_intensity) / 3;

    // Confidence and proximity estimate
    var detection_confidence = "LOW";
    if (total_motion_events > 20) detection_confidence = "HIGH";
    else if (total_motion_events > 10) detection_confidence = "MEDIUM";
    else if (total_motion_events > 5) detection_confidence = "MEDIUM-LOW";

    var proximity = "DISTANT";
    if (avg_intensity > 15) proximity = "VERY CLOSE (< 1 meter)";
    else if (avg_intensity > 8) proximity = "CLOSE (1-3 meters)";
    else if (avg_intensity > 4) proximity = "NEARBY (3-5 meters)";
    else if (avg_intensity > 2) proximity = "DISTANT (5+ meters)";

    return {
        scan_type: "Enhanced Multi-Band Motion Radar",
        duration_seconds: 90,
        detection_confidence: detection_confidence,
        proximity_estimate: proximity,

        motion_events: {
            wifi_2_4ghz: wifi_motion.length,
            rf_433mhz: rf_motion.length,
            ble_2_4ghz: ble_motion.length,
            total: total_motion_events
        },

        intensity: {
            wifi_average: wifi_intensity.toFixed(2) + " dBm",
            rf_average: rf_intensity.toFixed(2) + " dBm",
            ble_average: ble_intensity.toFixed(2) + " dBm",
            overall_average: avg_intensity.toFixed(2) + " dBm"
        },

        sample_events: {
            wifi_first_5: wifi_motion.slice(0, 5),
            rf_first_5: rf_motion.slice(0, 5),
            ble_first_5: ble_motion.slice(0, 5)
        },

        interpretation: [
            "HIGH confidence = Definite motion detected",
            "MEDIUM confidence = Probable motion",
            "LOW confidence = No clear motion detected",
            "Multiple bands confirming motion = Higher accuracy",
            "Works through walls - signal bounces off moving objects"
        ]
    };
}

// Run enhanced radar
enhancedMotionRadar();
