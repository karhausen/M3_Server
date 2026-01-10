# RadioRemote – ESP32 Funkgeräte-Remote

Webbasierte Fernbedienung für ein Funkgerät auf Basis eines ESP32.  
Bedienung erfolgt über eine schlanke Web-GUI (Smartphone / Tablet / PC),  
die Kommunikation zum Funkgerät läuft über eine serielle Schnittstelle.

Das Projekt ist für **Feldbetrieb** ausgelegt:
- WLAN-STA mit Fallback-AP
- Touch-/Swipe-Bedienung (VFO-ähnlich)
- minimale, robuste Kommandoübertragung

---

## ✨ Features

- 🌐 **Web-GUI** (mobil & desktopfähig)
- 📡 **Frequenzsteuerung wie am VFO**
  - Swipe hoch/runter → Frequenz ±
  - Swipe links/rechts → aktive Stelle (1 Hz … 10 MHz)
  - aktive Stelle wird unterstrichen angezeigt
- 🎚️ **Presets (Platin, 1–9)**  
  (Preset-Inhalt wird vom Funkgerät selbst verwaltet)
- 🔁 **Debounced Frequency Send**
  - Frequenz wird erst gesendet, wenn keine Änderung mehr erfolgt
  - minimiert serielle Kommandos → höhere Betriebssicherheit
- 🌙 **Dark / Light Mode** (umschaltbar, im Browser gespeichert)
- 🔌 **Connect / Disconnect Toggle**
- ⚙️ **Setup-Seite**
  - STA-WLAN SSID & Passwort setzen
  - Reboot-Button
- 💾 **Persistente Konfiguration**
  - WLAN-Zugangsdaten werden im ESP32 (NVS / Preferences) gespeichert
  - überlebt stromloses Abschalten

---

## 📂 Projektstruktur

```
RadioRemote/
├─ RadioRemote.ino
├─ README.md
├─ config.h
├─ app_state.h
│
├─ wifi_config.h/.cpp
├─ wifi_manager.h/.cpp
│
├─ radio_link.h/.cpp
│
├─ web_ui.h/.cpp
├─ web_pages.h/.cpp
├─ setup_page.h
```

---

## 🚀 Inbetriebnahme

### 1️⃣ Flashen
- ESP32 mit Arduino IDE flashen
- Serial Monitor auf 115200 Baud öffnen

### 2️⃣ Erststart
- Wenn **keine WLAN-Daten gespeichert** sind:
  - ESP startet automatisch im **AP-Mode**
  - SSID: `RadioRemote-ESP32`
  - IP: `http://192.168.4.1`

### 3️⃣ WLAN konfigurieren
- Browser öffnen: `http://192.168.4.1/setup`
- STA-SSID & Passwort eingeben
- **Reboot drücken**

### 4️⃣ Normalbetrieb
- ESP verbindet sich mit dem gespeicherten WLAN
- Web-GUI über die im Serial Monitor angezeigte IP erreichbar

---

## 🖐️ Bedienung (GUI)

### Frequenz
- **Swipe ↑ / ↓** → Frequenz ändern (aktueller Step)
- **Swipe ← / →** → aktive Stelle ändern
- Aktive Stelle ist **unterstrichen**

### Presets
- Buttons `Platin`, `1` … `9`
- Preset-Inhalt (Frequenz, Mode, etc.) wird vom Funkgerät selbst gesetzt

### Dark Mode
- 🌙 Button oben links
- Zustand wird im Browser gespeichert

### Setup
- ⚙️ Button → `/setup`
- WLAN konfigurieren
- Reboot auslösen

---

## 🔧 Funkgeräte-Anbindung

Aktuell:
- Dummy-Ausgabe auf **Serial0** (Debug)

Geplant:
- Umschaltung auf **Serial2**
- Unterstützung realer Funkgeräte-Protokolle (z.B. CAT, CI-V, SCPI o.ä.)

Der Web-Teil ist **protokollunabhängig** – die Funklogik ist gekapselt in:
```
radio_link.cpp
```

---

## 🛡️ Robustheitskonzept

- WLAN-Fallback (STA → AP)
- Frequenzänderungen werden **gebündelt**
- Kein unnötiges Flooding der seriellen Schnittstelle
- UI bleibt responsiv, auch bei langsamen Geräten

---

## 📌 ToDo / Ideen

- WebSocket für Live-Status
- PTT (Hold-to-Transmit)
- Haptisches Feedback (Android)
- Beschleunigung bei schnellem Swipe
- Erweiterte Setup-Optionen (Baudrate, Protokoll, …)

---

## 🧑‍🔧 Zielgruppe

- Funkamateure
- Entwickler
- Servicetechniker
- Feld- und Remote-Betrieb

---

## 📜 Lizenz

Privates Projekt / Lernprojekt  
Lizenz nach Bedarf ergänzen.
