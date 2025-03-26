#include <WiFi.h>
#include <WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Configurações de Wi-Fi
const char* ssid = "*****";
const char* password = "*****";

// Configurações do NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000); // Fuso horário -3 (Brasília)

// Pino do relé
const int relayPin = 5;

// Variáveis de controle
unsigned long relayStartTime = 0;
unsigned long relayDuration = 0; // Duração em milissegundos
bool relayActive = false;

// Servidor Web na porta 80
WebServer server(80);

// Página HTML
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
    .dark-theme #countdown {
      color: #fff; /* Cor da fonte no tema escuro */
    }
    #status {
      font-size: 1.2em;
      margin-top: 10px;
    }
    .progress {
      height: 20px;
      margin-top: 20px;
    }
    .theme-switch {
      position: fixed;
      bottom: 20px;
      right: 20px;
    }
    .dark-theme {
      background-color: #333;
      color: #fff;
    }
    .dark-theme h1 {
      color: #4CAF50;
    }
    .dark-theme .btn-custom {
      background-color: #555;
      border-color: #555;
    }
    .time-inputs {
      display: flex;
      justify-content: center;
      gap: 10px;
      margin-bottom: 20px;
    }
    .time-inputs input {
      width: 80px;
      text-align: center;
    }
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

  <!-- Botão para alternar tema -->
  <button class="btn btn-secondary theme-switch" onclick="toggleTheme()">
    <i class="fas fa-moon"></i>
  </button>

  <script>
    let countdownInterval;
    let isDarkTheme = false;

    // Atualiza o horário atual
    function updateTime() {
      fetch('/time')
        .then(response => response.text())
        .then(time => {
          document.getElementById('currentTime').innerText = time;
        });
    }

    // Inicia o temporizador
    function startRelay() {
      const hours = parseInt(document.getElementById('hours').value);
      const minutes = parseInt(document.getElementById('minutes').value);
      const totalMinutes = hours * 60 + minutes;

      if (totalMinutes <= 0) {
        document.getElementById('status').innerText = "Por favor, insira um tempo válido.";
        document.getElementById('status').className = "alert alert-danger mt-3";
        return;
      }

      fetch(`/start?duration=${totalMinutes}`)
        .then(response => response.text())
        .then(message => {
          document.getElementById('status').innerText = "Ventilador Ligado";
          document.getElementById('status').className = "alert alert-success mt-3";
          startCountdown(totalMinutes * 60);
        });
    }

    // Para o temporizador
    function stopRelay() {
      fetch('/stop')
        .then(response => response.text())
        .then(message => {
          document.getElementById('status').innerText = "Ventilador Desligado";
          document.getElementById('status').className = "alert alert-warning mt-3";
          clearInterval(countdownInterval);
          document.getElementById('countdown').innerText = '';
          document.getElementById('progressBar').style.width = "0%";
        });
    }

    // Inicia a contagem regressiva
    function startCountdown(seconds) {
      clearInterval(countdownInterval);
      let remaining = seconds;
      const totalSeconds = seconds;
      countdownInterval = setInterval(() => {
        if (remaining <= 0) {
          clearInterval(countdownInterval);
          document.getElementById('countdown').innerText = '';
          document.getElementById('progressBar').style.width = "0%";
          document.getElementById('status').innerText = "Ventilador Desligado";
          document.getElementById('status').className = "alert alert-info mt-3";
        } else {
          const minutes = Math.floor(remaining / 60);
          const secs = remaining % 60;
          // Atualiza a cada 30 segundos
          if (secs % 30 === 0) {
            document.getElementById('countdown').innerText = `Desligando em: ${minutes}:${secs < 10 ? '0' : ''}${secs}`;
          }
          // Atualiza a barra de progresso
          document.getElementById('progressBar').style.width = `${((totalSeconds - remaining) / totalSeconds) * 100}%`;
          remaining--;
        }
      }, 1000);
    }

    // Alterna entre tema claro e escuro
    function toggleTheme() {
      isDarkTheme = !isDarkTheme;
      document.body.classList.toggle('dark-theme', isDarkTheme);
    }

    // Atualiza o horário periodicamente
    setInterval(updateTime, 1000);
  </script>
</body>
</html>
)rawliteral";

// Função para iniciar o temporizador
void handleStart() {
  if (server.hasArg("duration")) {
    relayDuration = server.arg("duration").toInt() * 60 * 1000; // Converte minutos para milissegundos
    relayStartTime = millis();
    relayActive = true;
    digitalWrite(relayPin, HIGH); // Liga o relé
    server.send(200, "text/plain", "Ventilador ligado por " + String(relayDuration / 60000) + " minutos!");
  }
}

// Função para parar o temporizador
void handleStop() {
  relayActive = false;
  digitalWrite(relayPin, LOW); // Desliga o relé
  server.send(200, "text/plain", "Ventilador desligado manualmente!");
}

// Função para verificar o status do temporizador
void handleStatus() {
  unsigned long remainingTime = relayActive ? (relayStartTime + relayDuration - millis()) : 0;
  String status = "{\"active\":" + String(relayActive ? "true" : "false") + ",\"endTime\":" + String(relayStartTime + relayDuration) + "}";
  server.send(200, "application/json", status);
}

// Função para exibir o horário atual
void handleTime() {
  timeClient.update();
  server.send(200, "text/plain", timeClient.getFormattedTime());
}

// Função para servir a página HTML
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Inicializa o relé desligado

  // Conecta ao Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado ao Wi-Fi!");

  // Inicializa o NTPClient
  timeClient.begin();

  // Inicia o servidor
  server.on("/", handleRoot);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.on("/status", handleStatus);
  server.on("/time", handleTime);
  server.begin();
  Serial.println("Servidor iniciado!");

  // Exibe o IP no Serial Monitor
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient(); // Gerencia as requisições do servidor
  timeClient.update();   // Atualiza o horário

  // Verifica se o temporizador acabou
  if (relayActive && millis() - relayStartTime >= relayDuration) {
    relayActive = false;
    digitalWrite(relayPin, LOW); // Desliga o relé
    Serial.println("Ventilador desligado automaticamente!");
  }
}