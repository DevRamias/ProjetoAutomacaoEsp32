#include <WiFiManager.h>
#include "NTPManager.h"
#include "RelayManager.h"
#include "WebServerManager.h"
#include "DHTManager.h"
#include <ESPmDNS.h>
#include "LittleFS.h"
#include <ArduinoOTA.h>
#include "OTAManager.h"
#include <ArduinoJson.h>

#define DHTPIN 4
#define DHTTYPE DHT11

// Protótipos
void setup();
void loop();

WiFiManager wifiManager;
NTPManager ntpManager;
RelayManager relayManager(5);
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

  // Configura o portal WiFi
  // O portal agora serve SO para a rede Wi-Fi. A senha do OTA e trocada pelo
  // painel web (card "Configuracoes da placa").
  wifiManager.setConfigPortalTimeout(180);

  // Tenta conectar; se falhar, abre o portal
  if (!wifiManager.autoConnect("ESP32-Config")) {
    Serial.println("Falha na conexão e timeout do portal");
    delay(3000);
    ESP.restart();
  }

  Serial.println("Conectado ao WiFi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Inicializa os managers
  ntpManager.begin();
  relayManager.begin();
  relayManager.setNTPManager(&ntpManager);
  dhtManager.begin();  // CORRIGIDO: o DHT precisa ser inicializado antes de ser lido
  webServerManager.setOTAManager(&otaManager);
  webServerManager.begin(&relayManager, &ntpManager, &wifiManager, &dhtManager);

  // Configura OTA
  const char* otaHost = "esp32";
  otaManager.begin(otaHost);  // carrega a senha salva (ou "senha" na primeira vez)

  // mDNS
  if (WiFi.status() == WL_CONNECTED) {
    if (!MDNS.begin(otaHost)) {
      Serial.println("Erro ao iniciar mDNS!");
    } else {
      Serial.print("mDNS iniciado! Acesse: http://");
      Serial.print(otaHost);
      Serial.println(".local");
      MDNS.addService("http", "tcp", 80);
    }
  }
}

void loop() {
  otaManager.handle();
  webServerManager.handleClient();
  ntpManager.update();
  relayManager.update(dhtManager.readTemperature());
}