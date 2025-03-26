#include "WiFiManager.h"
#include "NTPManager.h"
#include "RelayManager.h"
#include "WebServerManager.h"

WiFiManager wifiManager;
NTPManager ntpManager;
RelayManager relayManager(5); // Pino do relé
WebServerManager webServerManager;

void setup() {
  Serial.begin(115200);

  wifiManager.connect("SUA_REDE_WIFI", "SUA_SENHA_WIFI");
  ntpManager.begin();
  relayManager.begin();
  webServerManager.begin(&relayManager, &ntpManager);
}

void loop() {
  webServerManager.handleClient();
  ntpManager.update();
  relayManager.update();
}