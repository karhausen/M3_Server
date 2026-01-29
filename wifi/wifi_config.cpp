#include "wifi_config.h"
#include <Preferences.h>

static const char* NS = "cfg";
static const char* KEY_SSID = "ssid";
static const char* KEY_PASS = "pass";

StaCredentials wifi_cfg_load() {
  Preferences pref;
  StaCredentials c;
  if (!pref.begin(NS, true)) return c;

  c.ssid = pref.getString(KEY_SSID, "");
  c.pass = pref.getString(KEY_PASS, "");
  pref.end();
  return c;
}

bool wifi_cfg_save(const String& ssid, const String& pass) {
  Preferences pref;
  if (!pref.begin(NS, false)) return false;

  bool ok = true;
  ok &= pref.putString(KEY_SSID, ssid) > 0;
  // Passwort darf auch leer sein (offenes WLAN) -> dann trotzdem speichern
  pref.putString(KEY_PASS, pass);
  pref.end();
  return ok;
}

bool wifi_cfg_clear() {
  Preferences pref;
  if (!pref.begin(NS, false)) return false;
  pref.remove(KEY_SSID);
  pref.remove(KEY_PASS);
  pref.end();
  return true;
}