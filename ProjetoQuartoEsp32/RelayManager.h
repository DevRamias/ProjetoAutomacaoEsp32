#ifndef RELAY_MANAGER_H
#define RELAY_MANAGER_H

#include <Arduino.h>

class RelayManager {
public:
  // --- Construtor ---
  RelayManager(int pin);

  // --- Métodos Principais ---
  void begin();
  void update(float currentTemperature); // Agora recebe a temperatura atual
  
  // --- Controle Manual ---
  void start(unsigned long duration); // Inicia o modo manual por X minutos
  void stop();                        // Para qualquer operação (manual ou automática)

  // --- Controle Automático ---
  void startAutoCycle(unsigned long ventMinutes, unsigned long standbyMinutes, float triggerTemp);
  void stopAutoCycle();

  // --- Métodos de Status ---
  bool isActive();            // Retorna true se o relé está fisicamente ligado
  bool isAutoCycleActive();   // Retorna true se o modo automático está habilitado
  unsigned long getStartTime(); // Para o modo manual
  unsigned long getDuration();  // Para o modo manual

private:
  // --- Variáveis de Hardware e Estado Manual ---
  int relayPin;
  unsigned long relayStartTime;   // Início do ciclo manual
  unsigned long relayDuration;    // Duração do ciclo manual
  bool relayActive;               // Flag para o modo manual

  // --- Novas Variáveis para o Modo Automático ---
  bool _autoCycleActive;          // Flag principal do modo automático
  bool _isVentilating;            // True se está no período de ventilação
  bool _isInStandby;              // True se está no período de espera (standby)
  
  unsigned long _ventilationDurationMs; // Duração da ventilação em ms
  unsigned long _standbyDurationMs;     // Duração do standby em ms
  unsigned long _lastStateChangeTime;   // Momento da última mudança de estado (início da ventilação ou do standby)
  
  float _triggerTemperature;      // Temperatura que aciona o ciclo
};

#endif
