#include <Arduino.h>
#include "WebServerManager.h"

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Controle de Ventilador</title>
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
  <link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css" rel="stylesheet">
  <style>
    body {
      background-color: #f8f9fa;
      padding: 20px;
      transition: all 0.3s;
    }
    .container {
      max-width: 600px;
      margin: 0 auto;
      text-align: center;
    }
    h1 {
      color: #4CAF50;
      margin-bottom: 20px;
    }
    .btn-custom {
      margin: 5px;
      padding: 10px 20px;
      font-size: 1.2em;
    }
    .btn-custom i {
      margin-right: 10px;
    }
    #countdown {
      font-size: 1.5em;
      margin-top: 20px;
      color: #333;
    }
    .dark-theme {
      background-color: #212529;
      color: #f8f9fa;
    }
    .dark-theme h1 {
      color: #4CAF50;
    }
    .dark-theme .card {
      background-color: #343a40;
      color: #f8f9fa;
    }
    .dark-theme .form-control {
      background-color: #495057;
      color: #f8f9fa;
      border-color: #6c757d;
    }
    .dark-theme #countdown {
      color: #f8f9fa;
    }
    .time-inputs {
      display: flex;
      justify-content: center;
      gap: 15px;
      margin-bottom: 20px;
    }
    .time-inputs input {
      width: 80px;
      text-align: center;
    }
    #portalStatus {
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      padding: 10px;
      background-color: #ffc107;
      color: #212529;
      text-align: center;
      z-index: 1000;
      font-weight: bold;
    }
    .theme-switch {
      position: fixed;
      bottom: 20px;
      right: 20px;
      z-index: 1000;
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="card p-4">
      <h1><i class="fas fa-fan"></i> Controle de Ventilador</h1>
      <p class="lead">Horário atual: <span id="currentTime" class="fw-bold">Carregando...</span></p>
      
      <div class="time-inputs">
        <div class="mb-3">
          <label for="hours" class="form-label">Horas</label>
          <input type="number" id="hours" class="form-control" min="0" max="24" value="0">
        </div>
        <div class="mb-3">
          <label for="minutes" class="form-label">Minutos</label>
          <input type="number" id="minutes" class="form-control" min="0" max="59" value="5">
        </div>
      </div>
      
      <div class="d-flex justify-content-center flex-wrap">
        <button class="btn btn-success btn-custom" onclick="startRelay()">
          <i class="fas fa-power-off"></i> Ligar
        </button>
        <button class="btn btn-danger btn-custom" onclick="stopRelay()">
          <i class="fas fa-stop"></i> Desligar
        </button>
        <button class="btn btn-info btn-custom" onclick="configureWiFi()">
          <i class="fas fa-wifi"></i> WiFi
        </button>
      </div>
      
      <div id="countdown" class="my-3"></div>
      
      <div class="progress my-3">
        <div id="progressBar" class="progress-bar progress-bar-striped progress-bar-animated" role="progressbar" style="width: 0%"></div>
      </div>
      
      <div id="status" class="alert alert-info mt-3">
        <i class="fas fa-info-circle"></i> Ventilador Desligado
      </div>
    </div>
  </div>

  <button class="btn btn-secondary theme-switch rounded-circle" onclick="toggleTheme()">
    <i class="fas fa-moon"></i>
  </button>

  <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js"></script>
  <script>
    let countdownInterval;
    let isDarkTheme = false;
    let remainingSeconds = 0;

    // Atualiza o horário atual
    function updateTime() {
      fetch('/time')
        .then(response => response.text())
        .then(time => {
          document.getElementById('currentTime').textContent = time;
        });
      setTimeout(updateTime, 1000);
    }

    // Inicia o temporizador
    function startRelay() {
      const hours = parseInt(document.getElementById('hours').value) || 0;
      const minutes = parseInt(document.getElementById('minutes').value) || 5;
      const totalMinutes = hours * 60 + minutes;

      if (totalMinutes <= 0) {
        updateStatus("Por favor, insira um tempo válido", "danger");
        return;
      }

      fetch(`/start?duration=${totalMinutes}`)
        .then(response => response.text())
        .then(message => {
          updateStatus("Ventilador Ligado", "success");
          startCountdown(totalMinutes * 60);
        })
        .catch(error => {
          updateStatus("Erro ao ligar ventilador", "danger");
        });
    }

    // Para o temporizador
    function stopRelay() {
      fetch('/stop')
        .then(response => response.text())
        .then(message => {
          updateStatus("Ventilador Desligado", "warning");
          clearInterval(countdownInterval);
          document.getElementById('countdown').textContent = '';
          document.getElementById('progressBar').style.width = "0%";
          document.getElementById('progressBar').classList.remove('progress-bar-animated');
        });
    }

    // Configura WiFi
    function configureWiFi() {
      const statusElement = document.createElement('div');
      statusElement.id = 'portalStatus';
      statusElement.innerHTML = '<i class="fas fa-cog fa-spin me-2"></i> Preparando portal de configuração...';
      document.body.prepend(statusElement);

      fetch('/wificonfig')
        .then(response => response.text())
        .then(message => {
          let countdown = 10;
          const countdownInterval = setInterval(() => {
            statusElement.innerHTML = `<i class="fas fa-wifi me-2"></i> ${message} Recarregando em ${countdown--}s...`;
            if (countdown < 0) {
              clearInterval(countdownInterval);
              window.location.reload();
            }
          }, 1000);
        });
    }

    // Inicia a contagem regressiva
    function startCountdown(seconds) {
      clearInterval(countdownInterval);
      remainingSeconds = seconds;
      const totalSeconds = seconds;
      
      document.getElementById('progressBar').classList.add('progress-bar-animated');
      
      countdownInterval = setInterval(() => {
        if (remainingSeconds <= 0) {
          clearInterval(countdownInterval);
          document.getElementById('countdown').textContent = '';
          document.getElementById('progressBar').style.width = "0%";
          document.getElementById('progressBar').classList.remove('progress-bar-animated');
          updateStatus("Ventilador Desligado", "info");
        } else {
          const hours = Math.floor(remainingSeconds / 3600);
          const minutes = Math.floor((remainingSeconds % 3600) / 60);
          const secs = remainingSeconds % 60;
          
          let countdownText = '';
          if (hours > 0) countdownText += `${hours}h `;
          if (minutes > 0 || hours > 0) countdownText += `${minutes}m `;
          countdownText += `${secs}s`;
          
          document.getElementById('countdown').textContent = `Desligando em: ${countdownText}`;
          document.getElementById('progressBar').style.width = `${100 - (remainingSeconds / totalSeconds * 100)}%`;
          remainingSeconds--;
        }
      }, 1000);
    }

    // Atualiza o status
    function updateStatus(message, type) {
      const statusElement = document.getElementById('status');
      statusElement.textContent = message;
      statusElement.className = `alert alert-${type} mt-3`;
      
      // Adiciona ícone conforme o tipo
      let icon;
      switch(type) {
        case 'success': icon = 'fa-check-circle'; break;
        case 'danger': icon = 'fa-exclamation-circle'; break;
        case 'warning': icon = 'fa-stop-circle'; break;
        default: icon = 'fa-info-circle';
      }
      statusElement.innerHTML = `<i class="fas ${icon} me-2"></i> ${message}`;
    }

    // Alterna entre tema claro e escuro
    function toggleTheme() {
      isDarkTheme = !isDarkTheme;
      document.body.classList.toggle('dark-theme', isDarkTheme);
      const icon = document.querySelector('.theme-switch i');
      icon.classList.toggle('fa-moon', !isDarkTheme);
      icon.classList.toggle('fa-sun', isDarkTheme);
    }

    // Verifica status periodicamente
    function checkStatus() {
  fetch('/status')
    .then(response => response.text())
    .then(status => {
      if (status.includes("Ligado")) {
        fetch('/remaining')
          .then(r => r.json())
          .then(data => {
            if (data.remaining > 0 && !countdownInterval) {
              startCountdown(data.remaining);
            }
          });
      }
      updateStatusUI(status);
    });
}

function updateStatusUI(status) {
  const statusElement = document.getElementById('status');
  if (status.includes("Ligado")) {
    statusElement.innerHTML = '<i class="fas fa-check-circle me-2"></i> Ventilador Ligado';
    statusElement.className = "alert alert-success mt-3";
  } else if (status.includes("Desligado")) {
    statusElement.innerHTML = '<i class="fas fa-info-circle me-2"></i> Ventilador Desligado';
    statusElement.className = "alert alert-info mt-3";
  }
}

    // Inicialização
    document.addEventListener('DOMContentLoaded', () => {
      updateTime();
      setInterval(checkStatus, 5000);
      
      // Verifica se já está no tema escuro (salvo no localStorage)
      if (localStorage.getItem('darkTheme') === 'true') {
        toggleTheme();
      }
    });

    // Salva preferência do tema
    window.addEventListener('beforeunload', () => {
      localStorage.setItem('darkTheme', isDarkTheme);
    });
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
  server.on("/remaining", HTTP_GET, [this]() { handleRemaining(); });
  
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