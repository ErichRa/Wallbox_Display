# Wallbox Display – Victron / MQTT / CrowPanel

Stand: 21.08.2026

Dieses Projekt zeigt und steuert eine Victron EV Charging Station auf einem Elecrow CrowPanel DIS08070H V3.0 mit ESP32-S3, LVGL und MQTT.

Der aktuelle Stand ist als erste stabile Einsatzversion gedacht. Die Datenaufbereitung erfolgt überwiegend in Node-RED, das CrowPanel übernimmt vor allem die Darstellung und die Bedienung.

## 1. Architektur

```text
Victron EV Charging Station
        ↓
      Cerbo GX
        ↓
     Node-RED
        ↓ MQTT
   CrowPanel ESP32-S3
        ↓
     LVGL Display
```

Der Entwicklungs-Workflow ist:

```text
ChatGPT / GitHub
      ↓
   git pull
      ↓
  PlatformIO
      ↓
 Build + Upload
      ↓
  CrowPanel
```

Typischer Ablauf lokal:

```bash
git pull
pio run
pio run -t upload
pio device monitor
```

## 2. Aktuelles Dashboard

Das Display zeigt aktuell:

- Wallbox-Status
- Ladeleistung in kW
- Ladestrom in A
- Ladephase 1-ph / 3-ph
- Ladeenergie der aktuellen Session
- Ladezeit
- WLAN-RSSI
- MQTT-Verbindungsstatus
- IP-Adresse
- START- und STOP-Schaltfläche

Geplante Ablage des aktuellen Screenshots im Repository:

```text
docs/images/dashboard_v1.0.png
```

Nach dem Einfügen kann das Bild in dieser Dokumentation so eingebunden werden:

```markdown
![Wallbox Dashboard](docs/images/dashboard_v1.0.png)
```

## 3. MQTT – Node-RED → CrowPanel

Das Display abonniert folgende Topics:

| Topic | Bedeutung | Beispiel |
| --- | --- | --- |
| `wallbox/data/status` | numerischer Wallbox-Status | `2` |
| `wallbox/data/charging_power_w` | Ladeleistung in Watt | `3534` |
| `wallbox/data/charging_current_a` | Ladestrom in Ampere | `16` |
| `wallbox/data/charge_phase` | Ladephase | `1-ph` |
| `wallbox/data/session_energy_kwh` | Energie der aktuellen Ladesession in kWh | `8.42` |
| `wallbox/data/session_duration_s` | Ladedauer in Sekunden | `5820` |

## 4. MQTT – CrowPanel → Node-RED

Das Display veröffentlicht zusätzlich:

```text
wallbox/display/online
wallbox/display/ip
wallbox/display/rssi
```

Die Bedienknöpfe senden:

```text
wallbox/cmd/start
wallbox/cmd/stop
```

## 5. Wallbox-Status

Die Victron-Statuswerte werden auf dem CrowPanel weiterhin numerisch empfangen und dort in Text umgesetzt.

Unterstützte Zustände sind unter anderem:

- BEREIT
- VERBUNDEN
- LADEN
- GELADEN
- WARTE AUF SONNE
- RFID ERFORDERLICH
- WARTE AUF START
- BATTERIE ZU LEER
- ERDUNGSFEHLER
- KONTAKTFEHLER
- CP-FEHLER
- FEHLERSTROM
- UNTERSPANNUNG
- UEBERSPANNUNG
- UEBERHITZUNG
- LADELIMIT
- STARTE LADUNG
- WECHSLE AUF 3 PHASEN
- WECHSLE AUF 1 PHASE
- BEENDE LADUNG

Diese Logik bleibt bewusst im ESP32-Code bestehen.

## 6. Ladephasenerkennung in Node-RED

Für die Erkennung 1-phasig / 3-phasig werden `L1 Power` und `L2 Power` über einen Join-Node zusammengeführt.

Beispiel nach dem Join:

```json
{
  "L1": 3534,
  "L2": 0
}
```

Die Erkennung erfolgt über einen Function-Node mit einem Schwellwert von 100 W:

```javascript
let l1 = Number(msg.payload.L1 || 0);
let l2 = Number(msg.payload.L2 || 0);

const limit = 100;

if (l1 > limit && l2 <= limit) {
    msg.payload = "1-ph";
}
else if (l1 > limit && l2 > limit) {
    msg.payload = "3-ph";
}
else {
    msg.payload = "off";
}

return msg;
```

Ausgabe:

```text
wallbox/data/charge_phase
```

mit:

```text
1-ph
3-ph
off
```

Die Phasenwahl ändert sich im normalen Betrieb nicht laufend, sondern wird praktisch beim Start des Ladevorgangs festgelegt.

## 7. Ladezeit in Sekunden

Die Victron-Wallbox liefert die Ladezeit in Sekunden. Node-RED überträgt den
numerischen Wert ohne Formatierung:

```text
wallbox/data/session_duration_s
Payload: 5820
```

Das CrowPanel formatiert den Wert lokal als Stunden und Minuten.

Beispiele:

```text
90 s    → 00:01 h
3600 s  → 01:00 h
5820 s  → 01:37 h
14520 s → 04:02 h
```

## 8. Darstellung von Ladestrom und Phase

Der aktuelle Wert wird ohne Nachkommastellen angezeigt.

Beispiele:

```text
16 A  1-ph
16 A  3-ph
0 A   --
```

Bei `BEREIT` bzw. `GELADEN` werden Ladeleistung und Ladestrom auf 0 gesetzt.

Ladeenergie und Ladezeit bleiben dagegen sichtbar, damit die zuletzt abgeschlossene Session weiterhin ablesbar ist.

## 9. Screenshot-Funktion

Der aktuelle Bildschirminhalt kann direkt aus dem RGB565-Framebuffer gelesen werden.

Browser-Aufruf:

```text
http://<IP-des-CrowPanel>/
```

Direkter Screendump:

```text
http://<IP-des-CrowPanel>/screenshot.bmp
```

Der Screenshot wird als 24-Bit-BMP mit 800 × 480 Pixel ausgeliefert.

Vorteile:

- keine Spiegelungen
- keine perspektivischen Verzerrungen
- pixelgenauer Bildschirminhalt
- einfache Dokumentation
- sehr gute Diagnosemöglichkeit bei Darstellungsfehlern

Die BMP-Datei wird direkt aus dem Framebuffer erzeugt und nicht dauerhaft auf dem ESP32 gespeichert.

## 10. Touch-Fix CrowPanel DIS08070H V3.0

Ein wichtiger Hardware-Fix betrifft den GT911-Touchcontroller.

Der ursprüngliche Fehler war:

```text
ESP_ERR_INVALID_STATE
i2cWriteReadNonStop()
```

Nach mehreren Tests wurde die eigentliche Ursache gefunden:

GPIO19 und GPIO20 werden beim ESP32-S3 gleichzeitig vom USB-Serial/JTAG-Interface verwendet.

Vor der Initialisierung von `Wire1` müssen diese USB-Pads explizit für I2C freigegeben werden.

Seit dieser Änderung funktioniert der Touch stabil.

## 11. Aktueller stabiler Stand

Der aktuelle Funktionsumfang umfasst:

- stabile LVGL-Ausgabe auf dem CrowPanel
- WLAN-Verbindung
- MQTT-Verbindung
- START / STOP über MQTT
- numerische Victron-Statusauswertung
- Ladeleistungsanzeige
- Ladestromanzeige ohne Nachkommastellen
- Ladephasenerkennung 1-ph / 3-ph
- Anzeige der Session-Energie
- Anzeige der Ladezeit im Format `hh:mm h`
- HTTP-Webserver
- Screenshot des kompletten Displays über `/screenshot.bmp`
- funktionierender GT911-Touch

Damit ist das Projekt in einem sehr guten, praktisch einsetzbaren Zustand.

## 12. Lokale Zugangsdaten

WLAN- und MQTT-Zugangsdaten werden nicht im öffentlichen GitHub-Repository gespeichert.

Die lokale Konfiguration erfolgt über eine nicht versionierte Datei `include/secrets.h`, die von `config.h` eingebunden wird.
