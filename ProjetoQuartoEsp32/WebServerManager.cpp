#include <Arduino.h>
#include "WebServerManager.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#ifdef ESP_IDF_VERSION_MAJOR
#include <esp_flash.h>
#endif

// HTML inicial armazenado em uma constante
const char* initialHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Web Server</title>
</head>
<body>
    <h1>Bem-vindo ao ESP32!</h1>
    <p>Este é o HTML inicial carregado diretamente do código.</p>
    <p>Use a rota <code>/upload-html</code> para atualizar este HTML.</p>
    <input type="file" id="htmlUpload" accept=".html">
    <button onclick="uploadHTML()">Upload HTML</button>
    <script>
        async function uploadHTML() {
            const file = document.getElementById("htmlUpload").files[0];
            if (!file) {
                alert("Por favor, selecione um arquivo HTML.");
                return;
            }
            const response = await fetch("/upload-html", {
                method: "POST",
                headers: {
                    "Content-Type": "text/plain"
                },
                body: await file.text()
            });
            if (response.ok) {
                alert("HTML atualizado com sucesso!");
            } else {
                alert("Erro ao atualizar o HTML.");
            }
        }
    </script>
</body>
</html>
)rawliteral";

void WebServerManager::begin(RelayManager* relayManager, NTPManager* ntpManager, WiFiManager* wifiManager, DHTManager* dhtManager) {
  // Inicializa ponteiros
  this->relayManager = relayManager;
  this->ntpManager = ntpManager;
  this->wifiManager = wifiManager;
  this->dhtManager = dhtManager;

  // Configura valores padrão
  this->autoModeActive = false;
  this->autoMinTemp = 31.0f;
  this->autoCheckIntervalMinutes = 10;
  this->ventilationDuration = 15;    // 15 minutos ligado por padrão
  this->standbyDuration = 30;       // 30 minutos de standby por padrão
  this->autoStartTime = "05:00";    // Horário de início padrão
  this->autoEndTime = "22:00";      // Horário de fim padrão
  this->shouldStartPortal = false;
  this->_lastMemoryLog = 0;

  // Configura rotas básicas
  server.on("/", HTTP_GET, [this]() { this->handleRoot(); });
  server.on("/start", HTTP_GET, [this]() { this->handleStart(); });
  server.on("/stop", HTTP_GET, [this]() { this->handleStop(); });
  server.on("/time", HTTP_GET, [this]() { this->handleTime(); });
  server.on("/status", HTTP_GET, [this]() { this->handleStatus(); });
  server.on("/wificonfig", HTTP_GET, [this]() { this->handleWiFiConfig(); });
  server.on("/remaining", HTTP_GET, [this]() { this->handleRemaining(); });
  server.on("/system", HTTP_GET, [this]() { this->handleSystemInfo(); });
  server.on("/flash-info", HTTP_GET, [this]() { this->handleFlashInfo(); });

  // Rota para dados do sensor (com cache automático via DHTManager)
  server.on("/sensor-data", HTTP_GET, [this]() {
    DynamicJsonDocument doc(200);
    
    // O DHTManager já implementa cache internamente
    float temp = this->dhtManager->readTemperature();
    float humidity = this->dhtManager->readHumidity();
    
    if (!isnan(temp) && !isnan(humidity)) {
      doc["temp"] = temp;
      doc["humidity"] = humidity;
      doc["feelsLike"] = temp + (humidity * 0.1f); // Sensação térmica
      doc["lastUpdate"] = this->dhtManager->getLastReadingTime() / 1000; // em segundos
    } else {
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
      DeserializationError error = deserializeJson(doc, server.arg("plain"));
      
      if (error) {
        server.send(400, "text/plain", "JSON inválido");
        return;
      }
      
      this->autoModeActive = doc["active"] | false;
      this->autoMinTemp = doc["temp"] | 31.0f;
      this->ventilationDuration = doc["ventTime"] | 15;  // 15 minutos ligado
      this->standbyDuration = doc["standby"] | 30;      // 30 minutos de standby
      this->autoStartTime = doc["startTime"] | "05:00"; // Começa às 5h
      this->autoEndTime = doc["endTime"] | "22:00";     // Termina às 22h

      // Verifica imediatamente se deve ligar quando o modo automático é ativado
      if (this->autoModeActive) {
        verificarCondicoesAutomaticas();
      }

      server.send(200, "application/json", "{\"success\":true}");
    } else {
      server.send(400, "text/plain", "Dados faltando");
    }
  });

  // Rota para obter configurações
  server.on("/get-auto-settings", HTTP_GET, [this]() {
    DynamicJsonDocument doc(200);
    doc["active"] = this->autoModeActive;
    doc["temp"] = this->autoMinTemp;
    doc["ventTime"] = this->ventilationDuration;
    doc["standby"] = this->standbyDuration;
    doc["startTime"] = this->autoStartTime;
    doc["endTime"] = this->autoEndTime;
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    server.send(200, "application/json", jsonStr);
  });

  // Rota para upload de HTML
  server.on("/upload-html", HTTP_POST, [this]() {
    if (server.hasArg("plain")) {
      File file = LittleFS.open("/index.html", "w");
      if (!file) {
        server.send(500, "text/plain", "Erro ao abrir o arquivo para escrita.");
        return;
      }
      file.print(server.arg("plain"));
      file.close();
      server.send(200, "text/plain", "HTML atualizado com sucesso!");
    } else {
      server.send(400, "text/plain", "Nenhum dado recebido.");
    }
  });

  server.begin();
  Serial.println("Servidor web iniciado!");
}

void WebServerManager::handleClient() {
  // Log de uso de memória a cada 30 segundos
  if (millis() - _lastMemoryLog > 30000) {
    logMemoryUsage();
    _lastMemoryLog = millis();
  }

  // Verificação automática (a cada X minutos)
  static unsigned long lastCheck = 0;
  if (autoModeActive && millis() - lastCheck >= (autoCheckIntervalMinutes * 60000)) {
    verificarCondicoesAutomaticas();
    lastCheck = millis();
  }
  
  // Configuração WiFi se necessário
  static bool portalRequested = false;
  static unsigned long portalStartTime = 0;
  
  if (shouldStartPortal && !portalRequested) {
    portalRequested = true;
    portalStartTime = millis() + 100;
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
  if (LittleFS.exists("/index.html")) {
    File file = LittleFS.open("/index.html", "r");
    if (file) {
      server.streamFile(file, "text/html");
      file.close();
      return;
    }
  }
  server.send(200, "text/html", initialHTML);
}

void WebServerManager::handleStart() {
  if (server.hasArg("duration")) {
    unsigned long duration = server.arg("duration").toInt();
    if (duration > 0 && duration <= 1440) { // Validação (1 min a 24h)
      relayManager->start(duration);
      server.send(200, "text/plain", "Ventilador ligado por " + String(duration) + " minutos!");
    } else {
      server.send(400, "text/plain", "Duração inválida (1-1440 minutos)");
    }
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
  server.send(200, "text/plain", relayManager->isActive() ? "Ventilador Ligado" : "Ventilador Desligado");
}

void WebServerManager::handleWiFiConfig() {
  shouldStartPortal = true;
  server.send(200, "text/plain", "Portal WiFi será iniciado. Conecte-se ao AP 'ESP32-Config'");
}

void WebServerManager::handleRemaining() {
  DynamicJsonDocument doc(100);
  
  if (relayManager->isActive()) {
    unsigned long elapsed = millis() - relayManager->getStartTime();
    doc["remaining"] = (relayManager->getDuration() - elapsed) / 1000; // em segundos
  } else {
    doc["remaining"] = 0;
  }
  
  String jsonStr;
  serializeJson(doc, jsonStr);
  server.send(200, "application/json", jsonStr);
}

bool WebServerManager::isWithinActiveHours() {
  String currentTime = ntpManager->getFormattedTime();
  
  // Extrai apenas a hora e minuto do horário atual (ignora segundos)
  String currentHourMin = currentTime.substring(0, 5);
  
  // Se horário atual está entre início e fim
  if (autoStartTime <= autoEndTime) {
    return currentHourMin >= autoStartTime && currentHourMin <= autoEndTime;
  } else {
    // Caso especial quando o período atravessa a meia-noite
    return currentHourMin >= autoStartTime || currentHourMin <= autoEndTime;
  }
}

void WebServerManager::verificarCondicoesAutomaticas() {
  if (!autoModeActive) return;

  // Verifica se está dentro do horário permitido
  if (!isWithinActiveHours()) {
    if (relayManager->isActive()) {
      relayManager->stop(); // Desliga se estiver fora do horário permitido
    }
    return;
  }

  static unsigned long lastActionTime = 0;
  static bool isInStandby = false;
  unsigned long now = millis();

  if (relayManager->isActive()) {
    if (now - relayManager->getStartTime() >= (ventilationDuration * 60000)) {
      relayManager->stop();
      isInStandby = true;
      lastActionTime = now;
    }
  } 
  else if (isInStandby) {
    if (now - lastActionTime >= (standbyDuration * 60000)) {
      isInStandby = false;
    }
  }
  else {
    float temp = dhtManager->readTemperature();
    float humidity = dhtManager->readHumidity();
    
    if (!isnan(temp) && !isnan(humidity)) {
      float feelsLike = temp + (humidity * 0.1f);
      if (feelsLike >= autoMinTemp) {
        relayManager->start(ventilationDuration);
        lastActionTime = now;
      }
    }
  }
}

void WebServerManager::handleSystemInfo() {
    DynamicJsonDocument doc(256);
    
    // Informações de memória
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t maxBlock = ESP.getMaxAllocHeap();
    
    doc["memory"]["free"] = freeHeap;
    doc["memory"]["min_free"] = ESP.getMinFreeHeap();
    doc["memory"]["max_alloc"] = maxBlock;
    doc["memory"]["fragmentation"] = (freeHeap > 0) ? 100 - (maxBlock * 100 / freeHeap) : 0;
    
    // Informações do sistema
    doc["system"]["uptime"] = millis() / 1000;
    doc["system"]["chip_model"] = ESP.getChipModel();
    doc["system"]["cpu_freq"] = ESP.getCpuFreqMHz();
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    server.send(200, "application/json", jsonStr);
}

void WebServerManager::logMemoryUsage() {
  uint32_t free = ESP.getFreeHeap();
  uint32_t maxBlock = ESP.getMaxAllocHeap();
  
  Serial.printf("[MEM] Free: %6d | Min Free: %6d | Max Block: %6d | Frag: %2d%%\n",
               free,
               ESP.getMinFreeHeap(),
               maxBlock,
               100 - (maxBlock * 100 / free));
}

void WebServerManager::handleFlashInfo() {
  DynamicJsonDocument doc(256);
  
  // Método universal para obter tamanho da flash
  size_t flash_size;
  #if defined(ESP_IDF_VERSION_MAJOR) && ESP_IDF_VERSION_MAJOR >= 4
    flash_size = ESP.getFlashChipSize();
  #else
    esp_flash_get_size(nullptr, &flash_size);
  #endif
  
  doc["flash"]["total"] = flash_size;
  doc["flash"]["used"] = ESP.getSketchSize();
  doc["flash"]["free"] = flash_size - ESP.getSketchSize();
  
  // Informações do sistema de arquivos
  doc["fs"]["total"] = LittleFS.totalBytes();
  doc["fs"]["used"] = LittleFS.usedBytes();
  doc["fs"]["free"] = LittleFS.totalBytes() - LittleFS.usedBytes();
  
  String jsonStr;
  serializeJson(doc, jsonStr);
  server.send(200, "application/json", jsonStr);
}