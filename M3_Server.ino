#include <Arduino.h>
#include <WebServer.h>

#include "config.h"
#include "app_state.h"
#include "radio_link.h"
#include "wifi_manager.h"
#include "web_ui.h"
#include "debug_console.h"


// globaler Zustand
AppState g_state;

// Webserver-Instanz
WebServer server(80);

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(200);
  
  wifi_setup_with_fallback();
  webui_setup(server);
  dbg_setup();
  radio_init();

}

void loop() {
  webui_loop(server);
  radio_loop();
  dbg_loop();

}