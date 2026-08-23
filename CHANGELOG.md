# Changelog

## 2.1.0 – 23.08.2026

Finalisierter Bedien- und Darstellungsstand des Wallbox-Displays.

### Darstellung

- Sollstromanzeige und Regler direkt unter dem Ladestrom-Kreisdiagramm
- vergrößerter Ladestromwert und gleichmäßige Anordnung von Phase, Stromwert
  und Beschriftung im Kreisdiagramm
- Ladeleistung, Status und Fahrzeugtext rechtsbündig ausgerichtet
- verbreiterter `SCHEDULED`-Button bei gleich breiten `MANUAL`- und
  `AUTO`-Buttons
- WLAN-RSSI, MQTT-Status und IP-Adresse rechtsbündig ausgerichtet
- NTP-synchronisierte Uhrzeit mittig in der unteren Statuszeile

### Bedienung

- aktiver Lademodus blau, andere auswählbare Modi dunkelblau und gesperrte
  Modi grau dargestellt
- angeforderter Lademodus bis zur bestätigten Rückmeldung gelb markiert
- unbekannter Startstatus einzeilig dargestellt
- automatische Hintergrundbeleuchtung: nach zwei Minuten auf 5 Prozent
  dimmen und nach zehn Minuten ausschalten
- erste Berührung bei ausgeschalteter Beleuchtung dient ausschließlich zum
  Aufwecken

## 2.0.0 – 23.08.2026

Abgenommene stabile Version der Wallbox-Anzeige und -Steuerung.

### Neu

- Lademodi `manual`, `auto` und `scheduled` über MQTT und Modbus steuerbar
- manueller Soll-Ladestrom von 6 bis 16 A mit bestätigtem Rückmeldewert
- Kreisdiagramm für den tatsächlichen Ladestrom mit Ladephase
- Ladebeginn und Ladeende mit Datum und Uhrzeit
- Wiederherstellung eines fehlenden Ladebeginns aus der vorhandenen Ladedauer
- kumulierte Gesamtenergie der Wallbox
- Home-Assistant-Anbindung über einheitliche MQTT-Topics

### Bedienung

- kontextabhängige Freigabe von START und STOPP
- START ist in `auto` und `scheduled` deaktiviert
- STOPP ist nur während einer laufenden Ladung aktiv
- Sollstromregler unter dem Ladestrom-Kreisdiagramm ist ausschließlich in `manual` verfügbar
- deaktivierte Bedienelemente werden sichtbar grau dargestellt

### MQTT

- einheitliche Bereiche `wallbox/data`, `wallbox/session`, `wallbox/cmd`,
  `wallbox/config` und `wallbox/display`
- klare Trennung von Soll- und Ist-Ladestrom
- retained Rückmeldungen für Zustände, Sollwerte und Session-Zeitstempel
