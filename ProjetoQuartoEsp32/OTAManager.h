// OTAManager.h
#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <ArduinoOTA.h>

// Gerencia a gravacao pelo Wi-Fi (OTA) e a SENHA do OTA.
// - O OTA SEMPRE exige senha.
// - Na primeira vez (ou se o arquivo estiver vazio) a senha e "senha".
// - A senha fica salva em /ota_config.json (LittleFS) e pode ser trocada
//   pelo painel web (rota /set-ota-password), valendo na hora.
class OTAManager {
public:
    OTAManager();
    void begin(const char* hostname);
    void handle();

    // Troca a senha. Exige a senha atual. Em caso de erro, preenche 'error'.
    bool changePassword(const String& currentPassword, const String& newPassword, String& error);

    // true enquanto a senha ainda for a padrao ("senha")
    bool isDefaultPassword() const;

    static const char* DEFAULT_PASSWORD;

private:
    String _password;
    String loadPassword();
    bool savePassword(const String& password);
    void configureOTAEvents();
};

#endif
