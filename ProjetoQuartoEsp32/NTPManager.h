#ifndef NTP_MANAGER_H
#define NTP_MANAGER_H

#include <NTPClient.h>
#include <WiFiUdp.h>

class NTPManager {
public:
  void begin();
  void update();
  String getFormattedTime();
};

#endif