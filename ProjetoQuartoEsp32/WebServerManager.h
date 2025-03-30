#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include "DHTManager.h"
#include <WebServer.h>
#include "RelayManager.h"
#include "NTPManager.h"
#include <WiFiManager.h>

class WebServerManager {
public:
  void begin(RelayManager* relayManager, NTPManager* ntpManager, WiFiManager* wifiManager, DHTManager* dhtManager);
  void handleClient();
  void verificarCondicoesAutomaticas();

private:
  WebServer server;
  RelayManager* relayManager;
  NTPManager* ntpManager;
  WiFiManager* wifiManager;
  DHTManager* dhtManager;
  bool shouldStartPortal;
  // Controle automático
  String autoStartTime, autoEndTime;
  float autoMinTemp;
  int autoCheckIntervalMinutes;
  bool autoModeActive;

  int ventilationDuration;  // Tempo que o ventilador fica ligado (minutos)
  int standbyDuration;      // Tempo em standby entre ciclos (minutos)

  void handleRoot();
  void handleStart();
  void handleStop();
  void handleTime();
  void handleWiFiConfig();
  void handleStatus();
  void handleRemaining();
};

#endif