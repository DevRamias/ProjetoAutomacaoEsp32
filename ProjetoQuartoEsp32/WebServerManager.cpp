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
    /* Mantenha todo o CSS original aqui */
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

      <!-- Card de Dados do Sensor -->
      <div class="card p-4 mt-3">
        <h3><i class="fas fa-thermometer-half"></i> Dados do Ambiente</h3>
        <div class="row text-center">
          <div class="col-md-4">
            <p class="mb-1"><strong>Temperatura</strong></p>
            <h2 id="currentTemp">--</h2>
          </div>
          <div class="col-md-4">
            <p class="mb-1"><strong>Umidade</strong></p>
            <h2 id="currentHumidity">--</h2>
          </div>
          <div class="col-md-4">
            <p class="mb-1"><strong>Sensação Térmica</strong></p>
            <h2 id="currentFeelsLike">--</h2>
          </div>
        </div>
      </div>

      <!-- Controle Manual -->
      <div class="card p-4 mt-3">
        <h3><i class="fas fa-cog"></i> Controle Manual</h3>
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
      </div>

      <!-- Controle Automático -->
      <div class="card p-4 mt-3">
  <h3><i class="fas fa-robot"></i> Controle Automático</h3>
  
  <!-- Toggle ON/OFF -->
  <div class="form-switch mb-3">
    <input class="form-check-input" type="checkbox" id="autoModeToggle">
    <label class="form-check-label" for="autoModeToggle">Modo Automático Ativo</label>
  </div>

  <!-- Configurações (mostrar só quando ativo) -->
  <div id="autoSettings" style="display:none;">
    <div class="row g-3">
      <div class="col-md-6">
        <label class="form-label">Ligar acima de</label>
        <div class="input-group">
          <input type="number" class="form-control" id="autoTemp" value="28" step="0.1">
          <span class="input-group-text">°C</span>
        </div>
      </div>
      <div class="col-md-6">
        <label class="form-label">Tempo ligado</label>
        <div class="input-group">
          <input type="number" class="form-control" id="ventilationTime" value="30" min="1">
          <span class="input-group-text">min</span>
        </div>
      </div>
      <div class="col-md-12">
        <label class="form-label">Standby entre ciclos</label>
        <div class="input-group">
          <input type="number" class="form-control" id="standbyTime" value="10" min="2">
          <span class="input-group-text">min</span>
        </div>
      </div>
    </div>
  </div>
</div>

      <!-- Status e Controles -->
      <div id="countdown" class="my-3"></div>
      <div class="progress my-3">
        <div id="progressBar" class="progress-bar progress-bar-striped progress-bar-animated" role="progressbar" style="width: 0%"></div>
      </div>
      <div id="status" class="alert alert-info mt-3">
        <i class="fas fa-info-circle"></i> Ventilador Desligado
      </div>

      <!-- Acesso Permanente -->
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
  </div>

  <!-- Mantenha todo o JavaScript original aqui -->
  <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js"></script>
<script>
    // Constantes e variáveis globais
    const ELEMENTS = {
        autoModeToggle: document.getElementById('autoModeToggle'),
        autoSettings: document.getElementById('autoSettings'),
        currentTime: document.getElementById('currentTime'),
        currentTemp: document.getElementById('currentTemp'),
        currentHumidity: document.getElementById('currentHumidity'),
        currentFeelsLike: document.getElementById('currentFeelsLike'),
        hours: document.getElementById('hours'),
        minutes: document.getElementById('minutes'),
        countdown: document.getElementById('countdown'),
        progressBar: document.getElementById('progressBar'),
        status: document.getElementById('status'),
        autoTemp: document.getElementById('autoTemp'),
        ventilationTime: document.getElementById('ventilationTime'),
        standbyTime: document.getElementById('standbyTime'),
        themeSwitch: document.querySelector('.theme-switch i')
    };

    const STATE = {
        countdownInterval: null,
        isDarkTheme: localStorage.getItem('darkTheme') === 'true',
        remainingSeconds: 0
    };

    // Funções utilitárias
    const fetchData = async (url, options = {}) => {
        try {
            const response = await fetch(url, options);
            if (!response.ok) throw new Error(response.statusText);
            return options.parseText ? await response.text() : await response.json();
        } catch (error) {
            console.error('Fetch error:', error);
            throw error;
        }
    };

    const updateElement = (element, content, className = null) => {
        if (!element) return;
        if (content !== undefined && element.textContent !== content) {
            element.textContent = content;
        }
        if (className && element.className !== className) {
            element.className = className;
        }
    };

    // Funções principais
    const updateTime = () => {
        fetchData('/time', { parseText: true })
            .then(time => updateElement(ELEMENTS.currentTime, time))
            .finally(() => setTimeout(updateTime, 1000));
    };

    const controlRelay = async (action, duration = null) => {
        try {
            const url = action === 'start' ? `/start?duration=${duration}` : '/stop';
            const message = await fetchData(url, { parseText: true });
            
            updateStatus(action === 'start' ? "Ventilador Ligado" : "Ventilador Desligado", 
                        action === 'start' ? "success" : "warning");
            
            if (action === 'start') {
                startCountdown(duration * 60);
            } else {
                clearCountdown();
            }
        } catch (error) {
            updateStatus(`Erro ao ${action === 'start' ? 'ligar' : 'desligar'} ventilador`, "danger");
        }
    };

    const startCountdown = (seconds) => {
        clearInterval(STATE.countdownInterval);
        STATE.remainingSeconds = seconds;
        
        if (ELEMENTS.progressBar) {
            ELEMENTS.progressBar.classList.add('progress-bar-animated');
        }
        
        STATE.countdownInterval = setInterval(() => {
            if (STATE.remainingSeconds <= 0) {
                clearCountdown();
                updateStatus("Ventilador Desligado", "info");
                return;
            }

            const hours = Math.floor(STATE.remainingSeconds / 3600);
            const minutes = Math.floor((STATE.remainingSeconds % 3600) / 60);
            const secs = STATE.remainingSeconds % 60;
            
            let countdownText = '';
            if (hours > 0) countdownText += `${hours}h `;
            if (minutes > 0 || hours > 0) countdownText += `${minutes}m `;
            countdownText += `${secs}s`;
            
            updateElement(ELEMENTS.countdown, `Desligando em: ${countdownText}`);
            
            if (ELEMENTS.progressBar) {
                ELEMENTS.progressBar.style.width = `${100 - (STATE.remainingSeconds / seconds * 100)}%`;
            }
            
            STATE.remainingSeconds--;
        }, 1000);
    };

    const clearCountdown = () => {
        clearInterval(STATE.countdownInterval);
        updateElement(ELEMENTS.countdown, '');
        if (ELEMENTS.progressBar) {
            ELEMENTS.progressBar.style.width = "0%";
            ELEMENTS.progressBar.classList.remove('progress-bar-animated');
        }
    };

    const updateStatus = (message, type) => {
        if (!ELEMENTS.status) return;
        
        const icons = {
            success: 'fa-check-circle',
            danger: 'fa-exclamation-circle',
            warning: 'fa-stop-circle',
            info: 'fa-info-circle'
        };
        
        ELEMENTS.status.innerHTML = `<i class="fas ${icons[type] || 'fa-info-circle'} me-2"></i> ${message}`;
        ELEMENTS.status.className = `alert alert-${type} mt-3`;
    };

    const toggleTheme = () => {
        STATE.isDarkTheme = !STATE.isDarkTheme;
        document.body.classList.toggle('dark-theme', STATE.isDarkTheme);
        
        if (ELEMENTS.themeSwitch) {
            ELEMENTS.themeSwitch.classList.toggle('fa-moon', !STATE.isDarkTheme);
            ELEMENTS.themeSwitch.classList.toggle('fa-sun', STATE.isDarkTheme);
        }
        
        localStorage.setItem('darkTheme', STATE.isDarkTheme);
    };

    const configureWiFi = () => {
        const statusElement = document.createElement('div');
        statusElement.id = 'portalStatus';
        statusElement.innerHTML = '<i class="fas fa-cog fa-spin me-2"></i> Preparando portal de configuração...';
        document.body.prepend(statusElement);

        fetchData('/wificonfig', { parseText: true })
            .then(message => {
                let countdown = 10;
                const interval = setInterval(() => {
                    statusElement.innerHTML = `<i class="fas fa-wifi me-2"></i> ${message} Recarregando em ${countdown--}s...`;
                    if (countdown < 0) {
                        clearInterval(interval);
                        window.location.reload();
                    }
                }, 1000);
            });
    };

    // Controle Automático
    const setupAutoMode = () => {
        if (!ELEMENTS.autoModeToggle || !ELEMENTS.autoSettings) return;

        const sendAutoSettings = () => {
            const settings = {
                active: ELEMENTS.autoModeToggle.checked,
                temp: parseFloat(ELEMENTS.autoTemp.value) || 28,
                ventTime: parseInt(ELEMENTS.ventilationTime.value) || 30,
                standby: parseInt(ELEMENTS.standbyTime.value) || 10
            };

            fetchData('/set-auto-settings', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(settings)
            }).catch(() => {
                ELEMENTS.autoModeToggle.checked = !ELEMENTS.autoModeToggle.checked;
            });
        };

        ELEMENTS.autoModeToggle.addEventListener('change', () => {
            ELEMENTS.autoSettings.style.display = ELEMENTS.autoModeToggle.checked ? 'block' : 'none';
            sendAutoSettings();
        });

        // Carrega configurações iniciais
        fetchData('/get-auto-settings')
            .then(settings => {
                if (settings.active) {
                    ELEMENTS.autoModeToggle.checked = true;
                    ELEMENTS.autoSettings.style.display = 'block';
                    ELEMENTS.autoTemp.value = settings.temp || 28;
                    ELEMENTS.ventilationTime.value = settings.ventTime || 30;
                    ELEMENTS.standbyTime.value = settings.standby || 10;
                }
            })
            .catch(console.error);
    };

    // Atualização de dados do sensor
    const updateSensorData = () => {
        fetchData('/sensor-data')
            .then(data => {
                if (!data.error) {
                    updateElement(ELEMENTS.currentTemp, `${data.temp?.toFixed(1) || '--'}°C`);
                    updateElement(ELEMENTS.currentHumidity, `${data.humidity?.toFixed(1) || '--'}%`);
                    updateElement(ELEMENTS.currentFeelsLike, `${data.feelsLike?.toFixed(1) || '--'}°C`);
                }
            })
            .catch(console.error);
    };

    const updateStatusUI = (status) => {
        if (!ELEMENTS.status) return;
        
        if (status.includes("Ligado")) {
            ELEMENTS.status.innerHTML = '<i class="fas fa-check-circle me-2"></i> Ventilador Ligado';
            ELEMENTS.status.className = "alert alert-success mt-3";
        } else if (status.includes("Desligado")) {
            ELEMENTS.status.innerHTML = '<i class="fas fa-info-circle me-2"></i> Ventilador Desligado';
            ELEMENTS.status.className = "alert alert-info mt-3";
        }
    };

    // Inicialização
    document.addEventListener('DOMContentLoaded', () => {
        // Configura tema
        if (STATE.isDarkTheme) {
            document.body.classList.add('dark-theme');
            if (ELEMENTS.themeSwitch) {
                ELEMENTS.themeSwitch.classList.replace('fa-moon', 'fa-sun');
            }
        }

        // Configura eventos
        document.querySelector('.theme-switch')?.addEventListener('click', toggleTheme);
        
        // Inicia serviços
        updateTime();
        updateSensorData();
        setInterval(updateSensorData, 60000);
        setInterval(() => {
            fetchData('/status', { parseText: true })
                .then(updateStatusUI)
                .catch(console.error);
        }, 5000);
        
        // Configura modo automático
        setupAutoMode();
    });

    // Exporta funções para chamada via HTML
    window.startRelay = () => {
        const hours = parseInt(ELEMENTS.hours?.value) || 0;
        const minutes = parseInt(ELEMENTS.minutes?.value) || 5;
        const totalMinutes = hours * 60 + minutes;
        
        if (totalMinutes <= 0) {
            updateStatus("Por favor, insira um tempo válido", "danger");
            return;
        }
        
        controlRelay('start', totalMinutes);
    };

    window.stopRelay = () => controlRelay('stop');
    window.configureWiFi = configureWiFi;
    window.toggleTheme = toggleTheme;
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
      doc["temp"] = temp;
      doc["humidity"] = humidity;
      doc["feelsLike"] = temp + (humidity * 0.1f); // Sensação térmica
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