#include "web_ui.h"
#include "app_state.h"
#include "wifi_manager.h"
#include "radio_link.h"
#include "web_pages.h"

static String readBody(WebServer& server) {
  if (server.hasArg("plain")) return server.arg("plain");
  return "";
}

static String extractJsonString(const String& body, const char* key) {
  String k = String("\"") + key + "\":\"";
  int p = body.indexOf(k);
  if (p < 0) return "";
  int s = p + k.length();
  int e = body.indexOf("\"", s);
  if (e <= s) return "";
  return body.substring(s, e);
}

static long extractJsonNumber(const String& body, const char* key) {
  String k = String("\"") + key + "\":";
  int p = body.indexOf(k);
  if (p < 0) return 0;
  int s = p + k.length();
  int e1 = body.indexOf("}", s);
  int e2 = body.indexOf(",", s);
  int e = (e2 > 0 && e2 < e1) ? e2 : e1;
  if (e <= s) return 0;
  return body.substring(s, e).toInt();
}

static void handleRoot(WebServer& server) {
  server.send(200, "text/html; charset=utf-8", INDEX_HTML);
}

static void handleCmd(WebServer& server) {
  String body = readBody(server);
  Serial.println("[API CMD] " + body);

  String cmd = extractJsonString(body, "cmd");

  if (cmd == "connect") {
    g_state.radio_connected = true;
    radio_send_connect();
  } else if (cmd == "disconnect") {
    g_state.radio_connected = false;
    radio_send_disconnect();
  } else if (cmd == "preset") {
    String v = extractJsonString(body, "value");
    if (v.length()) {
      g_state.preset = v;
      radio_send_preset(v);
    }
  } else if (cmd == "mode") {
    String v = extractJsonString(body, "value");
    if (v.length()) {
      g_state.mode = v;
      radio_send_mode(v);
    }
  } else if (cmd == "freq") {
    long hz = extractJsonNumber(body, "hz");
    if (hz >= 0) {
      g_state.freq_hz = (uint32_t)hz;
      radio_send_freq(g_state.freq_hz);
    }
  }

  server.send(200, "text/plain", "OK");
}

static void handleState(WebServer& server) {
  WiFiStatusInfo w = wifi_get_status();

  String json = "{";
  json += "\"radio_connected\":" + String(g_state.radio_connected ? "true" : "false") + ",";
  json += "\"freq_hz\":" + String(g_state.freq_hz) + ",";
  json += "\"mode\":\"" + g_state.mode + "\",";
  json += "\"preset\":\"" + g_state.preset + "\",";
  json += "\"wifi_mode\":\"" + w.wifi_mode + "\",";
  json += "\"sta_ip\":\"" + w.sta_ip + "\",";
  json += "\"ap_ip\":\"" + w.ap_ip + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

void webui_setup(WebServer& server) {
  server.on("/", HTTP_GET, [&server]() { handleRoot(server); });
  server.on("/api/cmd", HTTP_POST, [&server]() { handleCmd(server); });
  server.on("/api/state", HTTP_GET, [&server]() { handleState(server); });

  server.begin();
  Serial.println("HTTP server started.");
}

void webui_loop(WebServer& server) {
  server.handleClient();
}