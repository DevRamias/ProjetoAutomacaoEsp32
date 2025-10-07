#ifndef WEBSERVER_MANAGER_H
#define WEBSERVER_MANAGER_H

#include <WebServer.h>
#include "RelayManager.h"
#include "NTPManager.h"
#include "DHTManager.h"
#include <WiFiManager.h>

class WebServerManager {
public:
  void begin(RelayManager* relayManager, NTPManager* ntpManager, WiFiManager* wifiManager, DHTManager* dhtManager);
  void handleClient();

private:
  WebServer server;
  RelayManager* relayManager;
  NTPManager* ntpManager;
  WiFiManager* wifiManager;
  DHTManager* dhtManager;

  bool shouldStartPortal;
  unsigned long _lastMemoryLog;

  // Handlers
  void handleRoot();
  void handleStart();
  void handleStop();
  void handleTime();
  void handleStatus();
  void handleWiFiConfig();
  void handleRemaining();
  void handleSystemInfo();
  void handleFlashInfo();
  void handleSensorData();
  void handleUpload();
  void handleSetAutoSettings();
  void handleGetAutoSettings();

  void logMemoryUsage();
};

#endif