#include <Arduino.h>
#include "WebServerManager.h"

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Controle de Ventilador</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; }
    .container { max-width: 600px; margin: 0 auto; }
    .btn { padding: 10px 15px; margin: 5px; border: none; color: white; cursor: pointer; }
    .btn-success { background-color: #28a745; }
    .btn-danger { background-color: #dc3545; }
    .btn-info { background-color: #17a2b8; }
    .progress { height: 20px; background-color: #e9ecef; margin: 10px 0; }
    .progress-bar { height: 100%; background-color: #007bff; }
    #portalStatus {
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      padding: 10px;
      background-color: #ffcc00;
      text-align: center;
      z-index: 1000;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Controle de Ventilador</h1>
    <p>Horário atual: <span id="currentTime">Carregando...</span></p>
    <div class="time-inputs">
      <div>
        <label for="hours">Horas:</label>
        <input type="number" id="hours" min="0" max="24" value="0">
      </div>
      <div>
        <label for="minutes">Minutos:</label>
        <input type="number" id="minutes" min="0" max="59" value="5">
      </div>
    </div>
    <button class="btn btn-success" onclick="startRelay()">Ligar Ventilador</button>
    <button class="btn btn-danger" onclick="stopRelay()">Desligar Ventilador</button>
    <button class="btn btn-info" onclick="configureWiFi()">Configurar WiFi</button>
    <div id="countdown"></div>
    <div class="progress">
      <div id="progressBar" class="progress-bar" style="width: 0%"></div>
    </div>
    <div id="status">Ventilador Desligado</div>
  </div>

  <script>
    function updateTime() {
      fetch('/time')
        .then(response => response.text())
        .then(time => {
          document.getElementById('currentTime').textContent = time;
        });
      setTimeout(updateTime, 1000);
    }
    
    function startRelay() {
      const hours = parseInt(document.getElementById('hours').value) || 0;
      const minutes = parseInt(document.getElementById('minutes').value) || 5;
      const duration = hours * 60 + minutes;
      
      fetch('/start?duration=' + duration)
        .then(response => response.text())
        .then(message => alert(message));
    }
    
    function stopRelay() {
      fetch('/stop')
        .then(response => response.text())
        .then(message => alert(message));
    }
    
    function configureWiFi() {
      fetch('/wificonfig')
        .then(response => response.text())
        .then(message => {
          // Mostra o status do portal
          const statusElement = document.createElement('div');
          statusElement.id = 'portalStatus';
          statusElement.textContent = message;
          document.body.prepend(statusElement);
          
          // Atualiza a cada segundo
          let countdown = 10;
          const interval = setInterval(() => {
            statusElement.textContent = `${message} Recarregando em ${countdown--}s...`;
            if (countdown < 0) {
              clearInterval(interval);
              window.location.reload();
            }
          }, 1000);
        });
    }
    
    function checkStatus() {
      fetch('/status')
        .then(response => response.text())
        .then(status => {
          document.getElementById('status').textContent = status;
        });
    }
    
    // Inicia as atualizações
    window.onload = function() {
      updateTime();
      setInterval(checkStatus, 1000);
    };
  </script>
</body>
</html>
)rawliteral";

void WebServerManager::begin(RelayManager* relayManager, NTPManager* ntpManager, WiFiManager* wifiManager) {
  this->relayManager = relayManager;
  this->ntpManager = ntpManager;
  this->wifiManager = wifiManager;

  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/start", HTTP_GET, [this]() { handleStart(); });
  server.on("/stop", HTTP_GET, [this]() { handleStop(); });
  server.on("/time", HTTP_GET, [this]() { handleTime(); });
  server.on("/status", HTTP_GET, [this]() { handleStatus(); });
  server.on("/wificonfig", HTTP_GET, [this]() { handleWiFiConfig(); });
  
  server.begin();
  Serial.println("Servidor web iniciado!");
}

void WebServerManager::handleClient() {
  static bool portalRequested = false;
  static unsigned long portalStartTime = 0;
  
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
  server.send(200, "text/html", htmlPage);
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