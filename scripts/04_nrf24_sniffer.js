// NRF24 Packet Sniffer - Capture 2.4 GHz wireless packets
// Logs keyboard/mouse packets and other wireless device traffic

function nrf24Sniffer() {
    var channels = [0, 12, 25, 37, 50, 63, 76]; // Common nRF24 channels
    var packets = [];
    var scan_duration = 5000; // 5 seconds per channel

    for (var i = 0; i < channels.length; i++) {
        var channel = channels[i];

        // // nrf24.sniff(channel, duration)
        var result = // nrf24.sniff(channel, scan_duration);

        if (result && result.packets) {
            for (var j = 0; j < result.packets.length; j++) {
                packets.push({
                    channel: channel,
                    address: result.packets[j].address,
                    length: result.packets[j].length + " bytes",
                    rssi: result.packets[j].rssi + " dBm"
                });
            }
        }
    }

    return {
        scan_type: "NRF24 Packet Capture",
        channels_scanned: channels.length,
        packets_captured: packets.length,
        results: packets
    };
}

// Run sniffer
nrf24Sniffer();
