// Signal Strength Mapper - Map WiFi RSSI across frequencies
// Creates a visual heat map of signal strength

function signalStrengthMapper() {
    var channels = [1, 6, 11, 13]; // WiFi channels
    var samples_per_channel = 10;
    var results = [];

    for (var c = 0; c < channels.length; c++) {
        var channel = channels[c];
        var rssi_values = [];

        for (var s = 0; s < samples_per_channel; s++) {
            // wifi.scanChannel(channel) returns RSSI
            var rssi = wifi.scanChannel(channel);
            if (rssi) {
                rssi_values.push(rssi);
            }
        }

        // Calculate average
        var avg = 0;
        for (var i = 0; i < rssi_values.length; i++) {
            avg += rssi_values[i];
        }
        avg = avg / rssi_values.length;

        // Determine strength level
        var strength = "Weak";
        if (avg > -60) strength = "Strong";
        else if (avg > -75) strength = "Medium";

        results.push({
            channel: channel,
            average_rssi: Math.round(avg) + " dBm",
            strength: strength,
            samples: rssi_values.length
        });
    }

    return {
        scan_type: "WiFi Signal Strength Map",
        channels_tested: channels.length,
        results: results
    };
}

// Run mapper
signalStrengthMapper();
