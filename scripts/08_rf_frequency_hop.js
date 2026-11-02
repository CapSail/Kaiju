// RF Frequency Hopper - Scan multiple frequencies looking for activity
// Useful for finding active radio signals

function rfFrequencyHop() {
    var frequencies = [
        315.00, 433.05, 433.50, 434.00, // ISM bands
        868.00, 915.00 // Regional ISM
    ];

    var results = [];
    var strongest_signal = null;
    var strongest_rssi = -200;

    for (var i = 0; i < frequencies.length; i++) {
        var freq = frequencies[i];

        // // rf.scan(freq) - scan for signal
        var signal = // rf.scan(freq);

        if (signal && signal.strength > -150) {
            results.push({
                frequency: freq + " MHz",
                signal_strength: signal.strength + " dBm",
                active: true
            });

            if (signal.strength > strongest_rssi) {
                strongest_rssi = signal.strength;
                strongest_signal = freq;
            }
        }
    }

    return {
        scan_type: "RF Frequency Hop",
        frequencies_checked: frequencies.length,
        signals_detected: results.length,
        strongest_signal: strongest_signal ? (strongest_signal + " MHz") : "None",
        results: results
    };
}

// Run hopper
rfFrequencyHop();
