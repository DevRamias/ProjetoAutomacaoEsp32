#include <Arduino.h>
#include "WebServerManager.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

void WebServerManager::begin(RelayManager* relayManager, NTPManager* ntpManager, WiFiManager* wifiManager, DHTManager* dhtManager) {
  // Inicializa ponteiros
  this->relayManager = relayManager;
  this->ntpManager = ntpManager;
  this->wifiManager = wifiManager;
  this->dhtManager = dhtManager;

  // Configura valores padrão
  this->autoModeActive = false;
  this->autoMinTemp = 28.0f;
  this->autoCheckIntervalMinutes = 10;
  this->ventilationDuration = 15;    // 15 minutos ligado por padrão
  this->standbyDuration = 10;       // 10 minutos de standby por padrão
  this->shouldStartPortal = false;

  // Configura rotas básicas
  server.on("/", HTTP_GET, [this]() { this->handleRoot(); });
  server.on("/start", HTTP_GET, [this]() { this->handleStart(); });
  server.on("/stop", HTTP_GET, [this]() { this->handleStop(); });
  server.on("/time", HTTP_GET, [this]() { this->handleTime(); });
  server.on("/status", HTTP_GET, [this]() { this->handleStatus(); });
  server.on("/wificonfig", HTTP_GET, [this]() { this->handleWiFiConfig(); });
  server.on("/remaining", HTTP_GET, [this]() { this->handleRemaining(); });

  // Rota para dados do sensor
  server.on("/sensor-data", HTTP_GET, [this]() {
    DynamicJsonDocument doc(200);
    float temp = this->dhtManager->readTemperature();
    float humidity = this->dhtManager->readHumidity();
    
    if (!isnan(temp) && !isnan(humidity)) {
Serial.printf("Temp: %.1fC | Umidade: %.1f%%\n", temp, humidity);
      doc["temp"] = temp;
      doc["humidity"] = humidity;
      doc["feelsLike"] = temp + (humidity * 0.1f); // Sensação térmica
    } else {
Serial.println("Erro na leitura do sensor DHT");
      doc["error"] = "Erro na leitura do sensor";
    }

    String jsonStr;
    serializeJson(doc, jsonStr);
    server.send(200, "application/json", jsonStr);
  });

  // Rota para configurações automáticas
  server.on("/set-auto-settings", HTTP_POST, [this]() {
    if (server.hasArg("plain")) {
        DynamicJsonDocument doc(256);
        deserializeJson(doc, server.arg("plain"));
        
        this->autoModeActive = doc["active"];
        this->autoMinTemp = doc["temp"];
        this->ventilationDuration = doc["ventTime"];
        this->standbyDuration = doc["standby"];

        Serial.printf("Config Auto: %s | Temp: %.1fC | Vent: %dmin | Standby: %dmin\n",
            autoModeActive ? "ON" : "OFF",
            autoMinTemp,
            ventilationDuration,
            standbyDuration);

        server.send(200, "application/json", "{\"success\":true}");
    } else {
        server.send(400, "text/plain", "Dados faltando");
    }
});

// Rota para obter configurações (opcional)
server.on("/get-auto-settings", HTTP_GET, [this]() {
    DynamicJsonDocument doc(200);
    doc["active"] = this->autoModeActive;
    doc["temp"] = this->autoMinTemp;
    doc["ventTime"] = this->ventilationDuration;
    doc["standby"] = this->standbyDuration;
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    server.send(200, "application/json", jsonStr);
});

  server.begin();
  Serial.println("Servidor web iniciado!");
}

void WebServerManager::handleClient() {
  static bool portalRequested = false;
  static unsigned long portalStartTime = 0;
  static unsigned long lastCheck = 0;
  
  // Verificação automática (a cada X minutos)
  if (autoModeActive && millis() - lastCheck >= (autoCheckIntervalMinutes * 60000)) {
    verificarCondicoesAutomaticas();  // Você implementará essa função
    lastCheck = millis();
  }
  
  if (shouldStartPortal && !portalRequested) {
    portalRequested = true;
    portalStartTime = millis() + 100; // Pequeno delay para responder ao cliente
  }
  
  if (portalRequested && millis() >= portalStartTime) {
    shouldStartPortal = false;
    portalRequested = false;
    server.stop();
    wifiManager->startConfigPortal("ESP32-Config");
    server.begin();
  }
  
  server.handleClient();
}

void WebServerManager::handleRoot() {
    File file = LittleFS.open("/index.html", "r");  // Abre o arquivo HTML
    if (!file) {
        server.send(500, "text/plain", "Erro: HTML não encontrado!");
        return;
    }
    server.streamFile(file, "text/html");  // Envia o arquivo
    file.close();
}

void WebServerManager::handleStart() {
  if (server.hasArg("duration")) {
    unsigned long duration = server.arg("duration").toInt();
    relayManager->start(duration);
    server.send(200, "text/plain", "Ventilador ligado por " + String(duration) + " minutos!");
  } else {
    server.send(400, "text/plain", "Parâmetro 'duration' faltando");
  }
}

void WebServerManager::handleStop() {
  relayManager->stop();
  server.send(200, "text/plain", "Ventilador desligado manualmente!");
}

void WebServerManager::handleTime() {
  server.send(200, "text/plain", ntpManager->getFormattedTime());
}

void WebServerManager::handleStatus() {
  String status = relayManager->isActive() ? "Ventilador Ligado" : "Ventilador Desligado";
  server.send(200, "text/plain", status);
}

void WebServerManager::handleWiFiConfig() {
  shouldStartPortal = true;
  server.send(200, "text/plain", "Portal WiFi será iniciado. Conecte-se ao AP 'ESP32-Config'");
}

void WebServerManager::handleRemaining() {
  if (relayManager->isActive()) {
    unsigned long elapsed = millis() - relayManager->getStartTime();
    unsigned long remaining = (relayManager->getDuration() - elapsed) / 1000; // Converte para segundos
    String json = "{\"remaining\":" + String(remaining) + "}";
    server.send(200, "application/json", json);
  } else {
    server.send(200, "application/json", "{\"remaining\":0}");
  }
}

void WebServerManager::verificarCondicoesAutomaticas() {
    if (!autoModeActive) return;

    static unsigned long lastActionTime = 0;
    static bool isInStandby = false;
    unsigned long now = millis();

    // Se o ventilador está ligado, verifica se já passou o tempo de ventilação
    if (relayManager->isActive()) {
        if (now - relayManager->getStartTime() >= (ventilationDuration * 60000)) {
            relayManager->stop();
            isInStandby = true;
            lastActionTime = now;
            Serial.println("Ventilador desligado (fim do ciclo)");
        }
    } 
    // Se está em standby, verifica se já passou o tempo
    else if (isInStandby) {
        if (now - lastActionTime >= (standbyDuration * 60000)) {
            isInStandby = false;
            Serial.println("Standby concluído, verificando temperatura...");
        }
    }
    // Se não está em standby, verifica as condições
    else {
        float temp = dhtManager->readTemperature();
        float humidity = dhtManager->readHumidity();
        
        if (!isnan(temp) && !isnan(humidity)) {
            float feelsLike = temp + (humidity * 0.1f);
            Serial.printf("Temp: %.1fC | Umidade: %.1f%% | Sensação: %.1fC\n", temp, humidity, feelsLike);
            
            if (feelsLike >= autoMinTemp) {
                relayManager->start(ventilationDuration);
                lastActionTime = now;
                Serial.println("Ventilador ligado (temperatura alta)");
            }
        }
    }
}