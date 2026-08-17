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
- RGB-Timing 24 MHz

## 1. GitHub + PlatformIO Workflow

Der Entwicklungsablauf:

```text
ChatGPT
   ↓
GitHub
   ↓ git pull
lokales Projekt
   ↓ Build
PlatformIO
   ↓ Upload
CrowPanel
```

Typischer Ablauf:

```bash
git pull
pio run
pio run -t upload
pio device monitor
```

## 2. Zugangsdaten nicht in GitHub speichern

WLAN- und MQTT-Zugangsdaten werden lokal in `include/config.h` bzw. einer nicht versionierten Datei abgelegt.

## 3. MQTT-Test

Das Display abonniert:

`wallbox/display/status`

Beispiel:

```json
{
  "connected": true,
  "charging": true,
  "power": 1630,
  "current": 7.1
}
```

Das Display veröffentlicht zusätzlich:

```text
wallbox/display/online
wallbox/display/ip
wallbox/display/rssi
```

Die ON/OFF-Schaltflächen senden:

```text
wallbox/cmd/start
wallbox/cmd/stop
```

## 4. Touch-Fehlersuche – Chronologie

### Ausgangsfehler

```text
ESP_ERR_INVALID_STATE
i2cWriteReadNonStop()
```

Der Touch funktionierte nicht.

### Test 1 – Repeated Start entfernen

Änderung:

```cpp
endTransmission(false)
→
endTransmission(true)
```

Ergebnis:

❌ Fehler blieb bestehen.

Erkenntnis:

Der Repeated-Start war nicht die Ursache.

### Test 2 – Wire1.end()/Wire1.begin() entfernen

Ergebnis:

❌ Fehler blieb bestehen.

Erkenntnis:

Die erneute Initialisierung des I²C-Busses war nicht die Ursache.

### Test 3 – I²C-Scan vor und nach lcd.begin()

Ergebnis:

```text
0x18 → PCA9557
0x5D → GT911
```

Vor und nach `lcd.begin()` wurden beide Geräte gefunden.

Erkenntnis:

- Bus funktioniert.
- Adressen stimmen.
- `lcd.begin()` zerstört den Bus nicht.

### Test 4 – GT911-Registerdiagnose

Ergebnis:

```text
Product-ID: $$$$
Status: 0x24
Config: 0x24
```

Erkenntnis:

Der GT911 antwortete zwar, lieferte aber keine gültigen Registerdaten.

### Test 5 – GPIO38 nicht mehr als Relais verwenden

Erkenntnis:

GPIO38 ist die Interrupt-Leitung des GT911 und darf nicht als Relaisausgang verwendet werden.

### Test 6 – PCA9557-Resetsequenz entfernen

Ergebnis:

❌ Fehler blieb bestehen.

### Test 7 – USB-Pads auf GPIO19/GPIO20 freigeben

Ergebnis:

✅ Touch funktioniert.

✅ Keine `ESP_ERR_INVALID_STATE`-Fehler mehr.

Erkenntnis:

Beim CrowPanel DIS08070H V3.0 werden GPIO19 und GPIO20 gleichzeitig vom USB-Serial/JTAG-Interface verwendet.

Vor der Initialisierung von `Wire1` müssen die USB-Pads explizit freigegeben werden.

Das war die eigentliche Ursache.

## 5. Nächster Schritt

Die Anzeige wird jetzt schrittweise von der Elecrow-Demo auf eine echte Victron-Wallbox-Oberfläche umgestellt.
