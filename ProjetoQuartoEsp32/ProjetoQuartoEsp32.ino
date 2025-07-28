#include <WiFiManager.h>
#include "NTPManager.h"
#include "RelayManager.h"
#include "WebServerManager.h"
#include "DHTManager.h"
#include <ESPmDNS.h>
#include "LittleFS.h"
#include <Arduino_ESP32_OTA.h>
//#include <ESP32OTA.h>
#include <ArduinoOTA.h>
#include "OTAManager.h"

#define DHTPIN 4      // Pino onde o DHT está conectado (ajuste se necessário)
#define DHTTYPE DHT11 // Tipo do sensor (DHT11 ou DHT22)

WiFiManager wifiManager;
NTPManager ntpManager;
RelayManager relayManager(5); // Pino do relé
DHTManager dhtManager(DHTPIN, DHTTYPE);
WebServerManager webServerManager;
OTAManager otaManager;

void setup() {
  Serial.begin(115200);

  // Inicializa o sistema de arquivos
  if (!LittleFS.begin()) {
    Serial.println("Erro ao montar o sistema de arquivos!");
    return;
  }
Serial.println("LittleFS inicializado!");

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

  // Configura OTA (com hostname e senha opcionais)
  otaManager.begin("tomada-inteligente", "Teste123");

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
  otaManager.handle();
  webServerManager.handleClient();
  ntpManager.update();
  relayManager.update();
}