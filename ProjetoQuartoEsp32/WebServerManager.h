#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include "DHTManager.h"
#include "RelayManager.h"
#include "NTPManager.h"
#include <WebServer.h>
#include <WiFiManager.h>
#include <esp_system.h>  // Para funções de sistema do ESP32

class WebServerManager {
public:
    void begin(RelayManager* relayManager, NTPManager* ntpManager, 
              WiFiManager* wifiManager, DHTManager* dhtManager);
    void handleClient();
    
    // Controle automático
    void verificarCondicoesAutomaticas();
    void setAutoMode(bool active);
    bool getAutoMode() const;
    
    // Monitoramento do sistema
    void logMemoryUsage();
    void handleSystemInfo();
    
    // Configurações automáticas
    void setAutoSettings(float minTemp, int ventDuration, int standby);
    void getAutoSettings(float& minTemp, int& ventDuration, int& standby) const;

private:
    WebServer server;
    
    // Gerenciadores
    RelayManager* relayManager;
    NTPManager* ntpManager;
    WiFiManager* wifiManager;
    DHTManager* dhtManager;
    
    // Controle de conexão
    bool shouldStartPortal;
    unsigned long _lastMemoryLog;
    
    // Configurações automáticas
    struct {
        bool active;
        float minTemp;
        int ventilationDuration;  // em minutos
        int standbyDuration;     // em minutos
        unsigned long lastCheck;
    } autoModeConfig;

    // Handlers das rotas
    void handleRoot();
    void handleStart();
    void handleStop();
    void handleTime();
    void handleWiFiConfig();
    void handleStatus();
    void handleRemaining();
    void handleSensorData();
    void handleAutoSettings();
    
    // Utilitários
    String formatUptime() const;
    const char* getResetReason() const;
};

#endif