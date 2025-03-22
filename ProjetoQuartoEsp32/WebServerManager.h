#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <WebServer.h>
#include "RelayManager.h"
#include "NTPManager.h"

class WebServerManager {
public:
  void begin(RelayManager* relayManager, NTPManager* ntpManager);
  void handleClient();

private:
  WebServer server;
  RelayManager* relayManager;
  NTPManager* ntpManager;

  void handleRoot();
  void handleStart();
  void handleStop();
  void handleTime();
};

#endif