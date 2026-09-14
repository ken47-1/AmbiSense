/* ==================== debug.cpp ==================== */
#include "debug/debug.h"

/* =============== INCLUDES =============== */

/* ============ CONFIG ============ */
#include "config/DebugConfig.h"

/* ============ CORE ============ */
#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

namespace Debug {

/* =============== INTERNAL STATE =============== */
/* ============ STATIC VARS ============ */
static uint8_t mask_ = 0;

/* =============== INTERNAL HELPERS =============== */
/* ============ LOGIC ============ */
static const char* tag_of(Ch c) {
    switch (c) {
        case Ch::CH_NETWORK: return "NET";
        case Ch::CH_GEO:     return "GEO";
        case Ch::CH_WEATHER: return "WEA";
        case Ch::CH_SENSORS: return "SNR";
        case Ch::CH_RTC:     return "RTC";
        default:             return "???";
    }
}

/* =============== PUBLIC API =============== */
void init() {
    mask_ = 0;
}

void reset() {
    mask_ = 0;
}

bool enabled(Ch c) {
    if ((uint8_t)c >= (uint8_t)Ch::CH_COUNT) return false;
    return (mask_ & (1u << (uint8_t)c)) != 0;
}

void set(Ch c, bool on) {
    if ((uint8_t)c >= (uint8_t)Ch::CH_COUNT) return;
    uint8_t bit = 1u << (uint8_t)c;
    if (on) {
        mask_ |= bit;
    } else {
        mask_ &= ~bit;
    }
}

void toggle(Ch c) {
    set(c, !enabled(c));
}

void set_all(bool on) {
    mask_ = on ? 0xFF : 0x00;
}

void dump_short() {
    Serial.print("[DBG] mask: 0x");
    Serial.println(mask_, HEX);
}

void dump() {
    dump_short();

    static const char* names[] = {
        "NETWORK", "GEO", "WEATHER", "SENSORS", "RTC"
    };

    for (uint8_t i = 0; i < (uint8_t)Ch::CH_COUNT; i++) {
        Serial.print("[DBG]   ");
        Serial.print(names[i]);
        Serial.print(": ");
        Serial.println(enabled((Ch)i) ? "ON" : "OFF");
    }
}

void emit(Ch c, const char* fmt, ...) {
    Serial.print('[');
    Serial.print(tag_of(c));
    Serial.print("] ");

    char buf[96];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    Serial.println(buf);
}

} // namespace Debug