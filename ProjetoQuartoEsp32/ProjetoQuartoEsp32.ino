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

#define OTA_CONFIG_FILE "/ota_config.json"

// Protótipos
void setup();
void loop();

WiFiManager wifiManager;
NTPManager ntpManager;
RelayManager relayManager(5);
DHTManager dhtManager(DHTPIN, DHTTYPE);
WebServerManager webServerManager;
OTAManager otaManager;

// Flag que indica que o portal terminou e devemos salvar
bool shouldSaveConfig = false;

// Parâmetro personalizado do portal (senha OTA)
WiFiManagerParameter otaPasswordParam(
    "ota_password",
    "Senha para OTA (deixe vazio para desabilitar)",
    "",
    32,
    "type='password' maxlength='32'"
);

// Callback chamado pelo WiFiManager quando o portal salva
void saveConfigCallback() {
  shouldSaveConfig = true;
}

// Lê a senha OTA salva no LittleFS
String loadOtaPassword() {
  if (!LittleFS.exists(OTA_CONFIG_FILE)) {
    return "";
  }
  File file = LittleFS.open(OTA_CONFIG_FILE, "r");
  if (!file) {
    Serial.println("Erro ao abrir arquivo de config OTA");
    return "";
  }
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error) {
    Serial.println("Erro ao ler JSON da config OTA");
    return "";
  }
  return doc["ota_password"].as<String>();
}

// Salva a senha OTA no LittleFS
void saveOtaPassword(const String& password) {
  JsonDocument doc;
  doc["ota_password"] = password;

  File file = LittleFS.open(OTA_CONFIG_FILE, "w");
  if (!file) {
    Serial.println("Erro ao abrir arquivo para salvar config OTA");
    return;
  }
  if (serializeJson(doc, file) == 0) {
    Serial.println("Erro ao escrever config OTA");
  } else {
    Serial.println("Config OTA salva com sucesso");
  }
  file.close();
}

void setup() {
  Serial.begin(115200);

  // Inicializa o sistema de arquivos
  if (!LittleFS.begin()) {
    Serial.println("Erro ao montar o sistema de arquivos!");
    return;
  }
  Serial.println("LittleFS inicializado!");

  // Configura o portal WiFi
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&otaPasswordParam);

  // Tenta conectar; se falhar, abre o portal
  if (!wifiManager.autoConnect("ESP32-Config")) {
    Serial.println("Falha na conexão e timeout do portal");
    delay(3000);
    ESP.restart();
  }

  Serial.println("Conectado ao WiFi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Se o portal foi usado, salva a senha OTA informada
  if (shouldSaveConfig) {
    String newPassword = otaPasswordParam.getValue();
    saveOtaPassword(newPassword);
    Serial.println("Senha OTA atualizada via portal");
  }

  // Carrega a senha OTA (do portal ou de configuração anterior)
  String otaPassword = loadOtaPassword();
  if (otaPassword.length() == 0) {
    Serial.println("AVISO: OTA sem senha configurada.");
  } else {
    Serial.println("OTA com senha carregada do LittleFS.");
  }

  // Inicializa os managers
  ntpManager.begin();
  relayManager.begin();
  relayManager.setNTPManager(&ntpManager);
  dhtManager.begin();  // CORRIGIDO: o DHT precisa ser inicializado antes de ser lido
  webServerManager.begin(&relayManager, &ntpManager, &wifiManager, &dhtManager);

  // Configura OTA
  const char* otaHost = "esp32";
  otaManager.begin(otaHost, otaPassword.c_str());

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