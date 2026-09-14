/* ==================== debug.h ==================== */
#pragma once

/* =============== INCLUDES =============== */

/* ============ CONFIG ============ */
#include "config/DebugConfig.h"

/* ============ CORE ============ */
#include <stdint.h>

namespace Debug {

/* =============== TYPES =============== */
/* ============ ENUMS ============ */
enum class Ch : uint8_t {
    CH_NETWORK = 0,
    CH_DISPLAY = 1,
    CH_UI      = 2,
    CH_COUNT   = 3
};

/* =============== API =============== */
void init();
void reset();

bool enabled(Ch c);
void set(Ch c, bool on);
void toggle(Ch c);

void set_all(bool on);
void dump_short();
void dump();

void emit(Ch c, const char* fmt, ...);

} // namespace Debug

/* =============== MACROS =============== */
#if DEBUG_ENABLED

#define DBG_PRINT(ch, ...)                    \
    do {                                      \
        if (Debug::enabled(ch)) {             \
            Debug::emit(ch, __VA_ARGS__);     \
        }                                     \
    } while (0)

#else

#define DBG_PRINT(ch, ...) do {} while (0)

#endif // DEBUG_ENABLED