#include <Arduino.h>
#include "WebServerManager.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

// Versao do firmware. O GitHub Actions define com a tag da Release (ex.: v2.1.0).
// Compilando no PC, fica "local".
#ifndef FW_VERSION
#define FW_VERSION "local"
#endif
#ifdef ESP_IDF_VERSION_MAJOR
#include <esp_flash.h>
#endif

// Pagina de RECUPERACAO embutida no firmware.
// Ela nunca depende do LittleFS: mesmo que o /index.html salvo esteja
// quebrado (sem JS, com erro etc.), esta pagina continua funcionando em
//   http://esp32.local/recovery
// E tambem a pagina mostrada em "/" quando nao existe /index.html salvo.
const char* initialHTML = R"rawliteral(
<!DOCTYPE html><html lang="pt-BR"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>ESP32 - Recuperacao</title>
<style>body{font-family:sans-serif;max-width:520px;margin:24px auto;padding:0 16px;line-height:1.5}button{padding:8px 14px;margin:6px 0;cursor:pointer}.danger{background:#c0392b;color:#fff;border:0;border-radius:4px}#msg{margin-top:12px;font-weight:bold}</style></head>
<body><h1>ESP32 - Modo de recuperacao</h1>
<p>Esta pagina vem do proprio firmware e funciona mesmo se o painel salvo estiver quebrado.</p>
<h3>1. Enviar um index.html</h3>
<input type="file" id="htmlUpload" accept=".html"><br><button onclick="uploadHTML()">Enviar HTML</button>
<h3>2. Apagar a pagina salva</h3>
<p>Remove o /index.html da memoria. O endereco principal passa a mostrar esta pagina ate voce enviar outro HTML.</p>
<button class="danger" onclick="resetHTML()">Apagar pagina salva</button>
<p><a href="/">Ir para a pagina principal</a></p><div id="msg"></div>
<script>
function msg(t){document.getElementById("msg").textContent=t;}
async function uploadHTML(){const f=document.getElementById("htmlUpload").files[0];if(!f){msg("Selecione um arquivo .html primeiro.");return;}
const r=await fetch("/upload-html",{method:"POST",headers:{"Content-Type":"text/plain"},body:await f.text()});msg(r.ok?"HTML enviado! Abra a pagina principal para conferir.":"Erro ao enviar o HTML.");}
async function resetHTML(){if(!confirm("Apagar a pagina salva na ESP32?"))return;
const r=await fetch("/reset-html",{method:"POST"});msg(await r.text());}
</script></body></html>
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
  // Recuperacao: pagina embutida no firmware (sempre funciona) e rota que apaga o /index.html
  server.on("/recovery", HTTP_GET, std::bind(&WebServerManager::handleRecovery, this));
  server.on("/reset-html", HTTP_ANY, std::bind(&WebServerManager::handleResetHtml, this));
  // Configuracoes da placa (sem precisar abrir o portal Wi-Fi)
  server.on("/set-ota-password", HTTP_POST, std::bind(&WebServerManager::handleSetOtaPassword, this));
  server.on("/device-info", HTTP_GET, std::bind(&WebServerManager::handleDeviceInfo, this));
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
    // Por seguranca, desliga o ventilador: enquanto o portal estiver aberto
    // (ate 3 min) a placa nao roda o resto do programa.
    relayManager->stop();
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
    
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    
    // Cria estrutura AutoSettings com dados do JSON
    AutoSettings settings;
    settings.active = doc["active"] | false;
    settings.minTemp = doc["temp"] | 30.0f;
    settings.ventTime = doc["ventTime"] | 15;
    settings.standbyTime = doc["standby"] | 30;
    settings.startTime = doc["startTime"] | "21:00";
    settings.endTime = doc["endTime"] | "05:00";

    // Passa as configuracoes para o RelayManager (ele decide o que fazer)
    relayManager->setAutoSettings(settings);

    server.send(200, "application/json", "{\"success\":true}");
}

void WebServerManager::handleGetAutoSettings() {
    // Pega as configuracoes diretamente do RelayManager
    AutoSettings settings = relayManager->getAutoSettings();
    
    JsonDocument doc;
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

// Sempre mostra a pagina de recuperacao embutida, ignorando o LittleFS
void WebServerManager::handleRecovery() {
  server.send(200, "text/html", initialHTML);
}

// Apaga o /index.html salvo; "/" volta a mostrar a pagina embutida.
// Aceita GET tambem, para funcionar so digitando o endereco no navegador:
//   http://esp32.local/reset-html
void WebServerManager::handleResetHtml() {
  if (!LittleFS.exists("/index.html")) {
    server.send(200, "text/plain", "Nao ha pagina salva. Ja esta usando a pagina do firmware.");
    return;
  }
  if (LittleFS.remove("/index.html")) {
    Serial.println("[WEB] /index.html apagado; usando a pagina do firmware.");
    server.send(200, "text/plain", "Pagina salva apagada! A pagina principal agora e a do firmware.");
  } else {
    server.send(500, "text/plain", "Erro ao apagar /index.html.");
  }
}

// Troca a senha do OTA. Espera JSON: {"current":"...","new":"..."}
void WebServerManager::handleSetOtaPassword() {
  if (!otaManager) {
    server.send(500, "text/plain", "OTA nao configurado.");
    return;
  }
  JsonDocument doc;
  if (!server.hasArg("plain") || deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "text/plain", "Dados invalidos.");
    return;
  }
  String error;
  if (otaManager->changePassword(doc["current"] | "", doc["new"] | "", error)) {
    server.send(200, "text/plain", "Senha do OTA alterada!");
  } else {
    server.send(error.startsWith("Senha atual") ? 403 : 400, "text/plain", error);
  }
}

// Informacoes da placa para o painel (nunca envia a senha)
void WebServerManager::handleDeviceInfo() {
  JsonDocument doc;
  doc["ssid"] = WiFi.SSID();
  doc["ip"] = WiFi.localIP().toString();
  doc["rssi"] = WiFi.RSSI();
  doc["hostname"] = "esp32.local";
  doc["firmware"] = FW_VERSION;
  doc["otaDefaultPassword"] = otaManager ? otaManager->isDefaultPassword() : true;
  String jsonStr;
  serializeJson(doc, jsonStr);
  server.send(200, "application/json", jsonStr);
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
  server.send(200, "text/plain", "Portal WiFi iniciado. Conecte-se a rede 'ESP32-Config' e abra 192.168.4.1");
}

void WebServerManager::handleRemaining() {
  JsonDocument doc;
  // CORRIGIDO: so informa tempo quando ha timer MANUAL rodando, e protege
  // contra conta negativa (unsigned) que virava um numero gigante.
  if (relayManager->isManualActive() && relayManager->isActive()) {
    unsigned long elapsed = millis() - relayManager->getStartTime();
    unsigned long duration = relayManager->getDuration();
    doc["remaining"] = (elapsed < duration) ? (duration - elapsed) / 1000 : 0;
  } else {
    doc["remaining"] = 0;
  }
  String jsonStr;
  serializeJson(doc, jsonStr);
  server.send(200, "application/json", jsonStr);
}

void WebServerManager::handleSensorData() {
    JsonDocument doc;
    float temp = this->dhtManager->readTemperature();
    float humidity = this->dhtManager->readHumidity();
    
    if (!isnan(temp) && !isnan(humidity)) {
      doc["temp"] = temp;
      doc["humidity"] = humidity;
      // CORRIGIDO: era 'temp + (humidity * 0.1f)' (formula sem base fisica).
      // Agora usa o indice de calor padrao NWS/NOAA (Rothfusz).
      doc["feelsLike"] = DHTManager::heatIndex(temp, humidity);
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
    JsonDocument doc;
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
  JsonDocument doc;
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