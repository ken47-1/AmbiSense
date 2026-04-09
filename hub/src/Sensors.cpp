/* ========== Sensors.cpp ========== */
#include "Sensors.h"

/* ===== INCLUDES ===== */
/* --- CORE --- */
#include <Arduino.h>

Sensors::Sensors()
    : _dht(DHT_PIN, DHT_TYPE)
    , _temp(0.0f)
    , _humidity(0.0f)
    , _valid(false)
    , _lastRead(0)
{}

void Sensors::begin() {
    _dht.begin();
}

void Sensors::update() {
    if (millis() - _lastRead < DHT_INTERVAL_MS) return;
    _lastRead = millis();

    float t = _dht.readTemperature();
    float h = _dht.readHumidity();

    if (isnan(t) || isnan(h)) {
        _valid = false;
        return;
    }

    _temp     = t;
    _humidity = h;
    _valid    = true;
}
