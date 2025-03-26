#include <Arduino.h>
#include "WiFiManager.h"
#include <WebServer.h>  // Adicione esta linha
#include <WiFiClient.h>

void WiFiManager::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  
  if(WiFi.status() != WL_CONNECTED) {
    startConfigurationPortal();
  }
}

void WiFiManager::connect(const char* ssid, const char* password) {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado ao Wi-Fi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

bool WiFiManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::startConfigurationPortal() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32-Config");
  
  server = new WebServer(80);  // Cria uma nova instância do WebServer
  
  server->on("/", HTTP_GET, [this]() {
    String html = "<form action='/config' method='post'>";
    html += "SSID: <input type='text' name='ssid'><br>";
    html += "Senha: <input type='password' name='password'><br>";
    html += "<input type='submit' value='Conectar'>";
    html += "</form>";
    server->send(200, "text/html", html);
  });
  
  server->on("/config", HTTP_POST, [this]() {
    String ssid = server->arg("ssid");
    String password = server->arg("password");
    
    // Aqui você deve implementar a lógica para salvar as credenciais
    // Por exemplo, usando Preferences ou EEPROM
    
    server->send(200, "text/html", "Configuração salva! Reiniciando...");
    delay(1000);
    ESP.restart();
  });
  
  server->begin();
  
  while(true) {
    server->handleClient();
    delay(10);
  }
  
  delete server;  // Libera a memória quando não for mais necessário
}