/* ==================== sensors.h ==================== */
#pragma once

/* =============== INCLUDES =============== */

/* ============ CONFIG ============ */
#include "config/HubConfig.h"
#include "config/HardwareConfig.h"

/* ============ THIRD-PARTY ============ */
#include <DHT.h>

namespace AmbiSense::Hub {

/* =============== API =============== */
class Sensors {
public:
    Sensors();
    void begin();
    void update();

    float getTemp()     const { return _temp; }
    float getHumidity() const { return _humidity; }
    bool  isValid()     const { return _valid; }

private:
    DHT   _dht;
    float _temp;
    float _humidity;
    bool  _valid;
    uint32_t _lastRead;
};

} // namespace AmbiSense::Hub