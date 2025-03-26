#include <Arduino.h>
#include "WiFiManager.h"

void WiFiManager::connect(const char* ssid, const char* password) {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado ao Wi-Fi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

bool WiFiManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}