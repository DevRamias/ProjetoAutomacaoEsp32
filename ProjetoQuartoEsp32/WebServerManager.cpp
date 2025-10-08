#include <Arduino.h>
#include "WebServerManager.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#ifdef ESP_IDF_VERSION_MAJOR
#include <esp_flash.h>
#endif

// O HTML inicial permanece o mesmo.
const char* initialHTML = R"rawliteral(
<!DOCTYPE html><html><head><title>ESP32 Web Server</title></head><body><h1>Bem-vindo ao ESP32!</h1><p>Este é o HTML inicial carregado diretamente do código.</p><p>Use a rota <code>/upload-html</code> para atualizar este HTML.</p><input type="file" id="htmlUpload" accept=".html"><button onclick="uploadHTML()">Upload HTML</button><script>async function uploadHTML(){const file=document.getElementById("htmlUpload").files[0];if(!file){alert("Por favor, selecione um arquivo HTML.");return;}const response=await fetch("/upload-html",{method:"POST",headers:{"Content-Type":"text/plain"},body:await file.text()});if(response.ok){alert("HTML atualizado com sucesso!");}else{alert("Erro ao atualizar o HTML.");}}</script></body></html>
)rawliteral";

void WebServerManager::begin(RelayManager* relayManager, NTPManager* ntpManager, WiFiManager* wifiManager, DHTManager* dhtManager) {
  this->relayManager = relayManager;
  this->ntpManager = ntpManager;
  this->wifiManager = wifiManager;
  this->dhtManager = dhtManager;

  this->shouldStartPortal = false;
  this->_lastMemoryLog = 0;

  // Configuracao de rotas
  server.on("/", HTTP_GET, std::bind(&WebServerManager::handleRoot, this));
  server.on("/start", HTTP_GET, std::bind(&WebServerManager::handleStart, this));
  server.on("/stop", HTTP_GET, std::bind(&WebServerManager::handleStop, this));
  server.on("/time", HTTP_GET, std::bind(&WebServerManager::handleTime, this));
  server.on("/status", HTTP_GET, std::bind(&WebServerManager::handleStatus, this));
  server.on("/wificonfig", HTTP_GET, std::bind(&WebServerManager::handleWiFiConfig, this));
  server.on("/remaining", HTTP_GET, std::bind(&WebServerManager::handleRemaining, this));
  server.on("/system", HTTP_GET, std::bind(&WebServerManager::handleSystemInfo, this));
  server.on("/flash-info", HTTP_GET, std::bind(&WebServerManager::handleFlashInfo, this));
  server.on("/sensor-data", HTTP_GET, std::bind(&WebServerManager::handleSensorData, this));
  server.on("/upload-html", HTTP_POST, std::bind(&WebServerManager::handleUpload, this));
  server.on("/set-auto-settings", HTTP_POST, std::bind(&WebServerManager::handleSetAutoSettings, this));
  server.on("/get-auto-settings", HTTP_GET, std::bind(&WebServerManager::handleGetAutoSettings, this));

  server.begin();
  Serial.println("Servidor web iniciado!");
}

void WebServerManager::handleClient() {
  if (millis() - _lastMemoryLog > 30000) {
    logMemoryUsage();
    _lastMemoryLog = millis();
  }
  
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

// --- Implementacao dos Handlers ---

void WebServerManager::handleSetAutoSettings() {
    if (!server.hasArg("plain")) {
      server.send(400, "text/plain", "Dados faltando");
      return;
    }
    
    DynamicJsonDocument doc(256);
    deserializeJson(doc, server.arg("plain"));
    
    // Cria estrutura AutoSettings com dados do JSON
    AutoSettings settings;
    settings.active = doc["active"] | false;
    settings.minTemp = doc["temp"] | 30.0f;
    settings.ventTime = doc["ventTime"] | 15;
    settings.standbyTime = doc["standby"] | 30;
    settings.startTime = doc["startTime"] | "21:00";
    settings.endTime = doc["endTime"] | "5:00";

    // Passa as configuracoes para o RelayManager (ele decide o que fazer)
    relayManager->setAutoSettings(settings);

    server.send(200, "application/json", "{\"success\":true}");
}

void WebServerManager::handleGetAutoSettings() {
    // Pega as configuracoes diretamente do RelayManager
    AutoSettings settings = relayManager->getAutoSettings();
    
    DynamicJsonDocument doc(256);
    doc["active"] = settings.active;
    doc["temp"] = settings.minTemp;
    doc["ventTime"] = settings.ventTime;
    doc["standby"] = settings.standbyTime;
    doc["startTime"] = settings.startTime;
    doc["endTime"] = settings.endTime;
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    server.send(200, "application/json", jsonStr);
}

void WebServerManager::handleRoot() {
  if (LittleFS.exists("/index.html")) {
    File file = LittleFS.open("/index.html", "r");
    server.streamFile(file, "text/html");
    file.close();
  } else {
    server.send(200, "text/html", initialHTML);
  }
}

void WebServerManager::handleStart() {
  if (server.hasArg("duration")) {
    unsigned long duration = server.arg("duration").toInt();
    if (duration > 0 && duration <= 1440) {
      relayManager->start(duration);
      server.send(200, "text/plain", "Ventilador ligado por " + String(duration) + " minutos!");
    } else {
      server.send(400, "text/plain", "Duracao invalida (1-1440 minutos)");
    }
  } else {
    server.send(400, "text/plain", "Parametro 'duration' faltando");
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
  server.send(200, "text/plain", "Portal WiFi sera iniciado. Conecte-se ao AP 'ESP32-Config'");
}

void WebServerManager::handleRemaining() {
  DynamicJsonDocument doc(100);
  if (relayManager->isActive() && !relayManager->isAutoCycleActive()) {
    unsigned long elapsed = millis() - relayManager->getStartTime();
    doc["remaining"] = (relayManager->getDuration() - elapsed) / 1000;
  } else {
    doc["remaining"] = 0;
  }
  String jsonStr;
  serializeJson(doc, jsonStr);
  server.send(200, "application/json", jsonStr);
}

void WebServerManager::handleSensorData() {
    DynamicJsonDocument doc(200);
    float temp = this->dhtManager->readTemperature();
    float humidity = this->dhtManager->readHumidity();
    
    if (!isnan(temp) && !isnan(humidity)) {
      doc["temp"] = temp;
      doc["humidity"] = humidity;
      doc["feelsLike"] = temp + (humidity * 0.1f);
      doc["lastUpdate"] = this->dhtManager->getLastReadingTime() / 1000;
    } else {
      doc["error"] = "Erro na leitura do sensor";
    }
    String jsonStr;
    serializeJson(doc, jsonStr);
    server.send(200, "application/json", jsonStr);
}

  void WebServerManager::handleUpload() {
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
}

void WebServerManager::handleSystemInfo() {
    DynamicJsonDocument doc(256);
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t maxBlock = ESP.getMaxAllocHeap();
    doc["memory"]["free"] = freeHeap;
    doc["memory"]["min_free"] = ESP.getMinFreeHeap();
    doc["memory"]["max_alloc"] = maxBlock;
    doc["memory"]["fragmentation"] = (freeHeap > 0) ? 100 - (maxBlock * 100 / freeHeap) : 0;
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
               free, ESP.getMinFreeHeap(), maxBlock, 100 - (maxBlock * 100 / free));
}

void WebServerManager::handleFlashInfo() {
  DynamicJsonDocument doc(256);
  size_t flash_size;
  #if defined(ESP_IDF_VERSION_MAJOR) && ESP_IDF_VERSION_MAJOR >= 4
    flash_size = ESP.getFlashChipSize();
  #else
    esp_flash_get_size(nullptr, &flash_size);
  #endif
  doc["flash"]["total"] = flash_size;
  doc["flash"]["used"] = ESP.getSketchSize();
  doc["flash"]["free"] = flash_size - ESP.getSketchSize();
  doc["fs"]["total"] = LittleFS.totalBytes();
  doc["fs"]["used"] = LittleFS.usedBytes();
  doc["fs"]["free"] = LittleFS.totalBytes() - LittleFS.usedBytes();
  String jsonStr;
  serializeJson(doc, jsonStr);
  server.send(200, "application/json", jsonStr);
}