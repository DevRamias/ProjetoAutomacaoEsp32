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

private:
    DHT _dht;
    float _lastValidTemp;
    float _lastValidHumidity;
    unsigned long _lastReadingTime;
    unsigned long _cacheDuration;
    bool _error;
    float _humidityOffset;
};

#endif