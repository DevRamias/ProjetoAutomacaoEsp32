#include "WiFiConfigManager.h"
#include "WiFiManager.h"
#include "NTPManager.h"
#include "RelayManager.h"
#include "WebServerManager.h"

WiFiConfigManager wifiConfigManager;
WiFiManager wifiManager;
NTPManager ntpManager;
RelayManager relayManager(5); // Pino do relé
WebServerManager webServerManager;

void setup() {
  Serial.begin(115200);

  // Inicializa o servidor web no WebServerManager
  webServerManager.begin(&relayManager, &ntpManager);

  // Passa o servidor web para o WiFiConfigManager
  //wifiConfigManager.begin(&webServerManager.getServer()); // Usando getServer()

  // Conecta ao Wi-Fi (se já estiver configurado)
  wifiManager.connect("SUA_REDE_WIFI", "SUA_SENHA_WIFI");

  // Inicializa o NTP e o relé
  ntpManager.begin();
  relayManager.begin();
}

void loop() {
  webServerManager.handleClient(); // Gerencia as requisições do servidor
  ntpManager.update();             // Atualiza o horário
  relayManager.update();           // Atualiza o relé
}