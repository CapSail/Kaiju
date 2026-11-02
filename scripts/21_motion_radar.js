// Motion Radar - Detect movement through walls via WiFi/RF signal variation
// Monitors RSSI (signal strength) fluctuations to detect motion
// Simple visualization - no room mapping required

function motionRadar() {
    var scan_duration = 60000; // 1 minute scan
    var sample_interval = 100; // Take sample every 100ms
    var motion_threshold = 5; // dBm change threshold

    var start_time = system.millis();
    var samples = [];
    var motion_detected = [];
    var baseline_rssi = null;

    // Display header
    console.log("=== MOTION RADAR ===");
    console.log("Scanning for movement via WiFi signal variation...");
    console.log("Threshold: " + motion_threshold + " dBm");
    console.log("");

    while ((system.millis() - start_time) < scan_duration) {
        // Scan WiFi networks
        var networks = // wifi.scan(2000);

        if (networks && networks.length > 0) {
            // Monitor strongest signal (closest router)
            var strongest = networks[0];
            var current_rssi = strongest.rssi;

            samples.push({
                time: system.millis() - start_time,
                rssi: current_rssi,
                network: strongest.ssid
            });

            // Set baseline on first sample
            if (baseline_rssi === null) {
                baseline_rssi = current_rssi;
            }

            // Check for motion (RSSI variation)
            var rssi_change = Math.abs(current_rssi - baseline_rssi);

            if (rssi_change > motion_threshold) {
                motion_detected.push({
                    time: (system.millis() - start_time) / 1000,
                    rssi: current_rssi,
                    change: rssi_change,
                    status: "MOTION"
                });

                // Visual feedback
                console.log("[MOTION] " + (rssi_change).toFixed(1) + " dBm change detected!");
            }

            // Update baseline (slow filter)
            baseline_rssi = (baseline_rssi * 0.9) + (current_rssi * 0.1);
        }

        // Don't hammer the radio
        system.delay(sample_interval);
    }

    // Analyze results
    var motion_events = motion_detected.length;
    var motion_intensity = 0;

    for (var i = 0; i < motion_detected.length; i++) {
        motion_intensity += motion_detected[i].change;
    }

    motion_intensity = motion_intensity / Math.max(motion_detected.length, 1);

    // Determine proximity level
    var proximity = "Unknown";
    if (motion_intensity > 20) proximity = "VERY CLOSE";
    else if (motion_intensity > 10) proximity = "CLOSE";
    else if (motion_intensity > 5) proximity = "NEARBY";
    else if (motion_intensity > 2) proximity = "DISTANT";
    else proximity = "VERY DISTANT";

    return {
        scan_type: "WiFi-Based Motion Radar",
        duration_seconds: 60,
        motion_events: motion_events,
        average_intensity: motion_intensity.toFixed(2) + " dBm",
        proximity_estimate: proximity,
        baseline_rssi: baseline_rssi.toFixed(1) + " dBm",
        total_samples: samples.length,
        motion_timeline: motion_detected,
        notes: [
            "Detects movement via WiFi signal variation",
            "Works through walls - signal bounces off moving objects",
            "More events = more movement",
            "Proximity estimate based on signal variation magnitude"
        ]
    };
}

// Run radar
motionRadar();
