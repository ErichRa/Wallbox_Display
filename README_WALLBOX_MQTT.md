# CrowPanel DIS08070H V3.0 – erster Wallbox/MQTT-Test

Dieses Projekt basiert direkt auf der stabil laufenden Elecrow-Demo `PlatformIO70`.

Unverändert übernommen:
- Elecrow Board-JSON
- LovyanGFX 1.2.25
- Elecrow `patch_lovyangfx.py`
- gepatchte `Bus_RGB.cpp/.hpp`
- LVGL 9.1.0
- zwei vollständige RGB-Framebuffer
- `presentFrameBuffer()` / VSYNC
- GT911/PCA9557 Touch-Code
- RGB-Timing 24 MHz

## 1. WLAN und MQTT eintragen

`include/config.h` bearbeiten.

Beispiel:
```cpp
#define WIFI_SSID       "MeinWLAN"
#define WIFI_PASSWORD   "MeinPasswort"
#define MQTT_HOST       "192.168.1.20"
#define MQTT_PORT       1883
```

## 2. Build auf dem Mac

Im Projektordner:

```bash
~/.platformio/penv/bin/pio run
```

## 3. MQTT-Test

Das Display hört auf:

`wallbox/display/status`

Payload:

```json
{
  "connected": true,
  "charging": true,
  "power": 1630,
  "current": 7.1
}
```

Die bisherige Temperaturanzeige zeigt dann:

`1.63 kW`

Die bisherige Feuchteanzeige zeigt:

`LÄDT 7.1 A`

## 4. Button-Test

Der originale ON-Button sendet:

Topic:
`wallbox/cmd/start`

Payload:
`1`

Der originale OFF-Button sendet:

Topic:
`wallbox/cmd/stop`

Payload:
`1`

## 5. Erst danach

Wenn Display + Touch + WLAN + MQTT stabil laufen, ersetzen wir die
Elecrow-Demooberfläche durch die 800×480-Wallbox-Startseite.
