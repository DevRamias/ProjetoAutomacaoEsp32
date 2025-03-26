#include <WiFiManager.h>
#include "NTPManager.h"
#include "RelayManager.h"
#include "WebServerManager.h"

WiFiManager wifiManager;
NTPManager ntpManager;
RelayManager relayManager(5); // Pino do relé
WebServerManager webServerManager;

void setup() {
  Serial.begin(115200);

  // Configuração WiFi com timeout de 3 minutos
  wifiManager.setConfigPortalTimeout(180);
  
  if (!wifiManager.autoConnect("ESP32-Config")) {
    Serial.println("Falha na conexão e timeout do portal");
    delay(3000);
    ESP.restart();
  }

  Serial.println("Conectado ao WiFi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  ntpManager.begin();
  relayManager.begin();
  webServerManager.begin(&relayManager, &ntpManager, &wifiManager);
}

void loop() {
  webServerManager.handleClient();
  ntpManager.update();
  relayManager.update();
}