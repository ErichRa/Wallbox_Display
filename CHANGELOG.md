# Changelog

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
