#include <Arduino.h>
#include <WebServer.h>

#include "config.h"
#include "app_state.h"
#include "wifi_manager.h"
#include "web_ui.h"

// globaler Zustand
AppState g_state;

// Webserver-Instanz
WebServer server(80);

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(200);

  wifi_setup_with_fallback();
  webui_setup(server);
}

void loop() {
  webui_loop(server);
}