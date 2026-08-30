#pragma once

// Lokale Zugangsdaten.
// Diese Datei wird NICHT in GitHub gespeichert.
#include "secrets.h"

#define WALLBOX_DISPLAY_VERSION "2.1.3"
#define OTA_HOSTNAME "wallbox-display"

// Das Display wird mechanisch um 180 Grad gedreht eingebaut, damit der
// umgelegte USB-C-Anschluss nach unten zeigt. Bild, Touch und HTTP-Screenshot
// werden gemeinsam gedreht. Zum Rueckbau lediglich auf 0 setzen.
#define DISPLAY_ROTATION_180 1

// Bestehende lokale secrets.h-Dateien bleiben kompatibel. OTA wird erst aktiv,
// wenn dort OTA_PASSWORD mit einem nicht leeren Wert definiert wurde.
#ifndef OTA_PASSWORD
#define OTA_PASSWORD ""
#endif

// Node-RED -> CrowPanel
#define TOPIC_STATUS           "wallbox/data/status"
#define TOPIC_CHARGE_MODE      "wallbox/data/charge_mode"
#define TOPIC_SET_CURRENT      "wallbox/data/set_current_a"
#define TOPIC_POWER            "wallbox/data/charging_power_w"
#define TOPIC_CURRENT          "wallbox/data/charging_current_a"
#define TOPIC_CHARGE_PHASE     "wallbox/data/charge_phase"
#define TOPIC_SESSION_ENERGY   "wallbox/data/session_energy_kwh"
#define TOPIC_TOTAL_ENERGY     "wallbox/data/total_energy_kwh"
#define TOPIC_SESSION_DURATION "wallbox/data/session_duration_s"
#define TOPIC_SESSION_START    "wallbox/session/start"
#define TOPIC_SESSION_END      "wallbox/session/end"

// CrowPanel -> Node-RED / MQTT Explorer
#define TOPIC_ONLINE         "wallbox/display/online"
#define TOPIC_IP             "wallbox/display/ip"
#define TOPIC_RSSI           "wallbox/display/rssi"
#define TOPIC_CMD_START       "wallbox/cmd/start"
#define TOPIC_CMD_STOP        "wallbox/cmd/stop"
#define TOPIC_CMD_CHARGE_MODE "wallbox/cmd/charge_mode"
#define TOPIC_CMD_SET_CURRENT "wallbox/cmd/set_current_a"
