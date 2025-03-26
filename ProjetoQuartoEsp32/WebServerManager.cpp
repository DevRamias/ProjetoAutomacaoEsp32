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
    /* Estilos aqui */
  </style>
</head>
<body>
  <div class="container">
    <h1>Controle de Ventilador</h1>
    <p>Horário atual: <span id="currentTime">Carregando...</span></p>
    <div class="time-inputs">
      <div>
        <label for="hours" class="form-label">Horas:</label>
        <input type="number" id="hours" class="form-control" min="0" max="24" value="0">
      </div>
      <div>
        <label for="minutes" class="form-label">Minutos:</label>
        <input type="number" id="minutes" class="form-control" min="0" max="59" value="5">
      </div>
    </div>
    <button class="btn btn-success btn-custom" onclick="startRelay()">
      <i class="fas fa-power-off"></i> Ligar Ventilador
    </button>
    <button class="btn btn-danger btn-custom" onclick="stopRelay()">
      <i class="fas fa-stop"></i> Desligar Ventilador
    </button>
    <div id="countdown"></div>
    <div class="progress">
      <div id="progressBar" class="progress-bar" role="progressbar" style="width: 0%"></div>
    </div>
    <div id="status" class="alert alert-info mt-3">Ventilador Desligado</div>
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

// Iniciar a atualização do tempo quando a página carrega
window.onload = function() {
  updateTime();
  
  // Verificar status do ventilador periodicamente
  setInterval(() => {
    fetch('/status') // Você precisará implementar esta rota
      .then(response => response.text())
      .then(status => {
        document.getElementById('status').textContent = status;
      });
  }, 1000);
};
  </script>
</body>
</html>
)rawliteral";

void WebServerManager::begin(RelayManager* relayManager, NTPManager* ntpManager) {
  this->relayManager = relayManager;
  this->ntpManager = ntpManager;

  server.on("/", std::bind(&WebServerManager::handleRoot, this));
  server.on("/start", std::bind(&WebServerManager::handleStart, this));
  server.on("/stop", std::bind(&WebServerManager::handleStop, this));
  server.on("/time", std::bind(&WebServerManager::handleTime, this));
  server.begin();
  Serial.println("Servidor web iniciado!");
}

void WebServerManager::handleClient() {
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
  }
}

void WebServerManager::handleStop() {
  relayManager->stop();
  server.send(200, "text/plain", "Ventilador desligado manualmente!");
}

void WebServerManager::handleTime() {
  server.send(200, "text/plain", ntpManager->getFormattedTime());
}