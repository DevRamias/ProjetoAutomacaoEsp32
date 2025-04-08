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

  // Novas funções adicionadas
  void logMemoryUsage();
  void handleSystemInfo();
  void handleFlashInfo();

private:
  WebServer server;
  RelayManager* relayManager;
  NTPManager* ntpManager;
  WiFiManager* wifiManager;
  DHTManager* dhtManager;
  bool shouldStartPortal;
  
  // Controle automático
  String autoStartTime;
  String autoEndTime;
  float autoMinTemp;
  int autoCheckIntervalMinutes;
  bool autoModeActive;
  int ventilationDuration;
  int standbyDuration;
  
  bool isWithinActiveHours();
  
  // Monitoramento de memória
  unsigned long _lastMemoryLog;

  // Handlers
  void handleRoot();
  void handleStart();
  void handleStop();
  void handleTime();
  void handleWiFiConfig();
  void handleStatus();
  void handleRemaining();
};
#endif