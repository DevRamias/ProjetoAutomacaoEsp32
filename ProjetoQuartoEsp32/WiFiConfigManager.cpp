#include "WiFiConfigManager.h"

void WiFiConfigManager::begin() {
  if (!SPIFFS.begin(true)) {
    Serial.println("Erro ao inicializar o SPIFFS!");
    return;
  }

  if (!connectToSavedNetwork()) {
    startAP(); // Inicia o Access Point
  }

  server.on("/", std::bind(&WiFiConfigManager::handleRoot, this));
  server.on("/scan", std::bind(&WiFiConfigManager::handleScan, this));
  server.on("/connect", std::bind(&WiFiConfigManager::handleConnect, this));
  server.on("/delete", std::bind(&WiFiConfigManager::handleDelete, this));
  server.begin();
}

void WiFiConfigManager::handleClient() {
  server.handleClient();
}

bool WiFiConfigManager::isConnected() {
  return connected;
}

bool WiFiConfigManager::connectToSavedNetwork() {
  File file = SPIFFS.open("/wifi.txt", "r");
  if (!file) {
    Serial.println("Nenhuma rede salva.");
    return false;
  }

  String ssid = file.readStringUntil('\n');
  String password = file.readStringUntil('\n');
  file.close();

  ssid.trim();
  password.trim();

  if (ssid.length() == 0 || password.length() == 0) {
    Serial.println("Credenciais invalidas no arquivo wifi.txt. Apagando...");
    SPIFFS.remove("/wifi.txt");
    return false;
  }

  Serial.println("Conectando a rede salva: " + ssid);
  WiFi.begin(ssid.c_str(), password.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    connected = true;
    return true;
  } else {
    Serial.println("\nFalha ao conectar a rede salva.");
    connected = false;
    return false;
  }
}

void WiFiConfigManager::startAP() {
  connected = false;
  WiFi.softAP("ESP32_Config", "12345678");
  Serial.println("Access Point iniciado!");
  Serial.print("IP do AP: ");
  Serial.println(WiFi.softAPIP());
}

void WiFiConfigManager::saveNetwork(String ssid, String password) {
  File file = SPIFFS.open("/wifi.txt", "w");
  if (!file) {
    Serial.println("Erro ao salvar rede!");
    return;
  }
  file.println(ssid);
  file.println(password);
  file.close();
}

void WiFiConfigManager::deleteSavedNetwork() {
  SPIFFS.remove("/wifi.txt");
  Serial.println("Rede salva apagada!");
}

void WiFiConfigManager::handleRoot() {
  String html = "<html><body>";
  html += "<h1>Configuracao Wi-Fi</h1>";
  html += "<p><a href='/scan'>Ver redes disponiveis</a></p>";
  html += "<p><a href='/delete'>Apagar rede salva</a></p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void WiFiConfigManager::handleScan() {
  String html = "<html><body>";
  html += "<h1>Redes Wi-Fi Disponiveis</h1>";
  html += "<ul>";

  int numNetworks = WiFi.scanNetworks();
  for (int i = 0; i < numNetworks; i++) {
    html += "<li>";
    html += WiFi.SSID(i);
    html += " <a href='/connect?ssid=" + WiFi.SSID(i) + "'>Conectar</a>";
    html += "</li>";
  }

  html += "</ul>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void WiFiConfigManager::handleConnect() {
  if (server.hasArg("ssid")) {
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    if (password == "") {
      String html = "<html><body>";
      html += "<h1>Conectar a rede: " + ssid + "</h1>";
      html += "<form action='/connect' method='get'>";
      html += "<input type='hidden' name='ssid' value='" + ssid + "'>";
      html += "Senha: <input type='password' name='password'>";
      html += "<input type='submit' value='Conectar'>";
      html += "</form>";
      html += "</body></html>";
      server.send(200, "text/html", html);
    } else {
      WiFi.begin(ssid.c_str(), password.c_str());
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
      }

      if (WiFi.status() == WL_CONNECTED) {
        saveNetwork(ssid, password);
        server.send(200, "text/plain", "Conectado! IP: " + WiFi.localIP());
        connected = true;
      } else {
        connected = false;
        server.send(200, "text/plain", "Falha ao conectar.");
      }
    }
  }
}

void WiFiConfigManager::handleDelete() {
  deleteSavedNetwork();
  server.send(200, "text/plain", "Rede salva apagada!");
}