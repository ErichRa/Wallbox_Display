#pragma once

// Vorlage für lokale Zugangsdaten.
// Diese Datei darf in GitHub bleiben.
// Kopiere sie lokal nach include/secrets.h und trage dort deine echten Werte ein.

#define WIFI_SSID       "DEIN_WLAN"
#define WIFI_PASSWORD   "DEIN_WLAN_PASSWORT"

#define MQTT_HOST       "192.168.1.10"
#define MQTT_PORT       1883

// Leer lassen, falls dein Broker keine Anmeldung verlangt.
#define MQTT_USER       ""
#define MQTT_PASSWORD   ""

#define MQTT_CLIENT_ID  "crowpanel-wallbox"
