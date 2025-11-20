#ifndef RELAY_MANAGER_H
#define RELAY_MANAGER_H

#include <Arduino.h>

// Forward declaration - evita include circular
class NTPManager;

// NOVO: Estrutura para configurações automáticas
struct AutoSettings {
    bool active = false;
    float minTemp = 30.0f;
    unsigned int ventTime = 15;
    unsigned int standbyTime = 30;
    String startTime = "21:00";
    String endTime = "5:00";
};

class RelayManager {
public:
    // --- Construtor ---
    RelayManager(int pin);

    // --- Métodos Principais ---
    void begin();
    void update(float currentTemperature);
    
    // --- Controle Manual ---
    void start(unsigned long duration);
    void stop();
    
    // --- Controle Automático ---
    void startAutoCycle(unsigned long ventMinutes, unsigned long standbyMinutes, float triggerTemp);
    void stopAutoCycle();
    
    // NOVO: Controle de configurações automáticas
    void setAutoSettings(const AutoSettings& settings);
    AutoSettings getAutoSettings() const;
    bool shouldAutoCycleRun() const;
    
    // NOVO: Persistência
    void loadAutoSettings();
    void saveAutoSettings();

    // ADICIONADO: Configura NTPManager
    void setNTPManager(NTPManager* ntpManager);

    // --- Métodos de Status ---
    bool isActive();
    bool isAutoCycleActive();
    unsigned long getStartTime();
    unsigned long getDuration();

private:
    // --- Variáveis de Hardware e Estado Manual ---
    int relayPin;
    unsigned long relayStartTime;
    unsigned long relayDuration;
    bool relayActive;

    // --- Variáveis para o Modo Automático ---
    bool _autoCycleActive;
    bool _isVentilating;
    bool _isInStandby;
    unsigned long _ventilationDurationMs;
    unsigned long _standbyDurationMs;
    unsigned long _lastStateChangeTime;
    float _triggerTemperature;
    
    // NOVO: Armazena configurações automáticas
    AutoSettings _autoSettings;
    
    // ADICIONADO: Ponteiro para NTPManager
    NTPManager* _ntpManager;
    
    // ADICIONADO: Cache para otimização
    mutable bool _lastTimeCheckResult;
    mutable unsigned long _lastTimeCheck;
    
    // NOVO: Verificação de horários
    bool isWithinActiveHours() const;
};

#endif