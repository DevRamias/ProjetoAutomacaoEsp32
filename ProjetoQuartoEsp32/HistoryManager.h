#ifndef HISTORY_MANAGER_H
#define HISTORY_MANAGER_H

#include <Arduino.h>

class DHTManager;
class RelayManager;
class NTPManager;

// Historico das ultimas 24 h para o grafico do painel.
//  - Uma amostra a cada 5 min (288 amostras = 24 h) num buffer circular em RAM
//    (8 bytes cada, ~2,3 KB no total).
//  - So grava depois que a hora veio da internet (antes disso o relogio e falso).
//  - Usa os valores em cache do DHTManager (nao faz leituras extras do sensor).
//  - Salva no LittleFS (/history.bin) a cada 30 min, para sobreviver a
//    reinicios e OTA sem desgastar a flash (~48 gravacoes de 2 KB por dia).
class HistoryManager {
public:
  static const uint16_t CAPACITY = 288;           // 24 h * 12 amostras por hora
  static const uint32_t STEP_SECONDS = 300;       // 5 min entre amostras
  static const unsigned long SAVE_INTERVAL_MS = 30UL * 60UL * 1000UL;

  struct Sample {
    uint32_t time;      // epoch (UTC) do fim do intervalo
    int16_t temp10;     // temperatura em decimos de grau (INT16_MIN = sem leitura)
    uint8_t humidity;   // umidade em % (255 = sem leitura)
    uint8_t flags;      // bit 0: ventilador ligado em algum momento do intervalo
  };

  void begin(DHTManager* dht, RelayManager* relay, NTPManager* ntp);
  void update();      // chamado no loop(); nunca bloqueia (sem delay)

  uint16_t count() const { return _count; }
  // i = 0 e a amostra mais antiga
  const Sample& at(uint16_t i) const;

private:
  DHTManager* _dht = nullptr;
  RelayManager* _relay = nullptr;
  NTPManager* _ntp = nullptr;

  Sample _buf[CAPACITY];
  uint16_t _head = 0;    // proxima posicao a escrever
  uint16_t _count = 0;

  uint32_t _lastSlot = 0;          // bloco de 5 min atual (epoch / 300)
  bool _onInInterval = false;      // ventilador ligou neste bloco?
  unsigned long _lastRelayCheck = 0;
  unsigned long _lastSaveAt = 0;
  bool _dirty = false;

  void record(uint32_t epoch);
  void push(const Sample& s);
  void load();
  void save();
};

#endif
