#pragma once

// Lokale Zugangsdaten.
// Diese Datei wird NICHT in GitHub gespeichert.
#include "secrets.h"

// Node-RED -> CrowPanel
#define TOPIC_STATUS         "wallbox/display/status"
#define TOPIC_POWER          "wallbox/display/power"
#define TOPIC_CURRENT        "wallbox/display/current"

// CrowPanel -> Node-RED / MQTT Explorer
#define TOPIC_ONLINE         "wallbox/display/online"
#define TOPIC_IP             "wallbox/display/ip"
#define TOPIC_RSSI           "wallbox/display/rssi"
#define TOPIC_CMD_START      "wallbox/cmd/start"
#define TOPIC_CMD_STOP       "wallbox/cmd/stop"
