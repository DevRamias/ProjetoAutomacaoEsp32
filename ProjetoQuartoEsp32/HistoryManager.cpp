#include "HistoryManager.h"
#include "DHTManager.h"
#include "RelayManager.h"
#include "NTPManager.h"
#include <LittleFS.h>
#include <time.h>
#include <math.h>

// Arquivo binario: cabecalho (magia + versao + quantidade) e as amostras em
// ordem cronologica. Se a magia/versao nao bater, o arquivo e ignorado.
static const char* HISTORY_FILE = "/history.bin";
static const char* HISTORY_TMP = "/history.tmp";
static const uint32_t HISTORY_MAGIC = 0x48495354;  // "HIST"
static const uint16_t HISTORY_VERSION = 1;

struct HistoryHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t count;
};

void HistoryManager::begin(DHTManager* dht, RelayManager* relay, NTPManager* ntp) {
  _dht = dht;
  _relay = relay;
  _ntp = ntp;
  load();
  _lastSaveAt = millis();
}

const HistoryManager::Sample& HistoryManager::at(uint16_t i) const {
  uint16_t inicio = (_head + CAPACITY - _count) % CAPACITY;
  return _buf[(inicio + i) % CAPACITY];
}

void HistoryManager::update() {
  // Anota se o ventilador esteve ligado no bloco (confere 1x por segundo)
  if (millis() - _lastRelayCheck >= 1000) {
    _lastRelayCheck = millis();
    if (_relay && _relay->isActive()) _onInInterval = true;

    if (_ntp && _ntp->isSynced()) {
      uint32_t agora = (uint32_t) time(nullptr);
      uint32_t slot = agora / STEP_SECONDS;
      if (_lastSlot == 0) {
        // Primeira vez com a hora certa: o bloco atual esta incompleto, so marca
        _lastSlot = slot;
      } else if (slot != _lastSlot) {
        _lastSlot = slot;
        record(slot * STEP_SECONDS);   // horario "redondo" (ex.: 21:05:00)
        _onInInterval = (_relay && _relay->isActive());
      }
    }
  }

  // Salva com pouca frequencia (gravacao rapida, ~2 KB)
  if (_dirty && millis() - _lastSaveAt >= SAVE_INTERVAL_MS) {
    _lastSaveAt = millis();
    save();
  }
}

void HistoryManager::record(uint32_t epoch) {
  // Hora voltou ou repetiu (ex.: arquivo carregado com o mesmo bloco): ignora
  if (_count > 0 && at(_count - 1).time >= epoch) return;

  Sample s;
  s.time = epoch;
  // A temperatura e lida a cada volta do loop (cache de 15 s). A umidade so e
  // lida aqui e no painel; o DHT devolve as duas na mesma leitura, entao isso
  // nao gera leitura extra do sensor.
  float t = _dht ? _dht->readTemperature() : NAN;
  float h = _dht ? _dht->readHumidity() : NAN;
  s.temp10 = isnan(t) ? INT16_MIN : (int16_t) lroundf(t * 10.0f);
  s.humidity = (isnan(h) || h < 0 || h > 100) ? 255 : (uint8_t) lroundf(h);
  s.flags = _onInInterval ? 1 : 0;
  push(s);
  _dirty = true;
}

void HistoryManager::push(const Sample& s) {
  _buf[_head] = s;
  _head = (_head + 1) % CAPACITY;
  if (_count < CAPACITY) _count++;
}

void HistoryManager::load() {
  File f = LittleFS.open(HISTORY_FILE, "r");
  if (!f) return;
  HistoryHeader h;
  bool ok = f.read((uint8_t*) &h, sizeof(h)) == sizeof(h) &&
            h.magic == HISTORY_MAGIC && h.version == HISTORY_VERSION && h.count <= CAPACITY;
  if (ok) {
    for (uint16_t i = 0; i < h.count; i++) {
      Sample s;
      if (f.read((uint8_t*) &s, sizeof(s)) != sizeof(s)) break;
      push(s);
    }
    Serial.printf("[HIST] %u amostras carregadas de %s\n", (unsigned) _count, HISTORY_FILE);
  } else {
    Serial.println("[HIST] Arquivo de historico invalido; comecando do zero.");
  }
  f.close();
}

void HistoryManager::save() {
  // Grava num temporario e troca: se faltar energia no meio, o antigo fica
  File f = LittleFS.open(HISTORY_TMP, "w");
  if (!f) {
    Serial.println("[HIST] Erro ao salvar o historico.");
    return;
  }
  HistoryHeader h = { HISTORY_MAGIC, HISTORY_VERSION, _count };
  bool ok = f.write((const uint8_t*) &h, sizeof(h)) == sizeof(h);
  for (uint16_t i = 0; ok && i < _count; i++) {
    ok = f.write((const uint8_t*) &at(i), sizeof(Sample)) == sizeof(Sample);
  }
  f.close();
  if (!ok) {
    LittleFS.remove(HISTORY_TMP);
    Serial.println("[HIST] Erro ao salvar o historico (memoria cheia?).");
    return;
  }
  LittleFS.remove(HISTORY_FILE);
  LittleFS.rename(HISTORY_TMP, HISTORY_FILE);
  _dirty = false;
  Serial.printf("[HIST] Historico salvo (%u amostras).\n", (unsigned) _count);
}
