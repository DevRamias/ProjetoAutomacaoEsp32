#include "NTPManager.h"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000);

void NTPManager::begin() {
  timeClient.begin();
}

void NTPManager::update() {
  timeClient.update();
}

String NTPManager::getFormattedTime() {
  return timeClient.getFormattedTime();
}