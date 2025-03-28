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

private:
    DHT _dht;
    bool _error = false;
};

#endif