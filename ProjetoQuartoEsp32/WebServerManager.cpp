#include <Arduino.h>
#include "WebServerManager.h"
#include <ArduinoJson.h>

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

    <!-- ===== NOVO CARD AUTOMÁTICO ===== -->
<div class="card p-4 mt-3" style="border-left: 4px solid #4CAF50;">
  <h3><i class="fas fa-robot"></i> Controle Automático</h3>
  
  <div class="row g-3 mb-3">
    <!-- HORÁRIO DE FUNCIONAMENTO -->
    <div class="col-md-6">
      <label class="form-label">Das</label>
      <input type="time" class="form-control" id="autoStart" value="14:00">
    </div>
    <div class="col-md-6">
      <label class="form-label">Até</label>
      <input type="time" class="form-control" id="autoEnd" value="22:00">
    </div>
    
    <!-- LIMIAR DE TEMPERATURA + INTERVALO DE VERIFICAÇÃO -->
    <div class="col-md-6">
      <label class="form-label">Ligar acima de</label>
      <div class="input-group">
        <input type="number" class="form-control" id="autoTemp" value="28" step="0.1">
        <span class="input-group-text">°C</span>
      </div>
    </div>
    <div class="col-md-6">
      <label class="form-label">Verificar a cada</label>
      <div class="input-group">
        <input type="number" class="form-control" id="autoCheckInterval" value="10" min="1">
        <span class="input-group-text">min</span>
      </div>
    </div>
  </div>
  
  <!-- BOTÃO DE ATIVAÇÃO -->
  <button onclick="saveAutoSettings()" class="btn btn-success w-100">
    <i class="fas fa-power-off me-2"></i> Ativar Automático
  </button>
  
  <!-- STATUS -->
  <div id="autoStatus" class="alert alert-warning mt-3 mb-0">
    <i class="fas fa-info-circle me-2"></i> Aguardando ativação...
  </div>
</div>

    <div class="access-info alert alert-secondary mt-4">
      <p><i class="fas fa-link"></i> <strong>Acesso permanente:</strong></p>
      <input type="text" class="form-control" value="http://esp32.local" readonly
             onclick="this.select()" style="text-align: center;">
      <small class="text-muted">Use este endereço em qualquer dispositivo na sua rede</small>
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
    // ===== FUNÇÃO PARA SALVAR AS CONFIGURAÇÕES AUTOMÁTICAS =====
function saveAutoSettings() {
  // Pega os valores dos inputs
  const autoStart = document.getElementById('autoStart').value;
  const autoEnd = document.getElementById('autoEnd').value;
  const autoTemp = parseFloat(document.getElementById('autoTemp').value);
  const autoCheckInterval = parseInt(document.getElementById('autoCheckInterval').value);

  // Validações básicas
  if (!autoStart || !autoEnd || isNaN(autoTemp) {
    alert("Preencha todos os campos corretamente!");
    return;
  }

  // Monta o objeto de configuração
  const settings = {
    startTime: autoStart,
    endTime: autoEnd,
    minTemp: autoTemp,
    checkInterval: autoCheckInterval
  };

  // Envia para o ESP32 via POST
  fetch('/set-auto', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(settings)
  })
  .then(response => response.json())
  .then(data => {
    if (data.success) {
      document.getElementById('autoStatus').innerHTML = `
        <i class="fas fa-check-circle me-2"></i> Automático ativo: ${autoStart} às ${autoEnd} | > ${autoTemp}°C
      `;
      document.getElementById('autoStatus').className = 'alert alert-success mt-3 mb-0';
    } else {
      alert("Erro ao ativar: " + data.error);
    }
  })
  .catch(error => {
    alert("Falha na conexão com o ESP32");
  });
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
  this->dhtManager = dhtManager;  // Novo: inicializa o DHTManager

  // Configura rotas
  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/start", HTTP_GET, [this]() { handleStart(); });
  server.on("/stop", HTTP_GET, [this]() { handleStop(); });
  server.on("/time", HTTP_GET, [this]() { handleTime(); });
  server.on("/status", HTTP_GET, [this]() { handleStatus(); });
  server.on("/wificonfig", HTTP_GET, [this]() { handleWiFiConfig(); });
  server.on("/remaining", HTTP_GET, [this]() { handleRemaining(); });

  // Nova rota para controle automático
  server.on("/set-auto", HTTP_POST, [this]() {
    if (server.hasArg("plain")) {
      DynamicJsonDocument doc(256);
      DeserializationError error = deserializeJson(doc, server.arg("plain"));
      
      if (error) {
        server.send(400, "text/plain", "JSON inválido");
        return;
      }

      // Salva as configurações
      this->autoStartTime = doc["startTime"].as<String>();
      this->autoEndTime = doc["endTime"].as<String>();
      this->autoMinTemp = doc["minTemp"];
      this->autoCheckIntervalMinutes = doc["checkInterval"];
      this->autoModeActive = true;  // Ativa o modo automático

      // Confirmação
      DynamicJsonDocument response(128);
      response["success"] = true;
      String responseStr;
      serializeJson(response, responseStr);
      server.send(200, "application/json", responseStr);
    } else {
      server.send(400, "text/plain", "Dados faltando");
    }
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

void WebServerManager::verificarCondicoesAutomaticas() {
    String horaAtual = ntpManager->getFormattedTime().substring(0, 5); // "HH:MM"
    
    if (horaAtual >= autoStartTime && horaAtual <= autoEndTime) {
        float temperatura = dhtManager->readTemperature();  // Lê do DHTManager
        float umidade = dhtManager->readHumidity();
        
        if (!isnan(temperatura) {  // Verifica se a leitura é válida
            float sensacaoTermica = temperatura + (umidade * 0.1);  // Fórmula simplificada
            
            if (sensacaoTermica >= autoMinTemp && !relayManager->isActive()) {
                relayManager->start(30);  // Liga por 30 minutos
            }
        }
    }
}