#ifndef WEBSERVER_MANAGER_H
#define WEBSERVER_MANAGER_H

#include <WebServer.h>
#include "RelayManager.h"
#include "NTPManager.h"
#include "DHTManager.h"
#include "OTAManager.h"
#include <WiFiManager.h>

class WebServerManager {
public:
  void begin(RelayManager* relayManager, NTPManager* ntpManager, WiFiManager* wifiManager, DHTManager* dhtManager);
  void handleClient();
  void setOTAManager(OTAManager* otaManager) { this->otaManager = otaManager; }

private:
  WebServer server;
  RelayManager* relayManager;
  NTPManager* ntpManager;
  WiFiManager* wifiManager;
  DHTManager* dhtManager;
  OTAManager* otaManager = nullptr;

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
  void handleRecovery();   // pagina de recuperacao embutida (/recovery)
  void handleResetHtml();
  void handleSetOtaPassword(); // troca a senha do OTA (/set-ota-password)
  void handleDeviceInfo();     // rede, IP, sinal e status da senha (/device-info)  // apaga o /index.html salvo (/reset-html)
  void handleSetAutoSettings();
  void handleGetAutoSettings();

  void logMemoryUsage();
};

#endif