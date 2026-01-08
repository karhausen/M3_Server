#pragma once
#include <Arduino.h>
#include <WiFi.h>

struct WiFiStatusInfo {
  bool sta_ok = false;
  bool ap_ok  = false;
  String wifi_mode;   // "STA" / "AP" / "AP+STA" / "OFF"
  String sta_ip;      // "" wenn nicht verbunden
  String ap_ip;       // "" wenn AP nicht läuft
};

void wifi_setup_with_fallback();
WiFiStatusInfo wifi_get_status();