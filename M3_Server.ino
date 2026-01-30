#include <Arduino.h>
#include <WebServer.h>

#include "config.h"
#include "app_state.h"

#include "radio_link.h"

#include "display.h"
#include "encoder.h"

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

  if (!displayInit()) {
    Serial.println("Display init failed!");
    while (true) delay(1000);
  }

  encoderInit();

  displaySetMode(RadioMode::J3E_PLUS);
  displaySetConnected(false);
  displaySetFrequencyHz(14074000UL);

}

void loop() {
  webui_loop(server);
  radio_loop();
  dbg_loop();


  // ---------- Encoder drehen ----------
  int16_t delta = encoderGetAndClearDelta();

  // Viele Encoder liefern 4 Ticks pro Rastung
  static int16_t acc = 0;
  acc += delta;

  while (acc >= ENC_TICKS_PER_DETENT) {
    acc -= ENC_TICKS_PER_DETENT;
    Serial.println("Encoder next");
    displayMenuNext();
  }

  while (acc <= -ENC_TICKS_PER_DETENT) {
    acc += ENC_TICKS_PER_DETENT;
    Serial.println("Encoder prev");
    displayMenuPrev();
  }

  // ---------- Encoder-Taster ----------
  EncButtonEvent ev = encoderGetButtonEvent();

  if (ev == EncButtonEvent::Click) {
    // TODO: echte Menü-Aktion
    // z.B. RX/TX togglen, Step ändern, in Edit-Modus gehen
    Serial.println("Encoder Click");
  }
  else if (ev == EncButtonEvent::LongPress) {
    Serial.println("Encoder LongPress");

    // Demo: Connection togglen
    static bool connected = false;
    connected = !connected;
    displaySetConnected(connected);
  }


  // ---------- Display ----------
  // Zeichnet nur, wenn dirty
  displayTick();

}