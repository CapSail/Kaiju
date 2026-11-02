// NRF24 Keyboard Monitor - Watch for wireless keyboard packets
// Identifies and logs keyboard traffic for analysis

function nrf24KeyboardMonitor() {
    var keyboard_channels = [37, 38, 39]; // Common Logitech/Microsoft channels
    var capture_duration = 10000; // 10 seconds
    var keyboard_packets = [];

    for (var c = 0; c < keyboard_channels.length; c++) {
        var channel = keyboard_channels[c];

        // // nrf24.sniff(channel, duration, filter) - filter for keyboard packets
        var result = // nrf24.sniff(channel, capture_duration, "keyboard");

        if (result && result.packets) {
            for (var p = 0; p < result.packets.length; p++) {
                keyboard_packets.push({
                    channel: channel,
                    address: result.packets[p].address,
                    packet_data: result.packets[p].hex || "[raw data]",
                    timestamp: result.packets[p].time
                });
            }
        }
    }

    return {
        scan_type: "NRF24 Keyboard Packet Capture",
        channels_monitored: keyboard_channels.length,
        keyboard_packets: keyboard_packets.length,
        packets: keyboard_packets,
        note: "Can be used to identify wireless keyboard protocol"
    };
}

// Run monitor
nrf24KeyboardMonitor();
