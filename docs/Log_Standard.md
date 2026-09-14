# Log Output Standard

Defines the required structure and conventions for log output in this project.

---

## General Rules

- One module owns the log system: `Log`.
- Modules call `LOG_D`, `LOG_I`, `LOG_W`, or `LOG_E`. They never call `Serial.print` or `Serial.printf` directly.
- The compile-time gate is `LOG_ENABLED`. The runtime gates are the level mask and the channel mask.
- Level names live in `Log::Lvl`. Channel names live in `Log::Ch`.
- Every channel value carries a `CH_` prefix. See "Naming".
- Every log line carries a level and a channel.
- Emit formatted lines. Do not build strings by hand.
- Do not print inside ISR context.

---

## Layout

- `include/log/log.h` — level enum, channel enum, API, `LOG_*` macros.
- `src/log/log.cpp` — masks, dispatch, tag formatting.
- `include/config/LogConfig.h` — `LOG_ENABLED`.

No other file defines a level, defines a channel, or emits a log line.

Each firmware keeps its own copy of the three files. Channels and levels are chosen per firmware. Divergence is expected.

When you fix a real bug (not a feature), check the other firmware by hand. If the bug exists there, apply the same fix.

---

## Format

```
[<lvl>][<tag>] <message>
```

- `<lvl>` is one character: `D`, `I`, `W`, or `E`.
- `<tag>` is four characters, uppercase, space-padded.
- One space separates the tag bracket from the message.
- One newline ends the line.

Examples:

```
[I][SYS ] boot complete
[I][NET ] ESP-NOW peer added: 24:6f:28:...
[W][WEA ] stale data, using last good
[E][GEO ] fetch failed: -1
```

The level goes first so errors land in the left column.

No timestamps.

---

## Levels

```
enum class Lvl : uint8_t { D = 0, I = 1, W = 2, E = 3, COUNT = 4 };
```

| Letter | Name | Use | Default |
|---|---|---|---|
| `D` | Debug | Developer detail, per-cycle traces | off |
| `I` | Info | Normal lifecycle events | on |
| `W` | Warn | Recoverable problem, degraded state | on |
| `E` | Error | Failure, fetch error, dropped packet | on |

`COUNT` is last and is not a real level.

Default mask: `I | W | E`. Debug is off.

---

## Channels

```
enum class Ch : uint8_t { CH_SYS = 0, ..., COUNT = N };
```

Rules:

- `COUNT` is last and is not a real channel.
- Values are contiguous, starting at 0.
- The mask is 8 bits. Do not exceed 8 channels without widening the mask.
- Channel names describe the module, not the action.

The channel set is firmware-specific. Choose the channels that match that firmware's
modules. The sets below are examples, not a required list.

## Example Channel Set

A firmware picks its own `Ch` values. Here is one example.

| Value | Tag | Module |
|---|---|---|
| `CH_COM` | `COM ` | Comms |
| `CH_INP` | `INP ` | Input parser, buttons, watchdog |
| `CH_MOT` | `MOT ` | MotorControl, MotorHardware |
| `CH_RMP` | `RMP ` | MotorRamp |
| `CH_PID` | `PID ` | MotorPID, Encoder |
| `CH_SNR` | `SNR ` | Ultrasonic, ObstacleDetection, Battery |
| `CH_SAF` | `SAF ` | SafetyManager, MotionPolicy, MotorFault |
| `CH_WDG` | `WDG ` | InputWatchdog |

---

## Naming

- Prefix every channel value with `CH_`. The preprocessor replaces bare names before the compiler applies `enum class` scoping.
- Arduino core reserves: `INPUT`, `OUTPUT`, `HIGH`, `LOW`, `B0`–`B15`, `LED_BUILTIN`, `PI`, `min`, `max`, `abs`.
- ESP-IDF reserves: `NETWORK`, `DISPLAY`, `WIFI_*`.
- The `CH_` prefix avoids all of them.
- Tags are four characters, uppercase, space-padded. No tag needs five.
- One tag per module. Do not add a second tag for a sub-part. Use the message to name the sub-part.

---

## API

- `Log::init()` — set default masks. Call once, from `setup()`.
- `Log::reset()` — clear both masks. Use from tests.
- `Log::enabled(Lvl, Ch)` — query a level and a channel together. Used by the macros.
- `Log::isLevelEnabled(Lvl)` — query the level mask only.
- `Log::isChannelEnabled(Ch)` — query the channel mask only.
- `Log::setLevel(Lvl, bool)` — enable or disable one level.
- `Log::setChannel(Ch, bool)` — enable or disable one channel.
- `Log::setAllLevels(bool)` — enable or disable all levels.
- `Log::setAllChannels(bool)` — enable or disable all channels.
- `Log::dump()` — print both masks and per-entry state.
- `Log::dumpShort()` — print enabled levels and channels, short form.
- `Log::write(Lvl, Ch, fmt, ...)` — formatted write. Not for direct use. Go through a macro.

---

## Macros

```
LOG_D(ch, fmt, ...)
LOG_I(ch, fmt, ...)
LOG_W(ch, fmt, ...)
LOG_E(ch, fmt, ...)
```

- Each macro expands to a scoped check plus `Log::write`.
- When `LOG_ENABLED` is 0, each macro expands to `do {} while (0)`. Arguments are not evaluated.
- `fmt` and `...` follow `printf` rules.
- Buffer is 128 bytes. Longer output is truncated.

---

## Defaults

`Log::init()` sets:

- Level mask: `I`, `W`, `E` on. `D` off.
- Channel mask: all channels on.

`Log::reset()` sets:

- Level mask: 0.
- Channel mask: 0.

---

## Call Sites

- One log call per logical event. Do not split one event across two calls.
- Pick the level that matches the reader, not the writer. A recoverable problem is `W`, not `E`. An expected event is `I`, not `D`.
- Write the message so the tag prefix is not repeated in the text.
  - Correct: `LOG_E(Log::Ch::CH_NET, "Config TX failed: %d", err);`
  - Wrong: `LOG_E(Log::Ch::CH_NET, "[NET] Config TX failed: %d", err);`
- Keep the format string under 60 characters. Details belong in the arguments.
- Never log inside a hot path without a rate limit or a counter.
- Never log a pointer, an address, or memory contents. Use a dedicated channel if needed.
- Never log inside ISR context.

---

## Behavior

- `init()` sets the default masks. Info and above are on. All channels are on.
- `reset()` clears both masks. Nothing prints until a mask is set.
- Enabling a level or a channel takes effect on the next macro call. No re-init needed.
- `write()` emits one line: level bracket, channel bracket, space, formatted text, newline.
- `dump()` prints both masks in hex, then one line per level and per channel with `ON` or `OFF`.
- The module holds no buffers between calls. State is the two masks.

---

## Migration from `Debug`

`Debug_Standard.md` is superseded. Use this table to move a call site.

| Old | New |
|---|---|
| `#include "debug/debug.h"` | `#include "log/log.h"` |
| `Debug::init()` | `Log::init()` |
| `Log::Ch::CH_NETWORK` | `Log::Ch::CH_NET` |
| `Log::Ch::CH_WEATHER` | `Log::Ch::CH_WEA` |
| `Log::Ch::CH_SENSORS` | `Log::Ch::CH_SNR` |
| `Log::Ch::CH_DISPLAY` | `Log::Ch::CH_DSP` |
| `LOG_D(Log::Ch::CH_X, ...)` | `LOG_D(Log::Ch::CH_X, ...)` |
| `Serial.printf("[NET] ...")` | `LOG_I(Log::Ch::CH_NET, "...")` |
| `Serial.println("[MAIN] ...")` | `LOG_I(Log::Ch::CH_SYS, "...")` |

Map `DBG_PRINT` to `LOG_D` by default. Move a call to `LOG_I`, `LOG_W`, or `LOG_E` when the event is not developer detail.

