#include "encoder_config.h"

#include "encoder.h"
#include "config_display.h"
#include "display.h"

// ---- Quadrature Decode (Interrupt) ----
// Zustandsmaschine (Gray code)
static volatile int16_t g_delta = 0;
static volatile uint8_t g_lastAB = 0;

// Button
static bool g_btnLast = true; // pullup => true = nicht gedrückt
static uint32_t g_btnDownMs = 0;
static EncButtonEvent g_btnEvent = EncButtonEvent::None;

// Debounce/Longpress
static constexpr uint32_t BTN_DEBOUNCE_MS  = ENC_BTN_DEBOUNCE_MS;
static constexpr uint32_t BTN_LONGPRESS_MS = ENC_BTN_LONGPRESS_MS;


static uint32_t g_btnLastChangeMs = 0;

static inline uint8_t readAB() {
  uint8_t a = (uint8_t)digitalRead(ENC_A);
  uint8_t b = (uint8_t)digitalRead(ENC_B);
  return (a << 1) | b;
}

// Lookup: Übergänge -> -1/0/+1
// Index = (old<<2) | new
static const int8_t TRANSITION_TABLE[16] = {
  0, -1, +1, 0,
  +1, 0, 0, -1,
  -1, 0, 0, +1,
  0, +1, -1, 0
};

static void IRAM_ATTR isrEnc() {
  uint8_t ab = readAB();
  uint8_t idx = (g_lastAB << 2) | ab;
  int8_t step = TRANSITION_TABLE[idx];
  g_lastAB = ab;

  if (step != 0) {
    g_delta += step;
  }
}

void encoderInit() {
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_BTN, INPUT_PULLUP);

  g_lastAB = readAB();

  // Beide Kanäle triggern
  attachInterrupt(digitalPinToInterrupt(ENC_A), isrEnc, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), isrEnc, CHANGE);
}

int16_t encoderGetAndClearDelta() {
  noInterrupts();
  int16_t d = g_delta;
  g_delta = 0;
  interrupts();

  // Viele Encoder liefern 4 Counts pro Rastung.
  // Wenn du 1 "Klick" pro Rastung willst, teile durch 4.
  // Lass es erstmal "roh" und entscheide später.
  return d;
}

EncButtonEvent encoderGetButtonEvent() {
  // Polling mit Debounce
  bool now = digitalRead(ENC_BTN); // pullup: true = offen, false = gedrückt
  uint32_t t = millis();

  if (now != g_btnLast && (t - g_btnLastChangeMs) > BTN_DEBOUNCE_MS) {
    g_btnLastChangeMs = t;
    g_btnLast = now;

    if (!now) {
      // gedrückt
      g_btnDownMs = t;
    } else {
      // losgelassen
      uint32_t held = t - g_btnDownMs;
      if (held >= BTN_LONGPRESS_MS) g_btnEvent = EncButtonEvent::LongPress;
      else g_btnEvent = EncButtonEvent::Click;
    }
  }

  EncButtonEvent e = g_btnEvent;
  g_btnEvent = EncButtonEvent::None;
  return e;
}

EncoderEvent encoderPoll() {
  EncoderEvent e{};
  e.steps = 0;
  e.button = EncButtonEvent::None;

  // ---- Drehung ----
  int16_t delta = encoderGetAndClearDelta();
  static int16_t acc = 0;
  acc += delta;

  while (acc >= ENC_TICKS_PER_DETENT) {
    acc -= ENC_TICKS_PER_DETENT;
    e.steps++;
  }
  while (acc <= -ENC_TICKS_PER_DETENT) {
    acc += ENC_TICKS_PER_DETENT;
    e.steps--;
  }

  // ---- Button ----
  e.button = encoderGetButtonEvent();
  return e;
}