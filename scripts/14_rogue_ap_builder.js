// Rogue AP Builder - Create fake networks to capture traffic
// Educational: Shows how man-in-the-middle attacks work

function rogueAPBuilder() {
    var fake_networks = [
        {
            ssid: "FreeAirport",
            password: "",
            channel: 6,
            is_open: true
        },
        {
            ssid: "Starbucks_WiFi",
            password: "",
            channel: 11,
            is_open: true
        },
        {
            ssid: "Public_Library",
            password: "",
            channel: 1,
            is_open: true
        }
    ];

    var created = [];

    for (var i = 0; i < fake_networks.length; i++) {
        var net = fake_networks[i];

        // wifi.createAP(ssid, channel, password)
        var result = wifi.createAP(net.ssid, net.channel, net.password);

        if (result) {
            created.push({
                ssid: net.ssid,
                channel: net.channel,
                security: net.is_open ? "Open" : "WPA2",
                status: "Broadcasting"
            });
        }
    }

    return {
        action: "Rogue Access Point Creation",
        networks_created: created.length,
        networks: created,
        warning: "Educational only - verify legality in your jurisdiction"
    };
}

// Run builder
rogueAPBuilder();
