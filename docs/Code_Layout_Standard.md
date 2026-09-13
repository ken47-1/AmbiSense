# Code Layout and Style Standard

This document defines the required structure and conventions for source files in this project.

---

## General Rules

- One logical module per file.
- Headers (`.h`) declare the public API only. They contain no implementation and no static variables.
- Source files (`.cpp`) contain all implementation and internal state.
- Use namespaces. Avoid free functions.
- Classes own hardware. Other modules borrow references or pointers. They never own hardware.
- No hidden global state, unless the file documents it.
- Omit a section if it has no content.
- Use braces for multi-line `if`, `for`, `while`, and `do-while` bodies.
- A single-line guard clause can omit braces.

✅ Correct (single-line guard):
```cpp
if (flag_active) return;
```

✅ Correct (multi-line block, braces required):
```cpp
if (flag_active) {
    flag_active = true;
    flag_reason = reason;
    Module::stop();
    return;
}
```

❌ Incorrect (line-broken single statement):
```cpp
if (flag_active)
    return;
```

❌ Incorrect (multi-line body without braces):
```cpp
if (flag_active)
    flag_active = true;
    flag_reason = reason;  // This runs OUTSIDE the if!
```

- For multi-line blocks, pick one brace placement. Use the same placement in every file. The two options are same-line and new-line.
- Put each `Print::print()` or `Print::println()` call on its own line.
- Do not chain print calls on one line.

❌ Incorrect (chained print calls):
```cpp
Output::print.print(">>> "); Output::print.print(to_string(value)); Output::print.println(" <<<");
```

---

## Universal Visual Hierarchy

Every file uses this comment scale to show the logical tree. Omit a section if it has no content.

| Tier | Visual Style                                          | Usage                                                   |
|------|-------------------------------------------------------|---------------------------------------------------------|
| T1   | `/* ==================== Text ==================== */` | Root: module or filename                                |
| T2   | `/* =============== Text =============== */`           | Major branch: primary divisions (includes, API)         |
| T3   | `/* ============ Text ============ */`                 | Branch: categories (project, core, state)               |
| T4   | `/* ========= Text ========= */`                       | Sub-branch: logical groupings (types, callbacks)        |
| T5   | `/* ------ Text ------ */`                             | Stem: functional blocks or handlers                     |
| T6   | `/* --- Text --- */`                                   | Twig: discrete logic steps                              |
| T7   | `/* Text */`                                           | Leaf: inline logic notes or specific code commands      |

---

## Header File Layout (`.h`)

```cpp
/* ==================== module_name.h ==================== */
#pragma once

/* =============== INCLUDES =============== */

/* ============ CONFIG ============ */
#include "config/Config.h"        // Project configuration headers

/* ============ PROJECT ============ */
#include "..."                  // Project headers the public API needs

/* ============ THIRD-PARTY ============ */
#include <...>                    // External library headers

/* ============ CORE ============ */
#include <Arduino.h>              // Platform headers

/* =============== TYPES =============== */
/* ============ ENUMS ============ */
/* ============ STRUCTS ============ */
/* ============ CLASSES ============ */
/* ============ FORWARD DECLS ============ */

/* =============== API =============== */
namespace ModuleName {
    /* --------- Public Functions --------- */
}
```

Rules:

- Only include headers that the public API needs. Internal-only includes belong in the `.cpp` file.
- Put `CONFIG` first under `INCLUDES`. Its position is fixed.
- Put every configuration header under `CONFIG`. A configuration header holds constants, pin maps, protocol layouts, timeouts, or feature flags.
- A forward declaration is a T4 entry under `TYPES`.

---

## Source File Layout (`.cpp`)

```cpp
/* ==================== module_name.cpp ==================== */
#include "path/module_name.h"    // Own header, always first

/* =============== INCLUDES =============== */

/* ============ CONFIG ============ */
#include "config/Config.h"        // First slot. Fixed position.

/* ============ PROJECT ============ */
/* ========= COMMS ========= */
#include "comms/..."              // Bluetooth, serial output

/* ========= INPUT ========= */
#include "input/..."              // Command parsing, watchdog

/* ========= CONTROL ========= */
#include "control/..."            // Motor, ramp, mode, autonomy

/* ========= SAFETY ========= */
#include "safety/..."             // Obstacles, policy, faults

/* ========= SENSORS ========= */
#include "sensors/..."            // Ultrasonic, scan, battery

/* ============ THIRD-PARTY ============ */
#include <...>                    // AFMS V2, UltraPing, etc.

/* ============ CORE ============ */
#include <Arduino.h>              // Platform

/* =============== INTERNAL STATE =============== */
/* ============ STATIC VARS ============ */
/* File-local variables. Never exposed. */

/* ============ SINGLETONS ============ */
/* Long-lived objects this module owns. */

/* =============== INTERNAL HELPERS =============== */
/* ============ CALLBACKS ============ */
/* ISR handlers and network callbacks. */

/* ============ LOGIC ============ */
/* Pure functions. No side effects. */

/* =============== PUBLIC API =============== */
/* ============ LIFECYCLE ============ */
/* init(), begin(), reset(). */

/* ========= begin / update ========= */
/* Main loop entry points. */
```

Rules:

- Sections appear in the order shown. Omit any section that has no content.
- Put `CONFIG` first under `INCLUDES`. Its position is fixed. It is not an exception.
- The module header on line 1 includes every configuration header its public API reads.
- Make every internal helper `static`.
- Omit a group label when its group is empty. Do not leave an empty label.

---

## Namespace Rules

- Every non-trivial module lives in a namespace.
- Never use `using namespace` in headers.
- Always fully qualify cross-module calls. Example: `ModuleName::get_state()`.

---

## Feature Flags and Safe Stubs

A feature flag (`ENABLE_X`) turns a module on or off at compile time. A module behind a flag must not break the build when the flag is off.

Rules:

1. In the header, wrap the real API with `#if ENABLE_X`.
2. In the `#else` branch, supply a **safe stub**. A safe stub returns a value that makes the caller inert, or that matches the caller's intent.
3. A safe stub is not a no-op. A no-op does nothing. A safe stub returns a value that keeps the caller safe.

Examples:

```cpp
#if ENABLE_DIRECTIONAL_SCAN
namespace DirectionalScan {
    void reset();
    bool sweep_ready();
    SweepResult get_sweep_result();
}
#else
namespace DirectionalScan {
    inline void reset() {}
    inline bool sweep_ready() { return false; }
    inline SweepResult get_sweep_result() { return {}; }
}
#endif
```

- A function that returns `void` uses an empty body.
- A function that returns a value returns the value that makes the caller inert. For a distance, return a large value such as `999`. For a boolean gate, return `false`. For a battery voltage, return the nominal maximum, never `0.0f`.
- Do not return `0` or `false` for a value that a caller compares against a critical threshold. That causes a false fault.
- A stub only removes the guard when the stub value makes the code inert. If the stub value changes a decision, keep the guard at the call site.

---

## Hardware Ownership Rules

- Hardware objects are owned by exactly one module or class.
- Other modules receive references or pointers. They never construct hardware inside utility, policy, or logic-only modules.

---

## Documentation

- Protocols, standards, and design notes belong in `docs/`.
- Do not embed large documentation blocks in headers or source files.