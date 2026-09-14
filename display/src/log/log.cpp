/* ==================== log.cpp ==================== */
#include "log/log.h"

/* =============== INCLUDES =============== */

/* ============ CONFIG ============ */
#include "config/LogConfig.h"

/* ============ CORE ============ */
#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

namespace Log {

/* =============== INTERNAL STATE =============== */
/* ============ STATIC VARS ============ */
static uint8_t levelMask_   = 0;
static uint8_t channelMask_ = 0;

/* =============== INTERNAL HELPERS =============== */
/* ============ LOGIC ============ */
static const char lvl_char(Lvl l) {
    switch (l) {
        case Lvl::D: return 'D';
        case Lvl::I: return 'I';
        case Lvl::W: return 'W';
        case Lvl::E: return 'E';
        default:     return '?';
    }
}

/* 4 chars, space-padded */
static const char* tag_of(Ch c) {
    switch (c) {
        case Ch::CH_SYS: return "SYS ";
        case Ch::CH_NET: return "NET ";
        case Ch::CH_DSP: return "DSP ";
        case Ch::CH_UI:  return "UI  ";
        default:         return "??? ";
    }
}

/* =============== PUBLIC API =============== */
void init() {
    /* I, W, E on. D off. All channels on. */
    levelMask_   = (1u << (uint8_t)Lvl::I)
                 | (1u << (uint8_t)Lvl::W)
                 | (1u << (uint8_t)Lvl::E);
    channelMask_ = 0xFF;
}

void reset() {
    levelMask_   = 0;
    channelMask_ = 0;
}

bool enabled(Lvl l, Ch c) {
    if ((uint8_t)l >= (uint8_t)Lvl::COUNT) return false;
    if ((uint8_t)c >= (uint8_t)Ch::COUNT)  return false;
    return ((levelMask_   & (1u << (uint8_t)l)) != 0)
        && ((channelMask_ & (1u << (uint8_t)c)) != 0);
}

bool isLevelEnabled(Lvl l) {
    if ((uint8_t)l >= (uint8_t)Lvl::COUNT) {
        return false;
    }
    return (levelMask_ & (1u << (uint8_t)l)) != 0;
}

bool isChannelEnabled(Ch c) {
    if ((uint8_t)c >= (uint8_t)Ch::COUNT) {
        return false;
    }
    return (channelMask_ & (1u << (uint8_t)c)) != 0;
}

void setLevel(Lvl l, bool on) {
    if ((uint8_t)l >= (uint8_t)Lvl::COUNT) return;
    uint8_t bit = 1u << (uint8_t)l;
    if (on) levelMask_ |= bit; else levelMask_ &= ~bit;
}

void setChannel(Ch c, bool on) {
    if ((uint8_t)c >= (uint8_t)Ch::COUNT) return;
    uint8_t bit = 1u << (uint8_t)c;
    if (on) channelMask_ |= bit; else channelMask_ &= ~bit;
}

void setAllLevels(bool on)   { levelMask_   = on ? 0xFF : 0x00; }
void setAllChannels(bool on) { channelMask_ = on ? 0xFF : 0x00; }

void dump() {
    static const char* lvls[] = {"D", "I", "W", "E"};
    static const char* chs[] = {"SYS", "NET", "DSP", "UI"};

    Serial.printf("level mask:   0x%02X\n", levelMask_);
    Serial.printf("channel mask: 0x%02X\n", channelMask_);

    for (uint8_t i = 0; i < (uint8_t)Lvl::COUNT; i++) {
        Serial.printf("  %s: %s\n", lvls[i],
            (levelMask_ & (1u << i)) ? "ON" : "OFF");
    }
    for (uint8_t i = 0; i < (uint8_t)Ch::COUNT; i++) {
        Serial.printf("  %s: %s\n", chs[i],
            (channelMask_ & (1u << i)) ? "ON" : "OFF");
    }
}

void dumpShort() {
    static const char* lvls[] = {"D", "I", "W", "E"};
    static const char* chs[] = {"SYS", "NET", "DSP", "UI"};

    Serial.print(F("Lvl:"));
    for (uint8_t i = 0; i < (uint8_t)Lvl::COUNT; i++) {
        if (levelMask_ & (1u << i)) {
            Serial.print(' ');
            Serial.print(lvls[i]);
        }
    }
    Serial.println();

    Serial.print(F("Ch: "));
    for (uint8_t i = 0; i < (uint8_t)Ch::COUNT; i++) {
        if (channelMask_ & (1u << i)) {
            Serial.print(chs[i]);
            Serial.print(' ');
        }
    }
    Serial.println();
}

void write(Lvl l, Ch c, const char* fmt, ...) {
    Serial.printf("[%c][%s] ", lvl_char(l), tag_of(c));

    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Serial.println(buf);
}

} // namespace Log