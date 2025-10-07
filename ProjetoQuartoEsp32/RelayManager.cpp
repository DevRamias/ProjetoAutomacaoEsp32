#include "RelayManager.h"
#include "NTPManager.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// --- Construtor ---
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
  _triggerTemperature(0.0f),
  _ntpManager(nullptr),
  _lastTimeCheckResult(false),
  _lastTimeCheck(0) {
  
  // Inicializa AutoSettings com valores padrão
  _autoSettings.active = false;
  _autoSettings.minTemp = 30.0f;
  _autoSettings.ventTime = 15;
  _autoSettings.standbyTime = 30;
  _autoSettings.startTime = "21:00";
  _autoSettings.endTime = "5:00";
}

// --- Métodos Principais ---

void RelayManager::begin() {
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  
  // Carrega configurações salvas
  loadAutoSettings();
  
  Serial.println("RelayManager inicializado");
}

void RelayManager::update(float currentTemperature) {
  // 1. Lógica do modo MANUAL
  if (relayActive && !_autoCycleActive && (millis() - relayStartTime >= relayDuration)) {
    stop();
    Serial.println("[MANUAL] Ventilador desligado automaticamente por tempo.");
  }

  // 2. Lógica do modo AUTOMÁTICO
  if (!_autoCycleActive) {
    return;
  }

  // Verifica se está dentro do horário permitido
  if (!shouldAutoCycleRun()) {
    if (_isVentilating || _isInStandby) {
      stopAutoCycle();
      Serial.println("[AUTO] Fora do horário permitido. Ciclo parado.");
    }
    return;
  }

  unsigned long now = millis();

  if (_isVentilating) {
    if (now - _lastStateChangeTime >= _ventilationDurationMs) {
      digitalWrite(relayPin, LOW);
      _isVentilating = false;
      _isInStandby = true;
      _lastStateChangeTime = now;
      Serial.println("[AUTO] Fim da ventilação, iniciando standby.");
    }
  } else if (_isInStandby) {
    if (now - _lastStateChangeTime >= _standbyDurationMs) {
      _isInStandby = false;
      Serial.println("[AUTO] Fim do standby. Aguardando verificação de temperatura.");
    }
  } else {
    if (!isnan(currentTemperature) && currentTemperature >= _triggerTemperature) {
      digitalWrite(relayPin, HIGH);
      _isVentilating = true;
      _lastStateChangeTime = now;
      Serial.printf("[AUTO] Temperatura (%.1fC) >= gatilho (%.1fC). Iniciando ventilação.\n", 
                    currentTemperature, _triggerTemperature);
    }
  }
}

// --- Controle Manual ---

void RelayManager::start(unsigned long duration) {
  if (_autoCycleActive) {
    stopAutoCycle();
  }
  relayDuration = duration * 60 * 1000;
  relayStartTime = millis();
  relayActive = true;
  digitalWrite(relayPin, HIGH);
  Serial.printf("[MANUAL] Ventilador ligado por %lu minutos.\n", duration);
}

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
  
  _isVentilating = false;
  _isInStandby = false; 
  digitalWrite(relayPin, LOW);
  
  Serial.println("[AUTO] Ciclo automático ATIVADO.");
}

void RelayManager::stopAutoCycle() {
  _autoCycleActive = false;
  _isVentilating = false;
  _isInStandby = false;
  digitalWrite(relayPin, LOW);
  Serial.println("[AUTO] Ciclo automático DESATIVADO.");
}

// NOVO: Configura NTPManager
void RelayManager::setNTPManager(NTPManager* ntpManager) {
  _ntpManager = ntpManager;
  Serial.println("NTPManager configurado no RelayManager");
}

// NOVO: Métodos de Configurações Automáticas

void RelayManager::setAutoSettings(const AutoSettings& settings) {
  _autoSettings = settings;
  
  // Aplica as configurações se o modo auto estiver ativo
  if (_autoSettings.active) {
    startAutoCycle(_autoSettings.ventTime, _autoSettings.standbyTime, _autoSettings.minTemp);
  } else {
    stopAutoCycle();
  }
  
  // Salva as configurações no LittleFS
  saveAutoSettings();
  
  Serial.println("Configuracoes automaticas atualizadas e salvas");
}

AutoSettings RelayManager::getAutoSettings() const {
  return _autoSettings;
}

bool RelayManager::shouldAutoCycleRun() const {
  return _autoSettings.active && isWithinActiveHours();
}

// NOVO: Persistência

void RelayManager::loadAutoSettings() {
  if (!LittleFS.exists("/auto_settings.json")) {
    Serial.println("Arquivo de configuracoes nao encontrado. Usando padroes.");
    return;
  }

  File file = LittleFS.open("/auto_settings.json", "r");
  if (!file) {
    Serial.println("Erro ao abrir arquivo de configuracoes");
    return;
  }

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.println("Erro ao ler configuracoes: " + String(error.c_str()));
    return;
  }

  _autoSettings.active = doc["active"] | false;
  _autoSettings.minTemp = doc["minTemp"] | 30.0f;
  _autoSettings.ventTime = doc["ventTime"] | 15;
  _autoSettings.standbyTime = doc["standbyTime"] | 30;
  _autoSettings.startTime = doc["startTime"] | "21:00";
  _autoSettings.endTime = doc["endTime"] | "5:00";

  Serial.println("Configuracoes automaticas carregadas");
}

void RelayManager::saveAutoSettings() {
  File file = LittleFS.open("/auto_settings.json", "w");
  if (!file) {
    Serial.println("Erro ao salvar configuracoes");
    return;
  }

  DynamicJsonDocument doc(512);
  doc["active"] = _autoSettings.active;
  doc["minTemp"] = _autoSettings.minTemp;
  doc["ventTime"] = _autoSettings.ventTime;
  doc["standbyTime"] = _autoSettings.standbyTime;
  doc["startTime"] = _autoSettings.startTime;
  doc["endTime"] = _autoSettings.endTime;

  if (serializeJson(doc, file) == 0) {
    Serial.println("Erro ao escrever configuracoes");
  } else {
    Serial.println("Configuracoes automaticas salvas");
  }
  
  file.close();
}

// NOVO: Verificação de Horários com cache

bool RelayManager::isWithinActiveHours() const {
  // Verifica a cada 30 segundos (cache)
  if (millis() - _lastTimeCheck < 30000) {
    return _lastTimeCheckResult;
  }

  _lastTimeCheck = millis();

  if (!_ntpManager) {
    Serial.println("Aviso: NTPManager nao configurado. Considerando dentro do horario.");
    _lastTimeCheckResult = true;
    return true;
  }

  String currentTime = _ntpManager->getFormattedTime();
  if (currentTime.length() >= 5) {
    currentTime = currentTime.substring(0, 5); // Pega HH:MM
  } else {
    Serial.println("Erro: Formato de hora invalido");
    _lastTimeCheckResult = true;
    return true;
  }

  // Lógica para período que cruza a meia-noite (21:00-05:00)
  if (_autoSettings.startTime <= _autoSettings.endTime) {
    // Período normal (não cruza meia-noite)
    _lastTimeCheckResult = (currentTime >= _autoSettings.startTime && 
                           currentTime <= _autoSettings.endTime);
  } else {
    // Período que cruza meia-noite (21:00-05:00)
    _lastTimeCheckResult = (currentTime >= _autoSettings.startTime || 
                           currentTime <= _autoSettings.endTime);
  }

  Serial.printf("[HORARIO] %s - %s a %s: %s\n", 
                currentTime.c_str(),
                _autoSettings.startTime.c_str(),
                _autoSettings.endTime.c_str(),
                _lastTimeCheckResult ? "DENTRO" : "FORA");

  return _lastTimeCheckResult;
}

// --- Métodos de Status ---

bool RelayManager::isActive() {
  return digitalRead(relayPin) == HIGH;
}

bool RelayManager::isAutoCycleActive() {
  return _autoCycleActive;
}

unsigned long RelayManager::getStartTime() {
  return relayStartTime;
}

unsigned long RelayManager::getDuration() {
  return relayDuration;
}