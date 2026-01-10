#pragma once
#include <Arduino.h>

struct StaCredentials {
  String ssid;
  String pass;
  bool valid() const { return ssid.length() > 0; }
};

StaCredentials wifi_cfg_load();
bool wifi_cfg_save(const String& ssid, const String& pass);
bool wifi_cfg_clear();  // optional