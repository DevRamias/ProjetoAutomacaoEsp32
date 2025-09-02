#include "RelayManager.h"

// --- Construtor ---
// Inicializa todas as variáveis, incluindo as novas do modo automático.
RelayManager::RelayManager(int pin) : 
  relayPin(pin), 
  relayStartTime(0), 
  relayDuration(0), 
  relayActive(false),
  _autoCycleActive(false), 
  _isVentilating(false), 
  _isInStandby(false), 
  _ventilationDurationMs(0), 
  _standbyDurationMs(0), 
  _lastStateChangeTime(0), 
  _triggerTemperature(0.0f) {}

// --- Métodos Principais ---

void RelayManager::begin() {
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Garante que o relé comece desligado
}

// Esta é a nova função principal, o cérebro do sistema.
void RelayManager::update(float currentTemperature) {
  // 1. Lógica do modo MANUAL (só funciona se o modo auto estiver desligado)
  if (relayActive && !_autoCycleActive && (millis() - relayStartTime >= relayDuration)) {
    stop(); // Usa a função stop() para consistência
    Serial.println("[MANUAL] Ventilador desligado automaticamente por tempo.");
  }

  // 2. Lógica do modo AUTOMÁTICO (ignora tudo se não estiver ativo)
  if (!_autoCycleActive) {
    return;
  }

  unsigned long now = millis();

  if (_isVentilating) {
    // Estamos ventilando. Checa se o tempo de ventilação acabou.
    if (now - _lastStateChangeTime >= _ventilationDurationMs) {
      digitalWrite(relayPin, LOW);
      _isVentilating = false;
      _isInStandby = true;
      _lastStateChangeTime = now; // Marca o início do standby
      Serial.println("[AUTO] Fim da ventilação, iniciando standby.");
    }
  } else if (_isInStandby) {
    // Estamos em standby. Checa se o tempo de standby acabou.
    if (now - _lastStateChangeTime >= _standbyDurationMs) {
      _isInStandby = false; // Fim do standby.
      Serial.println("[AUTO] Fim do standby. Aguardando verificação de temperatura.");
      // Na próxima iteração do loop, a condição de temperatura será checada.
    }
  } else {
    // Não estamos nem ventilando, nem em standby. Hora de checar a temperatura.
    if (!isnan(currentTemperature) && currentTemperature >= _triggerTemperature) {
      digitalWrite(relayPin, HIGH);
      _isVentilating = true;
      _lastStateChangeTime = now; // Marca o início da ventilação
      Serial.printf("[AUTO] Temperatura (%.1fC) atingiu o gatilho (%.1fC). Iniciando ventilação.\n", currentTemperature, _triggerTemperature);
    }
  }
}

// --- Controle Manual ---

// O controle manual agora desativa o modo automático para evitar conflitos.
void RelayManager::start(unsigned long duration) {
  if (_autoCycleActive) {
    stopAutoCycle();
  }
  relayDuration = duration * 60 * 1000; // Converte minutos para ms
  relayStartTime = millis();
  relayActive = true;
  digitalWrite(relayPin, HIGH);
  Serial.printf("[MANUAL] Ventilador ligado por %lu minutos.\n", duration);
}

// A função stop() agora é um "desliga tudo" geral.
void RelayManager::stop() {
  if (_autoCycleActive) {
    stopAutoCycle();
  }
  relayActive = false;
  digitalWrite(relayPin, LOW);
  Serial.println("[GERAL] Comando STOP recebido. Todas as operações paradas.");
}

// --- Controle Automático ---

void RelayManager::startAutoCycle(unsigned long ventMinutes, unsigned long standbyMinutes, float triggerTemp) {
  _autoCycleActive = true;
  _ventilationDurationMs = ventMinutes * 60 * 1000;
  _standbyDurationMs = standbyMinutes * 60 * 1000;
  _triggerTemperature = triggerTemp;
  
  // Reseta os estados para um início limpo
  _isVentilating = false;
  _isInStandby = false; 
  digitalWrite(relayPin, LOW); // Garante que comece desligado
  
  Serial.println("[AUTO] Ciclo automático ATIVADO.");
}

void RelayManager::stopAutoCycle() {
  _autoCycleActive = false;
  _isVentilating = false;
  _isInStandby = false;
  digitalWrite(relayPin, LOW); // Garante que o relé desligue ao parar o ciclo
  Serial.println("[AUTO] Ciclo automático DESATIVADO.");
}

// --- Métodos de Status ---

// Retorna true se o relé está fisicamente LIGADO, seja por modo manual ou automático.
bool RelayManager::isActive() {
  return digitalRead(relayPin) == HIGH;
}

bool RelayManager::isAutoCycleActive() {
  return _autoCycleActive;
}

// Funções para o modo manual (usadas pela página web para a contagem regressiva)
unsigned long RelayManager::getStartTime() {
  return relayStartTime;
}

unsigned long RelayManager::getDuration() {
  return relayDuration;
}
