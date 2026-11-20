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

    // Interferencia do rele: com a bobina ligada (mesmo 3,3 V do sensor) o
    // DHT11 marca a umidade ~20-27% acima do real (medido em 20/11/2026:
    // 66% -> 87-93%); a temperatura nao muda. Enquanto o rele estiver ligado,
    // e ate HUMIDITY_SETTLE_MS depois de desligar, a umidade fica segurada no
    // ultimo valor bom medido ANTES de ligar. Chamar a cada volta do loop().
    void setRelayActive(bool active);
    bool isHumidityHeld() const;   // true = umidade segurada (nao e medida agora)
    static const unsigned long HUMIDITY_SETTLE_MS = 45000;

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
    bool _relayActive;
    unsigned long _relayOffAt;     // quando o rele desligou (0 = nunca)
};

#endif