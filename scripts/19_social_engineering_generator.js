// Social Engineering Generator - Create convincing fake networks
// Shows how attackers trick users into connecting to fake APs

function socialEngineeringGenerator() {
    // Mimic legitimate networks users trust
    var fake_networks = [
        {
            ssid: "AWS_Redacted",
            reason: "Looks like corporate network"
        },
        {
            ssid: "Google-Guest",
            reason: "Impersonates trusted company"
        },
        {
            ssid: "Hotel_Lobby_WiFi",
            reason: "Common in travel scenarios"
        },
        {
            ssid: "Apple_Network",
            reason: "Apple users auto-connect"
        },
        {
            ssid: "Microsoft_Update_Center",
            reason: "Looks like system update"
        }
    ];

    var analysis = [];

    for (var i = 0; i < fake_networks.length; i++) {
        var net = fake_networks[i];
        analysis.push({
            fake_ssid: net.ssid,
            target_type: net.reason,
            effectiveness: "High - users recognize brand"
        });
    }

    return {
        attack_type: "Social Engineering Network Analysis",
        fake_networks: analysis.length,
        networks: analysis,
        mitigation: "Users should verify networks with venue staff before connecting"
    };
}

// Run generator
socialEngineeringGenerator();
