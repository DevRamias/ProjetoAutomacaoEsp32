#ifndef RELAY_MANAGER_H
#define RELAY_MANAGER_H

class RelayManager {
public:
  RelayManager(int pin);
  void begin();
  void update();
  void start(unsigned long duration);
  void stop();
  bool isActive();
  unsigned long getStartTime();
  unsigned long getDuration();
  
private:
  int relayPin;
  unsigned long relayStartTime;
  unsigned long relayDuration;
  bool relayActive;
};

#endif