// WiFi Channel Analyzer - Find least congested WiFi channels
// Helps find best channels for AP spoofing or analysis

function channelAnalyzer() {
    var wifi_channels = [1, 6, 11, 13]; // 2.4 GHz WiFi channels
    var results = [];
    var samples_per_channel = 5;

    for (var c = 0; c < wifi_channels.length; c++) {
        var channel = wifi_channels[c];
        var network_count = 0;
        var avg_signal = 0;
        var signals = [];

        for (var s = 0; s < samples_per_channel; s++) {
            var scan = wifi.scanChannel(channel);
            if (scan) {
                network_count += scan.networks || 0;
                var signal = scan.rssi || -100;
                signals.push(signal);
                avg_signal += signal;
            }
        }

        avg_signal = avg_signal / samples_per_channel;

        var congestion = "High";
        if (network_count < 3) congestion = "Low";
        else if (network_count < 5) congestion = "Medium";

        results.push({
            channel: channel,
            networks_detected: network_count,
            average_signal: Math.round(avg_signal) + " dBm",
            congestion: congestion,
            recommended: congestion === "Low" ? "Yes" : "No"
        });
    }

    return {
        scan_type: "WiFi Channel Analysis",
        channels_analyzed: wifi_channels.length,
        results: results,
        recommendation: "Use channels with 'Low' congestion for best results"
    };
}

// Run analyzer
channelAnalyzer();
