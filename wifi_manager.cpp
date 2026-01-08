#include "wifi_manager.h"
#include "config.h"

static bool staConnected = false;
static bool apStarted = false;

static bool connectSTA_withTimeout(uint32_t timeoutMs) {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(STA_SSID, STA_PASS);

  Serial.printf("STA: connecting to '%s' ...\n", STA_SSID);

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < timeoutMs) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("STA: connected!");
    Serial.print("STA IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("STA: connect timeout.");
  return false;
}

static bool startAP() {
  if (KEEP_AP_ALSO_WHEN_STA_OK && staConnected) WiFi.mode(WIFI_AP_STA);
  else WiFi.mode(WIFI_AP);

  WiFi.setSleep(false);

  IPAddress apIP(192,168,4,1);
  IPAddress gw(192,168,4,1);
  IPAddress mask(255,255,255,0);
  WiFi.softAPConfig(apIP, gw, mask);

  Serial.printf("AP: starting '%s' ...\n", AP_SSID);

  bool ok = WiFi.softAP(AP_SSID, AP_PASS, AP_CH, AP_HIDE);
  if (!ok) {
    Serial.println("AP: failed to start!");
    return false;
  }

  Serial.println("AP: started!");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  return true;
}

void wifi_setup_with_fallback() {
  Serial.println();
  Serial.println("=== WiFi setup ===");

  staConnected = connectSTA_withTimeout(STA_TIMEOUT_MS);

  if (!staConnected) {
    apStarted = startAP();
  } else {
    if (KEEP_AP_ALSO_WHEN_STA_OK) apStarted = startAP();
    else {
      apStarted = false;
      Serial.println("AP: not started (STA OK).");
    }
  }

  Serial.printf("Result: STA=%s, AP=%s\n",
                staConnected ? "ON" : "OFF",
                apStarted ? "ON" : "OFF");
}

WiFiStatusInfo wifi_get_status() {
  WiFiStatusInfo info;
  info.sta_ok = (WiFi.status() == WL_CONNECTED);
  info.ap_ok = (WiFi.getMode() == WIFI_MODE_AP || WiFi.getMode() == WIFI_MODE_APSTA);

  wifi_mode_t mode = WiFi.getMode();
  if (mode == WIFI_MODE_STA) info.wifi_mode = "STA";
  else if (mode == WIFI_MODE_AP) info.wifi_mode = "AP";
  else if (mode == WIFI_MODE_APSTA) info.wifi_mode = "AP+STA";
  else info.wifi_mode = "OFF";

  info.sta_ip = info.sta_ok ? WiFi.localIP().toString() : "";
  info.ap_ip  = info.ap_ok ? WiFi.softAPIP().toString() : "";
  return info;
}