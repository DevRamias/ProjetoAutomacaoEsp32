#ifndef WEBSERVER_MANAGER_H
#define WEBSERVER_MANAGER_H

#include <WebServer.h>
#include <FS.h>
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

  // Chamado a cada requisicao recebida: tira o Wi-Fi do modo economico
  // enquanto alguem estiver usando o painel.
  void markActivity();

private:
  WebServer server;
  RelayManager* relayManager;
  NTPManager* ntpManager;
  WiFiManager* wifiManager;
  DHTManager* dhtManager;
  OTAManager* otaManager = nullptr;

  bool shouldStartPortal;
  unsigned long _lastMemoryLog;

  // Economia de energia do Wi-Fi (modem sleep):
  //  - padrao: LIGADA (gasta menos, mas cada resposta demora ~100-300 ms a mais);
  //  - chegou requisicao: DESLIGA (painel responde rapido);
  //  - 60 s sem nenhuma requisicao: LIGA de novo.
  static const unsigned long FAST_WIFI_MS = 60000;
  bool _fastWiFi = false;
  unsigned long _lastActivity = 0;
  void updatePowerSave();

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
  void handleState();          // tudo do painel numa requisicao so (/state)
  void handleUploadRaw();      // recebe o HTML em pedacos (sem carregar tudo na RAM)

  File _uploadFile;
  size_t _uploadSize = 0;
  bool _uploadFailed = false;

  unsigned long remainingSeconds();

  // Cache do espaco usado no LittleFS (calcular percorre os arquivos: ~0,5 s)
  size_t _fsUsedCache = 0;
  unsigned long _fsUsedAt = 0;
  bool _fsUsedValid = false;
  size_t fsUsedBytes();
  void invalidateFsCache() { _fsUsedValid = false; }

  void logMemoryUsage();
};

#endif