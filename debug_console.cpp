#include "debug_console.h"

#include "app_state.h"
#include "wifi_manager.h"
#include "wifi_config.h"
#include "radio_link.h"

static String lineBuf;

static void printPrompt() {
  Serial.print("> ");
}

static void cmd_help() {
  Serial.println("Commands:");
  Serial.println("  help");
  Serial.println("  get_ssid");
  Serial.println("  get_wifi");
  Serial.println("  get_frequency");
  Serial.println("  get_mode");
  Serial.println("  get_preset");
  Serial.println("  get_radio");
  Serial.println("  connect or disconnect");
  Serial.println("  radio_raw <command>");
  Serial.println("  radio_get_rxfreq");
  Serial.println("  radio_get_preset");
  Serial.println("  reboot");
  Serial.println();
}

static void handleCommand(const String& lineRaw) {
  String line = lineRaw;
  line.trim();
  if (line.length() == 0) return;

  // Split: erstes Wort = cmd, Rest = args
  int sp = line.indexOf(' ');
  String cmd = (sp < 0) ? line : line.substring(0, sp);
  String args = (sp < 0) ? ""   : line.substring(sp + 1);
  cmd.trim();
  args.trim();

  String cmdLower = cmd;
  cmdLower.toLowerCase();

  if (cmdLower == "help" || cmdLower == "?") {
    cmd_help();
  }
  else if (cmdLower == "get_ssid") {
    StaCredentials c = wifi_cfg_load();
    if (c.valid()) {
      Serial.print("ssid=");
      Serial.println(c.ssid);
    } else {
      Serial.println("ssid=<not set>");
    }
  }
  else if (cmdLower == "get_wifi") {
    WiFiStatusInfo w = wifi_get_status();
    Serial.print("wifi_mode="); Serial.println(w.wifi_mode);
    Serial.print("sta_ok=");    Serial.println(w.sta_ok ? "true" : "false");
    Serial.print("sta_ip=");    Serial.println(w.sta_ip.length() ? w.sta_ip : "<none>");
    Serial.print("ap_ok=");     Serial.println(w.ap_ok ? "true" : "false");
    Serial.print("ap_ip=");     Serial.println(w.ap_ip.length() ? w.ap_ip : "<none>");
  }
  else if (cmdLower == "get_frequency") {
    Serial.print("freq_hz=");
    Serial.println((unsigned long)g_state.freq_hz);
  }
  else if (cmdLower == "get_mode") {
    Serial.print("mode=");
    Serial.println(g_state.mode);
  }
  else if (cmdLower == "get_preset") {
    Serial.print("preset=");
    Serial.println(g_state.preset);
  }
  else if (cmdLower == "get_radio") {
    Serial.print("radio_connected=");
    Serial.println(g_state.radio_connected ? "true" : "false");
  }
  else if (cmdLower == "connect") {
    radio_send_connect();
  }
  else if (cmdLower == "disconnect") {
    radio_send_disconnect();
  }
  // ✅ NEU: Radio raw command
  else if (cmdLower == "radio_raw") {
    if (args.length() == 0) {
      Serial.println("Usage: radio_raw <command>");
      Serial.println("Example: radio_raw FF SRF 1500000");
    } else {
      // sendet z.B. "M:" + args + "\r\n" (je nach radio_build/config)
      radio_send_raw(args);
      Serial.println("OK sent: " + args);
      // Serial.println(args);
    }
  }

  // ✅ Praktische Shortcuts (optional, aber nützlich)
  else if (cmdLower == "radio_get_rxfreq") {
    radio_query_rxfreq();
    Serial.println("OK query rx freq");
  }
  else if (cmdLower == "radio_get_preset") {
    radio_query_presetpage();
    Serial.println("OK query preset page");
  }

  else if (cmdLower == "reboot") {
    Serial.println("rebooting...");
    delay(200);
    ESP.restart();
  }
  else {
    Serial.print("Unknown command: ");
    Serial.println(line);
    Serial.println("Type 'help' for a list.");
  }
}

void dbg_setup() {
  lineBuf.reserve(96);
  Serial.println();
  Serial.println("Debug console ready. Type 'help'.");
  printPrompt();
}

void dbg_loop() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    // handle CR/LF robust
    if (c == '\r') continue;
    if (c == '\n') {
      Serial.println(); // echo newline nicely
      handleCommand(lineBuf);
      lineBuf = "";
      printPrompt();
      continue;
    }

    // simple line editing
    if (c == 0x08 || c == 0x7F) { // backspace
      if (lineBuf.length() > 0) {
        lineBuf.remove(lineBuf.length() - 1);
        Serial.print("\b \b");
      }
      continue;
    }

    // printable chars only
    if (c >= 32 && c <= 126) {
      if (lineBuf.length() < 120) {
        lineBuf += c;
        Serial.print(c); // echo
      }
    }
  }
}
