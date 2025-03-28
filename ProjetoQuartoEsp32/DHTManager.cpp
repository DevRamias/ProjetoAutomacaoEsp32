// DHTManager.cpp
#include "DHTManager.h"

DHTManager::DHTManager(uint8_t pin, uint8_t type) : _dht(pin, type) {}

void DHTManager::begin() {
    _dht.begin();
}

float DHTManager::readTemperature() {
    float t = _dht.readTemperature();
    _error = isnan(t);
    return t;
}

float DHTManager::readHumidity() {
    float h = _dht.readHumidity();
    _error = isnan(h);
    return h;
}

bool DHTManager::isError() {
    return _error;
}