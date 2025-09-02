#ifndef WEBSERVER_MANAGER_H
#define WEBSERVER_MANAGER_H

#include <WebServer.h> // A biblioteca correta que você está usando
#include "RelayManager.h"
#include "NTPManager.h"
#include "DHTManager.h"
#include <WiFiManager.h>

class WebServerManager {
public:
  void begin(RelayManager* relayManager, NTPManager* ntpManager, WiFiManager* wifiManager, DHTManager* dhtManager);
  void handleClient();

private:
  WebServer server; // Usando WebServer, não AsyncWebServer
  RelayManager* relayManager;
  NTPManager* ntpManager;
  WiFiManager* wifiManager;
  DHTManager* dhtManager;

  // Variáveis de configuração
  bool autoModeActive;
  float autoMinTemp;
  unsigned int ventilationDuration;
  unsigned int standbyDuration;
  String autoStartTime;
  String autoEndTime;
  bool shouldStartPortal;
  unsigned long _lastMemoryLog;

  // --- Funções Handler (declaradas corretamente para a classe) ---
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

  // Funções de suporte
  bool isWithinActiveHours();
  void logMemoryUsage();
};

#endif
