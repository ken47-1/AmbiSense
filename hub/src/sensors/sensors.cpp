/* ==================== sensors.cpp ==================== */
#include "sensors/sensors.h"

/* =============== INCLUDES =============== */
/* ============ CORE ============ */
#include <Arduino.h>
#include <Wire.h>

/* ============ THIRD-PARTY ============ */
#include <SensirionI2cSht4x.h>

namespace AmbiSense::Hub {

/* =============== PRIVATE =============== */
static SensirionI2cSht4x _sht4x;

/* =============== PUBLIC API =============== */
/* ============ LIFECYCLE ============ */
Sensors::Sensors()
    : _temp(0.0f)
    , _humidity(0.0f)
    , _valid(false)
    , _lastRead(0)
{}

void Sensors::begin() {
    _sht4x.begin(Wire, SHT40_I2C_ADDR_44);
}

void Sensors::update() {
    if (millis() - _lastRead < SENSOR_INTERVAL_MS) return;
    _lastRead = millis();

    float temperature;
    float humidity;

    int16_t error = _sht4x.measureHighPrecision(
        temperature,
        humidity
    );

    if (error != 0) {
        _valid = false;
        return;
    }

    _temp     = temperature;
    _humidity = humidity;
    _valid    = true;
}

} // namespace AmbiSense::Hub
