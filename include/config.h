#pragma once

// ===== HIER ANPASSEN =====
#define WIFI_SSID       "DEIN_WLAN"
#define WIFI_PASSWORD   "DEIN_WLAN_PASSWORT"

#define MQTT_HOST       "192.168.1.10"
#define MQTT_PORT       1883

// Leer lassen, falls dein Broker keine Anmeldung verlangt.
#define MQTT_USER       ""
#define MQTT_PASSWORD   ""

#define MQTT_CLIENT_ID  "crowpanel-wallbox"

// Node-RED -> CrowPanel
#define TOPIC_STATUS    "wallbox/display/status"

// CrowPanel -> Node-RED
#define TOPIC_CMD_START "wallbox/cmd/start"
#define TOPIC_CMD_STOP  "wallbox/cmd/stop"
