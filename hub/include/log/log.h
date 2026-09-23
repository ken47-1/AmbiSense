/* ==================== log.h ==================== */
#pragma once

/* =============== INCLUDES =============== */

/* ============ CONFIG ============ */
#include "config/LogConfig.h"

/* ============ CORE ============ */
#include <stdint.h>

namespace Log {

/* =============== TYPES =============== */
/* ============ ENUMS ============ */
enum class Lvl : uint8_t { D = 0, I = 1, W = 2, E = 3, COUNT = 4 };
enum class Ch  : uint8_t {
    CH_SYS = 0,
    CH_NET = 1,
    CH_GEO = 2,
    CH_WEA = 3,
    CH_SNR = 4,
    CH_RTC = 5,
    COUNT  = 6
};

/* =============== API =============== */
void init();
void reset();

bool enabled(Lvl l, Ch c);
bool isLevelEnabled(Lvl l);
bool isChannelEnabled(Ch c);

void setLevel(Lvl l, bool on);
void setChannel(Ch c, bool on);
void setAllLevels(bool on);
void setAllChannels(bool on);

void dump();
void dumpShort();
void write(Lvl l, Ch c, const char* fmt, ...);

} // namespace Log

/* =============== MACROS =============== */
#if LOG_ENABLED

#define LOG_D(ch, ...) do { if (Log::enabled(Log::Lvl::D, ch)) Log::write(Log::Lvl::D, ch, __VA_ARGS__); } while (0)
#define LOG_I(ch, ...) do { if (Log::enabled(Log::Lvl::I, ch)) Log::write(Log::Lvl::I, ch, __VA_ARGS__); } while (0)
#define LOG_W(ch, ...) do { if (Log::enabled(Log::Lvl::W, ch)) Log::write(Log::Lvl::W, ch, __VA_ARGS__); } while (0)
#define LOG_E(ch, ...) do { if (Log::enabled(Log::Lvl::E, ch)) Log::write(Log::Lvl::E, ch, __VA_ARGS__); } while (0)

#else

#define LOG_D(ch, ...) do {} while (0)
#define LOG_I(ch, ...) do {} while (0)
#define LOG_W(ch, ...) do {} while (0)
#define LOG_E(ch, ...) do {} while (0)

#endif // LOG_ENABLED
