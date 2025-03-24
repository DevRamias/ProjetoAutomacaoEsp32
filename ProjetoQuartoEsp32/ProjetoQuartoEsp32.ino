#include <SPIFFS.h> // Inclua a biblioteca SPIFFS
#include "WiFiConfigManager.h" // Descomente esta linha
#include "WiFiManager.h"
#include "NTPManager.h"
#include "RelayManager.h"
#include "WebServerManager.h"

// Declare o objeto wifiConfigManager
WiFiConfigManager wifiConfigManager;

WiFiManager wifiManager;
NTPManager ntpManager;
RelayManager relayManager(5); // Pino do relé
WebServerManager webServerManager;

void setup() {
  Serial.begin(115200);

  // Inicializa o SPIFFS
  if (SPIFFS.begin(true)) {
    Serial.println("SPIFFS inicializado com sucesso!");

    // Verifica se o arquivo wifi.txt existe
    if (SPIFFS.exists("/wifi.txt")) {
      Serial.println("Arquivo wifi.txt encontrado!");

      // Tenta ler o arquivo wifi.txt
      File file = SPIFFS.open("/wifi.txt", "r");
      if (file) {
        String ssid = file.readStringUntil('\n');
        String password = file.readStringUntil('\n');
        file.close();

        // Remove espaços em branco e verifica se as credenciais são válidas
        ssid.trim();
        password.trim();

        if (ssid.length() > 0 && password.length() > 0) {
          Serial.println("Credenciais lidas com sucesso:");
          Serial.println("SSID: " + ssid);
          Serial.println("Senha: " + password);
        } else {
          Serial.println("Arquivo wifi.txt corrompido ou vazio. Apagando...");
          SPIFFS.remove("/wifi.txt"); // Apaga o arquivo corrompido
        }
      } else {
        Serial.println("Erro ao abrir o arquivo wifi.txt. Apagando...");
        SPIFFS.remove("/wifi.txt"); // Apaga o arquivo corrompido
      }
    } else {
      Serial.println("Arquivo wifi.txt não encontrado!");
    }
  } else {
    Serial.println("Erro ao inicializar o SPIFFS!");
    return;
  }

  // Inicializa o servidor web no WebServerManager
  webServerManager.begin(&relayManager, &ntpManager);

  // Inicia o WiFiConfigManager
  wifiConfigManager.begin(); // Inicia o WiFiConfigManager

  // Comente esta linha se estiver usando o WiFiConfigManager
  // wifiManager.connect("SUA_REDE_WIFI", "SUA_SENHA_WIFI");

  // Inicializa o NTP e o relé
  ntpManager.begin();
  relayManager.begin();
}

void loop() {
  webServerManager.handleClient(); // Gerencia as requisições do servidor
  ntpManager.update();             // Atualiza o horário
  relayManager.update();           // Atualiza o relé
}