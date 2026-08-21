#pragma once

// Lokale Zugangsdaten.
// Diese Datei wird NICHT in GitHub gespeichert.
#include "secrets.h"

// Node-RED -> CrowPanel
#define TOPIC_WALLBOX_ONLINE   "wallbox/data/online"
#define TOPIC_STATUS           "wallbox/data/status"
#define TOPIC_POWER            "wallbox/data/charging_power_w"
#define TOPIC_CURRENT          "wallbox/data/charging_current_a"
#define TOPIC_CHARGE_PHASE     "wallbox/data/charge_phase"
#define TOPIC_SESSION_ENERGY   "wallbox/data/session_energy_kwh"
#define TOPIC_SESSION_DURATION "wallbox/data/session_duration_s"

// CrowPanel -> Node-RED / MQTT Explorer
#define TOPIC_ONLINE         "wallbox/display/online"
#define TOPIC_IP             "wallbox/display/ip"
#define TOPIC_RSSI           "wallbox/display/rssi"
#define TOPIC_CMD_START      "wallbox/cmd/start"
#define TOPIC_CMD_STOP       "wallbox/cmd/stop"
