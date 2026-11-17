#include "NTPManager.h"
#include <time.h>

// Fuso de Brasilia (UTC-3), sem horario de verao
static const long FUSO_SEGUNDOS = -3 * 3600;

void NTPManager::begin() {
  // Liga o SNTP do proprio ESP32: ele consulta os servidores em segundo plano
  // (sem bloquear) e mantem o relogio interno certo.
  configTime(FUSO_SEGUNDOS, 0, "pool.ntp.org", "a.ntp.br", "time.google.com");
}

void NTPManager::update() {
  // Nada a fazer: o SNTP nativo atualiza sozinho.
}

bool NTPManager::isSynced() {
  // Qualquer data depois de 2020 so pode ter vindo da internet
  return time(nullptr) > 1577836800;  // 01/01/2020
}

String NTPManager::getFormattedTime() {
  // Antes da primeira sincronizacao o relogio conta a partir de 1970, o que da
  // "21:00:00" + tempo ligado (mesmo comportamento da antiga NTPClient).
  time_t agora = time(nullptr);
  struct tm t;
  localtime_r(&agora, &t);
  char buf[9];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
  return String(buf);
}
