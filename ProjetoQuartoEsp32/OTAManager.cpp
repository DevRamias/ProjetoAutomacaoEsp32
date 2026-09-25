// OTAManager.cpp
#include "OTAManager.h"
#include <WiFi.h>

OTAManager::OTAManager() {}

void OTAManager::begin(const char* hostname, const char* password) {
    if (hostname) {
        ArduinoOTA.setHostname(hostname);
    }

    if (password && strlen(password) > 0) {
        ArduinoOTA.setPassword(password);
        Serial.println("OTA com senha configurada.");
    } else {
        Serial.println("AVISO: OTA sem senha! Qualquer um na rede pode atualizar.");
    }

    configureOTAEvents();
    ArduinoOTA.begin();

    Serial.println("OTA Configurado");
    Serial.print("Hostname: ");
    Serial.println(hostname ? hostname : WiFi.getHostname());
}

void OTAManager::handle() {
    ArduinoOTA.handle();
}

void OTAManager::configureOTAEvents() {
    ArduinoOTA.onStart([]() {
        String type = ArduinoOTA.getCommand() == U_FLASH ? "firmware" : "filesystem";
        Serial.println("Iniciando atualização " + type);
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\nAtualização concluída!");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progresso: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Erro[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Autenticação falhou");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Falha ao iniciar");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Falha na conexão");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Falha na recepção");
        else if (error == OTA_END_ERROR) Serial.println("Falha ao finalizar");
    });
}