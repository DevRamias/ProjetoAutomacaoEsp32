#ifndef NTP_MANAGER_H
#define NTP_MANAGER_H

#include <Arduino.h>

// Hora pela internet usando o SNTP nativo do ESP32.
// O ESP32 sincroniza sozinho, em segundo plano, sem travar o loop().
// (Antes usava a biblioteca NTPClient, que parava o loop por ate 1 s a cada
// tentativa e, quando a resposta nao chegava, tentava de novo em TODA volta
// do loop, deixando o painel e o OTA com ~1 s de atraso por requisicao.)
class NTPManager {
public:
  void begin();
  void update();              // mantido por compatibilidade; nao faz nada
  String getFormattedTime();  // "HH:MM:SS" no horario de Brasilia (UTC-3)

  // true depois que a hora veio da internet ao menos uma vez.
  // Antes disso o relogio marca ~21:00 (contado a partir de 1970) e NAO deve
  // ser usado para decidir horarios (o modo automatico espera).
  bool isSynced();
};

#endif
