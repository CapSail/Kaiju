// IR Blaster Helper - Send common IR codes for TVs and devices
// Useful for party tricks and device control

function irBlasterHelper() {
    var ir_codes = {
        tv_power: "0xffe01f",
        tv_volume_up: "0xffe01f",
        tv_volume_down: "0xffe01f",
        tv_mute: "0xffe01f",
        tv_channel_up: "0xffe01f",
        tv_channel_down: "0xffe01f",
        ac_power: "0xff10ef",
        ac_temp_up: "0xff50af",
        ac_temp_down: "0xff30cf",
        lights_on: "0xff00ff",
        lights_off: "0xff00ff"
    };

    var sent = [];

    // ir.send(code) - broadcast IR code
    for (var device in ir_codes) {
        var result = ir.send(ir_codes[device]);
        if (result) {
            sent.push(device);
        }
    }

    return {
        action: "IR Blaster Broadcast",
        codes_available: Object.keys(ir_codes).length,
        codes_sent: sent.length,
        targets: sent,
        status: "Broadcast complete"
    };
}

// Run blaster
irBlasterHelper();
