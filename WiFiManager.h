#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <WebServer.h>  // Adicione esta linha

class WiFiManager {
public:
  void begin();
  void connect(const char* ssid, const char* password);
  bool isConnected();
  bool isInConfigurationMode();
  void startConfigurationPortal();
  
private:
  WebServer* server;  // Alterado para ponteiro
};

#endif