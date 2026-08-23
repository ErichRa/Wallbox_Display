# Wallbox Display – Victron / MQTT / CrowPanel

Version: 2.0.0
Stand: 23.08.2026

Dieses Projekt zeigt und steuert eine Victron EV Charging Station auf einem Elecrow CrowPanel DIS08070H V3.0 mit ESP32-S3, LVGL und MQTT.

Version 2.0.0 ist die abgenommene stabile Einsatzversion. Die Datenaufbereitung erfolgt überwiegend in Node-RED, das CrowPanel übernimmt vor allem die Darstellung und die Bedienung.

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
- Auswahl des Lademodus Manual / Auto / Scheduled
- Ladeleistung in kW
- tatsächlichen Ladestrom in A als Zahlenwert und Kreisdiagramm (0–16 A),
  oben mit der Ladephase `1-ph`, `3-ph` oder `off` und unten mit der
  Beschriftung `LADESTROM`
- Ladeenergie der aktuellen Session
- kumulierte Gesamtenergie der Wallbox
- Ladezeit
- Ladebeginn und Ladeende mit Datum und Uhrzeit
- WLAN-RSSI
- MQTT-Verbindungsstatus
- IP-Adresse
- START- und STOP-Schaltfläche

Die Bedienfreigabe folgt dem bestätigten Modus und Wallbox-Status:

- der bestätigte Lademodus wird blau dargestellt
- ein angeforderter, noch nicht bestätigter Lademodus wird bis zu zehn
  Sekunden gelb dargestellt
- die beiden anderen verfügbaren Lademodi bleiben dunkel, vollständig sichtbar
  und auswählbar
- nur bei fehlender Wallbox- oder Modusrückmeldung werden alle drei
  Modusschaltflächen grau dargestellt und gesperrt
- `MANUAL`: im Stillstand START aktiv, während der Ladung STOP aktiv
- `AUTO` und `SCHEDULED`: START immer deaktiviert, STOP nur während der Ladung aktiv
- bei fehlender Wallbox-Rückmeldung, Fehlerstatus oder laufendem Beenden sind beide Tasten deaktiviert

### Hintergrundbeleuchtung

Nach zwei Minuten ohne Berührung wird die Hintergrundbeleuchtung automatisch
auf 10 Prozent reduziert. Nach insgesamt zehn Minuten wird sie vollständig
ausgeschaltet.
ESP32, WLAN, MQTT und die Displaydarstellung bleiben dabei aktiv.

Eine Berührung des gedimmten Displays stellt sofort die volle Helligkeit wieder
her und wird normal verarbeitet. Bei ausgeschalteter Beleuchtung dient die erste
Berührung ausschließlich zum Aufwecken; erst die folgende Berührung bedient ein
Element.

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
| `wallbox/data/charge_mode` | bestätigter Lademodus | `auto` |
| `wallbox/data/charging_power_w` | Ladeleistung in Watt | `3534` |
| `wallbox/data/charging_current_a` | Ladestrom in Ampere | `16` |
| `wallbox/data/set_current_a` | bestätigter Soll-Ladestrom in Ampere | `8` |
| `wallbox/data/charge_phase` | Ladephase | `1-ph` |
| `wallbox/data/session_energy_kwh` | Energie der aktuellen Ladesession in kWh | `8.42` |
| `wallbox/data/total_energy_kwh` | Kumulierte Gesamtenergie der Wallbox in kWh | `482.63` |
| `wallbox/data/session_duration_s` | Ladedauer in Sekunden | `5820` |
| `wallbox/session/start` | Beginn der aktuellen/letzten Session als ISO-8601-Zeitstempel | `2026-08-23T13:08:26+02:00` |
| `wallbox/session/end` | Ende als ISO-8601-Zeitstempel oder während des Ladens `---` | `---` |

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
wallbox/cmd/charge_mode
wallbox/cmd/set_current_a
```

### Lademodus

Das CrowPanel sendet den gewünschten Modus als Text an
`wallbox/cmd/charge_mode`. Node-RED schreibt den zugehörigen Zahlenwert
auf den Victron-Pfad `/Mode`:

| MQTT | Victron |
| --- | --- |
| `manual` | `0` |
| `auto` | `1` |
| `scheduled` | `2` |

Der tatsächlich aktive Modus wird separat über
`wallbox/data/charge_mode` zurückgemeldet. Erst diese Rückmeldung markiert
die entsprechende Schaltfläche auf dem Display als aktiv.

### Soll-Ladestrom

Im manuellen Modus kann das CrowPanel einen ganzzahligen Soll-Ladestrom von
6 bis 16 A an `wallbox/cmd/set_current_a` senden. Der Regler veröffentlicht
erst beim Loslassen und ohne Retain. Node-RED prüft den Bereich und schreibt
den Wert auf Victron `/SetCurrent` beziehungsweise Modbus-Register 5016.
Anzeige und Regler für den Sollstrom befinden sich direkt unter dem
Kreisdiagramm des tatsächlichen Ladestroms.

Die Wallbox bestätigt den aktuellen Sollwert separat über
`wallbox/data/set_current_a`. Dieser Rückmeldewert aktualisiert Anzeige und
Regler. `wallbox/data/charging_current_a` bleibt der davon unabhängige,
tatsächlich fließende Ladestrom.

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
- Umschaltung Manual / Auto / Scheduled mit bestätigter Zustandsanzeige
- numerische Victron-Statusauswertung
- Ladeleistungsanzeige
- Ladestromanzeige ohne Nachkommastellen
- Ladephasenerkennung 1-ph / 3-ph
- Anzeige der Session-Energie
- Anzeige der kumulierten Gesamtenergie aus Victron `/Ac/Energy/Forward`
- Footer ohne SOC- und Temperatur-Platzhalter
- Anzeige der Ladezeit im Format `hh:mm h`
- HTTP-Webserver
- Screenshot des kompletten Displays über `/screenshot.bmp`
- funktionierender GT911-Touch

Damit ist das Projekt in einem sehr guten, praktisch einsetzbaren Zustand.

## 12. Lokale Zugangsdaten

WLAN- und MQTT-Zugangsdaten werden nicht im öffentlichen GitHub-Repository gespeichert.

Die lokale Konfiguration erfolgt über eine nicht versionierte Datei `include/secrets.h`, die von `config.h` eingebunden wird.
