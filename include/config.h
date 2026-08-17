#pragma once

// Lokale Zugangsdaten.
// Diese Datei wird NICHT in GitHub gespeichert.
#include "secrets.h"

// Node-RED -> CrowPanel
#define TOPIC_STATUS    "wallbox/display/status"

// CrowPanel -> Node-RED
#define TOPIC_CMD_START "wallbox/cmd/start"
#define TOPIC_CMD_STOP  "wallbox/cmd/stop"
