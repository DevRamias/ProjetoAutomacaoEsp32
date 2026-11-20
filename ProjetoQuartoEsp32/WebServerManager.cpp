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

// Handler "invisivel": o WebServer pergunta a ele se atende cada requisicao.
// Ele so anota que houve atividade e responde "nao", deixando a requisicao
// seguir para a rota certa.
class ActivityHandler : public RequestHandler {
public:
  explicit ActivityHandler(WebServerManager* manager) : _manager(manager) {}
  bool canHandle(HTTPMethod method, String uri) override {
    (void) method; (void) uri;
    _manager->markActivity();
    return false;
  }
private:
  WebServerManager* _manager;
};

void WebServerManager::begin(RelayManager* relayManager, NTPManager* ntpManager, WiFiManager* wifiManager, DHTManager* dhtManager) {
  this->relayManager = relayManager;
  this->ntpManager = ntpManager;
  this->wifiManager = wifiManager;
  this->dhtManager = dhtManager;

  this->shouldStartPortal = false;
  this->_lastMemoryLog = 0;

  // Comeca economizando energia; a primeira requisicao desliga a economia.
  WiFi.setSleep(true);
  _fastWiFi = false;

  // Observador de atividade: TEM que ser o primeiro handler registrado, para
  // ser consultado em toda requisicao. Ele nunca responde nada (retorna false),
  // entao as rotas abaixo continuam funcionando exatamente como antes.
  server.addHandler(new ActivityHandler(this));

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
  // 2o handler: recebe o corpo em pedacos direto para o arquivo (sem RAM)
  server.on("/upload-html", HTTP_POST, std::bind(&WebServerManager::handleUpload, this),
            std::bind(&WebServerManager::handleUploadRaw, this));
  // Recuperacao: pagina embutida no firmware (sempre funciona) e rota que apaga o /index.html
  server.on("/recovery", HTTP_GET, std::bind(&WebServerManager::handleRecovery, this));
  server.on("/reset-html", HTTP_ANY, std::bind(&WebServerManager::handleResetHtml, this));
  // Configuracoes da placa (sem precisar abrir o portal Wi-Fi)
  server.on("/set-ota-password", HTTP_POST, std::bind(&WebServerManager::handleSetOtaPassword, this));
  server.on("/device-info", HTTP_GET, std::bind(&WebServerManager::handleDeviceInfo, this));
  server.on("/set-auto-settings", HTTP_POST, std::bind(&WebServerManager::handleSetAutoSettings, this));
  server.on("/get-auto-settings", HTTP_GET, std::bind(&WebServerManager::handleGetAutoSettings, this));
  // Tudo que o painel precisa a cada poucos segundos, numa requisicao so
  server.on("/state", HTTP_GET, std::bind(&WebServerManager::handleState, this));
  // Historico das ultimas 24 h para o grafico (uma amostra a cada 5 min)
  server.on("/history", HTTP_GET, std::bind(&WebServerManager::handleHistory, this));

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
    // O portal mexe no Wi-Fi; volta para o modo economico padrao
    WiFi.setSleep(true);
    _fastWiFi = false;
  }

  server.handleClient();
  updatePowerSave();
}

void WebServerManager::markActivity() {
  _lastActivity = millis();
  if (!_fastWiFi) {
    WiFi.setSleep(false);
    _fastWiFi = true;
    Serial.println("[WIFI] Painel em uso: economia de energia desligada.");
  }
}

void WebServerManager::updatePowerSave() {
  if (_fastWiFi && millis() - _lastActivity > FAST_WIFI_MS) {
    WiFi.setSleep(true);
    _fastWiFi = false;
    Serial.println("[WIFI] Painel parado: economia de energia ligada.");
  }
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
    invalidateFsCache();
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

// Segundos restantes do timer MANUAL (0 se nao houver timer manual rodando)
unsigned long WebServerManager::remainingSeconds() {
  // CORRIGIDO: so informa tempo quando ha timer MANUAL rodando, e protege
  // contra conta negativa (unsigned) que virava um numero gigante.
  if (relayManager->isManualActive() && relayManager->isActive()) {
    unsigned long elapsed = millis() - relayManager->getStartTime();
    unsigned long duration = relayManager->getDuration();
    return (elapsed < duration) ? (duration - elapsed) / 1000 : 0;
  }
  return 0;
}

// Estado completo para o painel. Substitui /time + /status + /remaining +
// /get-auto-settings + /sensor-data (as rotas antigas continuam existindo).
void WebServerManager::handleState() {
  JsonDocument doc;
  doc["time"] = ntpManager->getFormattedTime();
  doc["synced"] = relayManager->isTimeSynced();
  bool ligado = relayManager->isActive();
  doc["on"] = ligado;
  doc["status"] = ligado ? "Ventilador Ligado" : "Ventilador Desligado";
  doc["remaining"] = remainingSeconds();

  AutoSettings s = relayManager->getAutoSettings();
  JsonObject a = doc["auto"].to<JsonObject>();
  a["active"] = s.active;
  a["temp"] = s.minTemp;
  a["ventTime"] = s.ventTime;
  a["standby"] = s.standbyTime;
  a["startTime"] = s.startTime;
  a["endTime"] = s.endTime;

  // Usa o cache do sensor (o DHT so e lido de novo a cada 15 s)
  JsonObject sensor = doc["sensor"].to<JsonObject>();
  float temp = dhtManager->readTemperature();
  float humidity = dhtManager->readHumidity();
  if (!isnan(temp) && !isnan(humidity)) {
    sensor["temp"] = temp;
    sensor["humidity"] = humidity;
    sensor["feelsLike"] = DHTManager::heatIndex(temp, humidity);
    sensor["lastUpdate"] = dhtManager->getLastReadingTime() / 1000;
    sensor["humidityHeld"] = dhtManager->isHumidityHeld();
  } else {
    sensor["error"] = "Erro na leitura do sensor";
  }

  String jsonStr;
  serializeJson(doc, jsonStr);
  server.send(200, "application/json", jsonStr);
}

// Historico para o grafico. JSON compacto, em arrays paralelos:
//   {"now":epoch,"step":300,"auto":true,"trigger":30.0,
//    "t":[epoch...],"temp":[25.0,...],"hum":[60,...],"on":[0,1,...]}
// Sem leitura vai null. So entram amostras das ultimas 24 h.
// Vai em pedacos (chunked) para nao montar ~6 KB de uma vez na RAM.
void WebServerManager::handleHistory() {
  time_t agora = time(nullptr);
  bool synced = ntpManager->isSynced();
  AutoSettings s = relayManager->getAutoSettings();

  // Primeira amostra dentro das ultimas 24 h
  uint16_t total = historyManager ? historyManager->count() : 0;
  uint16_t inicio = 0;
  if (synced) {
    uint32_t limite = (uint32_t) agora > 86400 ? (uint32_t) agora - 86400 : 0;
    while (inicio < total && historyManager->at(inicio).time < limite) inicio++;
  }

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");

  String out;
  out.reserve(1200);
  out += "{\"now\":";
  out += synced ? String((uint32_t) agora) : String("null");
  out += ",\"step\":";
  out += String(HistoryManager::STEP_SECONDS);
  out += ",\"auto\":";
  out += s.active ? "true" : "false";
  out += ",\"trigger\":";
  out += String(s.minTemp, 1);

  // Uma passada por campo; manda o texto sempre que passa de ~1 KB
  const char* nomes[] = { "t", "temp", "hum", "on" };
  for (uint8_t campo = 0; campo < 4; campo++) {
    out += ",\"";
    out += nomes[campo];
    out += "\":[";
    for (uint16_t i = inicio; i < total; i++) {
      const HistoryManager::Sample& a = historyManager->at(i);
      if (i > inicio) out += ',';
      char num[16];
      switch (campo) {
        case 0: snprintf(num, sizeof(num), "%lu", (unsigned long) a.time); break;
        case 1:
          if (a.temp10 == INT16_MIN) strcpy(num, "null");
          else snprintf(num, sizeof(num), "%.1f", a.temp10 / 10.0f);
          break;
        case 2:
          if (a.humidity == 255) strcpy(num, "null");
          else snprintf(num, sizeof(num), "%u", (unsigned) a.humidity);
          break;
        default: strcpy(num, (a.flags & 1) ? "1" : "0"); break;
      }
      out += num;
      if (out.length() > 1000) {
        server.sendContent(out);
        out = "";
      }
    }
    out += ']';
  }
  out += '}';
  server.sendContent(out);
  server.sendContent("");  // fim da resposta em pedacos
}

void WebServerManager::handleRemaining() {
  JsonDocument doc;
  doc["remaining"] = remainingSeconds();
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
      doc["humidityHeld"] = this->dhtManager->isHumidityHeld();
    } else {
      doc["error"] = "Erro na leitura do sensor";
    }
    String jsonStr;
    serializeJson(doc, jsonStr);
    server.send(200, "application/json", jsonStr);
}

// ---------------------------------------------------------------------------
// Envio do index.html
// CORRIGIDO: antes o HTML inteiro ia para a RAM (duas copias) e, se faltasse
// memoria, a copia vinha VAZIA sem aviso: o /index.html era apagado e gravado
// com 0 bytes, deixando o painel em branco (aconteceu com uma pagina de 48 KB).
// Agora:
//   1) o corpo chega em pedacos de ~1,4 KB e vai direto para /index.tmp;
//   2) so se o tamanho recebido bater com o enviado, o temporario substitui o
//      /index.html. Em qualquer falha a pagina atual continua intacta.
// ---------------------------------------------------------------------------
static const char* UPLOAD_TMP = "/index.tmp";

void WebServerManager::handleUploadRaw() {
  HTTPRaw& raw = server.raw();
  switch (raw.status) {
    case RAW_START:
      _uploadFailed = false;
      _uploadSize = 0;
      LittleFS.remove(UPLOAD_TMP);
      _uploadFile = LittleFS.open(UPLOAD_TMP, "w");
      if (!_uploadFile) _uploadFailed = true;
      break;
    case RAW_WRITE:
      if (!_uploadFailed && _uploadFile) {
        size_t escrito = _uploadFile.write(raw.buf, raw.currentSize);
        if (escrito != raw.currentSize) _uploadFailed = true;  // ex.: memoria cheia
        _uploadSize += escrito;
      }
      break;
    case RAW_END:
      if (_uploadFile) _uploadFile.close();
      break;
    case RAW_ABORTED:
      // Conexao caiu no meio: descarta o temporario, pagina atual intacta
      if (_uploadFile) _uploadFile.close();
      LittleFS.remove(UPLOAD_TMP);
      _uploadFailed = true;
      Serial.println("[WEB] Envio do HTML interrompido; pagina atual mantida.");
      break;
  }
}

void WebServerManager::handleUpload() {
  size_t esperado = server.clientContentLength() > 0 ? (size_t) server.clientContentLength() : 0;
  bool completo = !_uploadFailed && _uploadSize > 0 && _uploadSize == esperado;
  _uploadFailed = false;

  if (!completo) {
    LittleFS.remove(UPLOAD_TMP);
    Serial.printf("[WEB] Envio do HTML recusado (%u de %u bytes); pagina atual mantida.\n",
                  (unsigned) _uploadSize, (unsigned) esperado);
    server.send(esperado == 0 ? 400 : 500, "text/plain",
                esperado == 0 ? "Nenhum dado recebido." : "Envio incompleto. A pagina atual foi mantida.");
    return;
  }

  // Troca a pagina so agora, com o arquivo novo completo
  if (!LittleFS.rename(UPLOAD_TMP, "/index.html")) {
    LittleFS.remove("/index.html");
    if (!LittleFS.rename(UPLOAD_TMP, "/index.html")) {
      server.send(500, "text/plain", "Erro ao salvar o HTML. Use /recovery.");
      return;
    }
  }
  invalidateFsCache();
  Serial.printf("[WEB] index.html atualizado (%u bytes).\n", (unsigned) _uploadSize);
  server.send(200, "text/plain", "HTML atualizado com sucesso!");
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

// LittleFS.usedBytes() percorre o sistema de arquivos (~0,5 s). Guarda o valor
// por 5 min; enviar ou apagar o HTML invalida o cache na hora.
size_t WebServerManager::fsUsedBytes() {
  if (!_fsUsedValid || millis() - _fsUsedAt > 300000) {
    _fsUsedCache = LittleFS.usedBytes();
    _fsUsedAt = millis();
    _fsUsedValid = true;
  }
  return _fsUsedCache;
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
  size_t fsUsed = fsUsedBytes();
  doc["fs"]["used"] = fsUsed;
  doc["fs"]["free"] = LittleFS.totalBytes() - fsUsed;
  String jsonStr;
  serializeJson(doc, jsonStr);
  server.send(200, "application/json", jsonStr);
}