// DHTManager.h
#ifndef DHT_MANAGER_H
#define DHT_MANAGER_H

#include <DHT.h>

class DHTManager {
public:
    DHTManager(uint8_t pin, uint8_t type);
    void begin();
    float readTemperature();
    float readHumidity();
    bool isError();
    void setHumidityOffset(float offset);
    unsigned long getLastReadingTime() const;
    float getLastTemperature() const;
    float getLastHumidity() const;

    // Sensacao termica / indice de calor (formula de Rothfusz, padrao NWS/NOAA).
    // E estatica pois e um calculo puro, que nao depende do estado do sensor.
    static float heatIndex(float temperatureC, float humidityPercent);

private:
    DHT _dht;
    float _lastValidTemp;
    float _lastValidHumidity;
    unsigned long _lastReadingTime;      // instante da ultima leitura de TEMPERATURA
    unsigned long _lastHumidityTime;     // CORRIGIDO: cache proprio da UMIDADE
    unsigned long _cacheDuration;
    bool _error;
    float _humidityOffset;
};

#endif