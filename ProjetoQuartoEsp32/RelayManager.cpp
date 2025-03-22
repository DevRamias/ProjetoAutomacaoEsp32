#include <Arduino.h>
#include "RelayManager.h"

RelayManager::RelayManager(int pin) : relayPin(pin), relayStartTime(0), relayDuration(0), relayActive(false) {}

void RelayManager::begin() {
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
}

void RelayManager::update() {
  if (relayActive && millis() - relayStartTime >= relayDuration) {
    relayActive = false;
    digitalWrite(relayPin, LOW);
    Serial.println("Ventilador desligado automaticamente!");
  }
}

void RelayManager::start(unsigned long duration) {
  relayDuration = duration * 60 * 1000; // Converte minutos para milissegundos
  relayStartTime = millis();
  relayActive = true;
  digitalWrite(relayPin, HIGH);
}

void RelayManager::stop() {
  relayActive = false;
  digitalWrite(relayPin, LOW);
}

bool RelayManager::isActive() {
  return relayActive;
}