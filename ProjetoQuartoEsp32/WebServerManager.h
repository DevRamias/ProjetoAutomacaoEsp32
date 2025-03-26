#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <WebServer.h>
#include "RelayManager.h"
#include "NTPManager.h"
#include <WiFiManager.h>

class WebServerManager {
public:
  void begin(RelayManager* relayManager, NTPManager* ntpManager, WiFiManager* wifiManager);
  void handleClient();

private:
  WebServer server;
  RelayManager* relayManager;
  NTPManager* ntpManager;
  WiFiManager* wifiManager;
  bool shouldStartPortal;

  void handleRoot();
  void handleStart();
  void handleStop();
  void handleTime();
  void handleWiFiConfig();
  void handleStatus();
  void handleRemaining();
};

#endif