#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

class WiFiManager {
public:
  void connect(const char* ssid, const char* password);
  bool isConnected();
};

#endif