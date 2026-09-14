# Debug Output Standard

Defines the required structure and conventions for debug output in this project.

---

## General Rules

- One module owns the debug system: `Debug`.
- Modules call `DBG_PRINT`. They never call `Serial.print` directly.
- The compile-time gate is `DEBUG_ENABLED`. The runtime gate is the channel mask.
- Channel names live in one enum, `Debug::Ch`.
- Every enum value carries a `CH_` prefix. See "Naming".
- Emit formatted lines. Do not build strings by hand.
- Do not print inside ISR context.

---

## Layout

- `include/debug/debug.h` — enum, API, `DBG_PRINT` macro.
- `src/debug/debug.cpp` — mask, dispatch, tag formatting.

No other file defines a channel or emits a log line.

---

## Channel Enum

```cpp
enum class Ch : uint8_t {
    CH_NETWORK = 0,
    CH_GEO     = 1,
    CH_WEATHER = 2,
    CH_SENSORS = 3,
    CH_RTC     = 4,
    CH_COUNT   = 5
};
```

Rules:

- `CH_COUNT` is last and is not a real channel.
- Values are contiguous, starting at 0.
- The mask is 8 bits. Do not exceed 8 channels without widening the mask.
- Channel names describe the module, not the action.

---

## Naming

- Prefix every enum value with `CH_`. The preprocessor replaces bare names before the compiler applies `enum class` scoping.
- Arduino core reserves: `INPUT`, `OUTPUT`, `HIGH`, `LOW`, `B0`–`B15`, `LED_BUILTIN`, `PI`, `min`, `max`, `abs`.
- ESP-IDF reserves: `NETWORK`, `DISPLAY`, `WIFI_*`.
- The `CH_` prefix avoids all of them.
- The macro is `DBG_PRINT`, not `DEBUG_PRINT`. Bare `DEBUG` is reserved by some toolchains.
- Each channel has a 3-character tag: `NET`, `GEO`, `WEA`, `SNR`, `RTC`. Tags match across the two firmwares where the channel is shared.

---

## API

- `Debug::init()` — clear the mask. Call once, from `setup()`.
- `Debug::reset()` — same as `init()`. Use from tests.
- `Debug::set(Ch, bool)` — enable or disable one channel.
- `Debug::toggle(Ch)` — flip one channel.
- `Debug::set_all(bool)` — enable or disable all.
- `Debug::enabled(Ch)` — query one channel. Used by the macro.
- `Debug::dump()` — print the mask and per-channel state.
- `Debug::emit(Ch, fmt, ...)` — formatted write. Not for direct use; go through the macro.

---

## Macro

```cpp
DBG_PRINT(ch, fmt, ...)
```

- Expands to a scoped check plus `Debug::emit`.
- When `DEBUG_ENABLED` is 0, expands to `do {} while (0)`. Arguments are not evaluated.
- `fmt` and `...` follow `printf` rules.
- Buffer is 96 bytes. Longer output is truncated.

---

## Call Sites

- One `DBG_PRINT` per logical event. Do not split one event across two calls.
- Write the message so the tag prefix is not repeated in the text.
  - Correct: `DBG_PRINT(Ch::CH_NETWORK, "Config TX failed: %d", err);`
  - Wrong: `DBG_PRINT(Ch::CH_NETWORK, "[NET] Config TX failed: %d", err);`
- Keep the format string under 60 characters. Details belong in the arguments.
- Never log inside a hot path without a rate limit or a counter.
- Never log a pointer, an address, or memory contents at `Ch::CH_NETWORK` or higher. Use a dedicated channel if needed.

---

## Behavior

- `init()` clears the mask. All channels off.
- Enabling a channel takes effect on the next `DBG_PRINT` call. No re-init needed.
- `emit` writes one line: tag, space, formatted text, newline.
- `dump` prints the mask in hex, then one line per channel with `ON` or `OFF`.
- The module holds no buffers between calls. State is the mask only.
