#ifndef WIFI_CONFIG_MANAGER_H
#define WIFI_CONFIG_MANAGER_H

#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>

class WiFiConfigManager {
public:
  void begin();
  void handleClient();
  bool isConnected();

private:
  WebServer server;
  bool connected;

  bool connectToSavedNetwork();
  void startAP();
  void saveNetwork(String ssid, String password);
  void deleteSavedNetwork();
  void handleRoot();
  void handleScan();
  void handleConnect();
  void handleDelete();
};

#endif