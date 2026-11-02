// RF Frequency Scanner - Find active signals on 433 MHz
// Scans common ISM frequencies and reports signal strength

function rfScanner() {
    var freqs = [
        433.05, 433.50, 434.79, // Common ISM
        434.00, 434.40, 434.45 // Regional variants
    ];

    var results = [];

    for (var i = 0; i < freqs.length; i++) {
        var freq = freqs[i];
        // rf.scan(freq) - adjust per Bruce's actual RF API
        var signal = rf.scan(freq);

        if (signal && signal.strength > 10) {
            results.push({
                freq: freq + " MHz",
                rssi: signal.strength + " dBm",
                found: true
            });
        }
    }

    return {
        scan_type: "RF 433 MHz Scanner",
        frequencies_checked: freqs.length,
        signals_found: results.length,
        results: results
    };
}

// Run scan
rfScanner();
