// OTAManager.cpp
#include "OTAManager.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define OTA_CONFIG_FILE "/ota_config.json"

// Senha usada na primeira vez. Troque pelo painel (card "Configuracoes da placa")
// e atualize tambem o arquivo ota_senha.ini do PC.
const char* OTAManager::DEFAULT_PASSWORD = "senha";

OTAManager::OTAManager() {}

void OTAManager::begin(const char* hostname) {
    if (hostname) {
        ArduinoOTA.setHostname(hostname);
    }

    // O OTA SEMPRE tem senha: a salva ou, na primeira vez, a padrao
    _password = loadPassword();
    if (_password.length() == 0) {
        _password = DEFAULT_PASSWORD;
        savePassword(_password);
        Serial.println("OTA: nenhuma senha salva. Usando a senha padrao.");
    }
    ArduinoOTA.setPassword(_password.c_str());
    if (isDefaultPassword()) {
        Serial.println("AVISO: OTA com a senha padrao. Troque pelo painel web.");
    } else {
        Serial.println("OTA com senha personalizada.");
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

// ---------------- Senha ----------------

bool OTAManager::isDefaultPassword() const {
    return _password == DEFAULT_PASSWORD;
}

bool OTAManager::changePassword(const String& currentPassword, const String& newPassword, String& error) {
    if (currentPassword != _password) {
        error = "Senha atual incorreta.";
        return false;
    }
    if (newPassword.length() < 4 || newPassword.length() > 32) {
        error = "A nova senha precisa ter de 4 a 32 caracteres.";
        return false;
    }
    if (!savePassword(newPassword)) {
        error = "Erro ao salvar a senha na memoria.";
        return false;
    }
    _password = newPassword;
    ArduinoOTA.setPassword(_password.c_str());  // vale na hora, sem reiniciar
    Serial.println("Senha do OTA alterada pelo painel.");
    return true;
}

String OTAManager::loadPassword() {
    if (!LittleFS.exists(OTA_CONFIG_FILE)) {
        return "";
    }
    File file = LittleFS.open(OTA_CONFIG_FILE, "r");
    if (!file) {
        Serial.println("Erro ao abrir arquivo de config OTA");
        return "";
    }
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();
    if (err) {
        Serial.println("Erro ao ler JSON da config OTA");
        return "";
    }
    return doc["ota_password"] | "";
}

bool OTAManager::savePassword(const String& password) {
    JsonDocument doc;
    doc["ota_password"] = password;
    File file = LittleFS.open(OTA_CONFIG_FILE, "w");
    if (!file) {
        Serial.println("Erro ao abrir arquivo para salvar config OTA");
        return false;
    }
    bool ok = serializeJson(doc, file) > 0;
    file.close();
    return ok;
}
