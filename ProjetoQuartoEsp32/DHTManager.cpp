// DHTManager.cpp
#include "DHTManager.h"

DHTManager::DHTManager(uint8_t pin, uint8_t type) : 
    _dht(pin, type),
    _lastValidTemp(NAN),
    _lastValidHumidity(NAN),
    _lastReadingTime(0),
    _cacheDuration(15000), // 15 segundos de cache
    _error(false),
    _humidityOffset(0.0) {} // Offset inicial zero

void DHTManager::begin() {
    _dht.begin();
    // Força primeira leitura ao iniciar
    readTemperature();
    readHumidity();
}

float DHTManager::readTemperature() {
    // Retorna cache se válido
    if (millis() - _lastReadingTime < _cacheDuration && !isnan(_lastValidTemp)) {
        return _lastValidTemp;
    }

    // Faz 3 tentativas de leitura sem bloquear
    float tempSum = 0;
    int validReadings = 0;
    unsigned long startTime = millis();

    for (int i = 0; i < 3; i++) {
        if (millis() - startTime >= i * 10) { // Aguarda 10ms entre as leituras
            float temp = _dht.readTemperature();
            if (!isnan(temp)) {
                tempSum += temp;
                validReadings++;
            }
        }
    }

    if (validReadings > 0) {
        _lastValidTemp = tempSum / validReadings;
        _lastReadingTime = millis();
        _error = false;
    } else {
        _error = true;
    }

    return _lastValidTemp;
}

float DHTManager::readHumidity() {
    // Retorna cache se válido
    if (millis() - _lastReadingTime < _cacheDuration && !isnan(_lastValidHumidity)) {
        return _lastValidHumidity + _humidityOffset;
    }

    // Faz 3 tentativas de leitura sem bloquear
    float humiditySum = 0;
    int validReadings = 0;
    unsigned long startTime = millis();

    for (int i = 0; i < 3; i++) {
        if (millis() - startTime >= i * 10) { // Aguarda 10ms entre as leituras
            float h = _dht.readHumidity();
            if (!isnan(h) && h >= 1.0 && h <= 99.9) { // Filtra valores absurdos
                humiditySum += h;
                validReadings++;
            }
        }
    }

    if (validReadings > 0) {
        _lastValidHumidity = humiditySum / validReadings;
        _lastReadingTime = millis();
        _error = false;
    } else {
        _error = true;
    }

    return _lastValidHumidity + _humidityOffset;
}

bool DHTManager::isError() {
    return _error;
}

void DHTManager::setHumidityOffset(float offset) {
    _humidityOffset = offset;
}

unsigned long DHTManager::getLastReadingTime() const {
    return _lastReadingTime;
}

float DHTManager::getLastTemperature() const {
    return _lastValidTemp;
}

float DHTManager::getLastHumidity() const {
    return _lastValidHumidity;
}