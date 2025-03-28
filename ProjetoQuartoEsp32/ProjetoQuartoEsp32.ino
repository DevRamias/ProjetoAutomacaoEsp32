#include <WiFiManager.h>
#include "NTPManager.h"
#include "RelayManager.h"
#include "WebServerManager.h"
#include <ESPmDNS.h>

#define DHTPIN 4      // Pino onde o DHT está conectado (ajuste se necessário)
#define DHTTYPE DHT11 // Tipo do sensor (DHT11 ou DHT22)

WiFiManager wifiManager;
NTPManager ntpManager;
RelayManager relayManager(5); // Pino do relé
DHTManager dhtManager(DHTPIN, DHTTYPE);
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
  webServerManager.begin(&relayManager, &ntpManager, &wifiManager, &dhtManager);

  if (WiFi.status() == WL_CONNECTED) {
    if (!MDNS.begin("esp32")) {
      Serial.println("Erro ao iniciar mDNS!");
    } else {
      Serial.println("mDNS iniciado! Acesse: http://esp32.local");
      MDNS.addService("http", "tcp", 80); // Adiciona serviço HTTP
    }
  }
}


void loop() {
  webServerManager.handleClient();
  ntpManager.update();
  relayManager.update();
}