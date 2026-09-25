// DHTManager.cpp
#include "DHTManager.h"
#include <math.h>

DHTManager::DHTManager(uint8_t pin, uint8_t type) : 
    _dht(pin, type),
    _lastValidTemp(NAN),
    _lastValidHumidity(NAN),
    _lastReadingTime(0),
    _lastHumidityTime(0),
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

    // CORRIGIDO: uma leitura por ciclo. O laço antigo de "3 tentativas" nao
    // respeitava o intervalo minimo do DHT11 (~1s) e, na pratica, fazia sempre
    // a mesma leitura. O cache acima e que controla a frequencia das leituras.
    float temp = _dht.readTemperature();
    if (!isnan(temp)) {
        _lastValidTemp = temp;
        _lastReadingTime = millis();
        _error = false;
    } else {
        // Mantem o ultimo valor valido e sinaliza a falha em isError()
        _error = true;
    }

    return _lastValidTemp;
}

float DHTManager::readHumidity() {
    // Retorna cache se válido (CORRIGIDO: cache proprio da umidade)
    if (millis() - _lastHumidityTime < _cacheDuration && !isnan(_lastValidHumidity)) {
        return _lastValidHumidity + _humidityOffset;
    }

    // CORRIGIDO: uma leitura por ciclo (mesmo motivo da temperatura)
    float h = _dht.readHumidity();
    if (!isnan(h) && h >= 1.0 && h <= 99.9) { // Filtra valores absurdos
        _lastValidHumidity = h;
        _lastHumidityTime = millis();
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
    // Devolve o instante da leitura mais recente (temperatura ou umidade)
    return (_lastHumidityTime > _lastReadingTime) ? _lastHumidityTime : _lastReadingTime;
}

float DHTManager::getLastTemperature() const {
    return _lastValidTemp;
}

float DHTManager::getLastHumidity() const {
    return _lastValidHumidity;
}

// Sensacao termica (indice de calor) - algoritmo oficial do NWS/NOAA.
// CORRIGIDO: a regressao de Rothfusz so vale no calor (>= 80 F / 26,7 C).
// A versao anterior calculava Rothfusz primeiro e, no frio com umidade alta,
// ela "explodia" (ex.: 10 C / 90% dava 27,8 C). Agora segue a ordem do NWS:
//   1) calcula a formula simples (Steadman);
//   2) so troca para Rothfusz se a media (simples + temperatura) der >= 80 F.
float DHTManager::heatIndex(float temperatureC, float humidityPercent) {
    if (isnan(temperatureC) || isnan(humidityPercent)) {
        return NAN;
    }

    // Converte para Fahrenheit e limita a umidade ao intervalo fisico
    float t = temperatureC * 9.0f / 5.0f + 32.0f;
    float rh = humidityPercent;
    if (rh < 0.0f) rh = 0.0f;
    if (rh > 100.0f) rh = 100.0f;

    // 1) Formula simples (vale para temperaturas amenas e frias)
    float hi = 0.5f * (t + 61.0f + (t - 68.0f) * 1.2f + rh * 0.094f);

    // 2) No calor, usa a regressao de Rothfusz com os ajustes do NWS.
    //    A media (hi + t) / 2 serve so para DECIDIR qual formula usar.
    if ((hi + t) / 2.0f >= 80.0f) {
        hi = -42.379f
           + 2.04901523f * t
           + 10.14333127f * rh
           - 0.22475541f * t * rh
           - 0.00683783f * t * t
           - 0.05481717f * rh * rh
           + 0.00122874f * t * t * rh
           + 0.00085282f * t * rh * rh
           - 0.00000199f * t * t * rh * rh;

        // Ajuste para ar muito seco
        if (rh < 13.0f && t >= 80.0f && t <= 112.0f) {
            hi -= ((13.0f - rh) / 4.0f) * sqrtf((17.0f - fabsf(t - 95.0f)) / 17.0f);
        }
        // Ajuste para ar muito umido
        else if (rh > 85.0f && t >= 80.0f && t <= 87.0f) {
            hi += ((rh - 85.0f) / 10.0f) * ((87.0f - t) / 5.0f);
        }
    }

    // Volta para Celsius
    return (hi - 32.0f) * 5.0f / 9.0f;
}