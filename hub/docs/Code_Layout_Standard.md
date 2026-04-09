# Code Layout & Style Standard

This document defines the required structure and conventions for source files in this project.

---

## General Rules

- One logical module per file
- Headers (`.h`) declare the public API only — no implementation, no static variables
- Source files (`.cpp`) contain all implementation and internal state
- Prefer namespaces over free functions
- Classes own hardware; other modules borrow references or pointers, never own hardware
- No hidden global state unless explicitly documented
- Omit a section entirely if it has no content

--- 

## Universal Visual Hierarchy
All files must use the following comment scale to represent the logical tree. Sections are omitted if empty.

| Tier | Visual Style                                           | Usage                                                  |
|------|--------------------------------------------------------|--------------------------------------------------------|
**T1** | `/* ==================== Text ==================== */` | **Root**: Module/Filename                              |
**T2** | `/* =============== Text =============== */`           | **Major Branch**: Primary Divisions (Includes, API)    |
**T3** | `/* ============ Text ============ */`                 | **Branch**: Categories (Project, Core, State)          |
**T4** | `/* ========= Text ========= */`                       | **Sub-Branch**: Logical Groupings (Types, Callbacks)   |
**T5** | `/* ------ Test ------ */`                             | **Stem**: Functional blocks or handlers                |
**T6** | `/* --- Text --- */`                                   | **Twig**: Discrete logic steps                         |
**T7** | `/* Text */`                                           | **Leaf**: Inline logic notes or specific code commands |

---

## Header File Layout (`.h`)

```cpp
/* ==================== ModuleName.h ==================== */
#pragma once

/* =============== INCLUDES =============== */
/* ============ PROJECT ============ */    // Internal headers required by public API
/* ============ THIRD-PARTY ============ */ // External library headers
/* ============ CORE ============ */       // Platform headers (Arduino, etc.)

/* =============== TYPES =============== */
/* ============ ENUMS ============ */
/* ============ STRUCTS ============ */
/* ============ CLASSES ============ */

/* =============== API =============== */
namespace ModuleName {
    /* --------- Public Functions --------- */
}
```

> Only include headers that are directly required by the public API.
> Internal-only includes belong in the `.cpp` file.

---

## Source File Layout (`.cpp`)

```cpp
/* ==================== ModuleName.cpp ==================== */
#include "path/ModuleName.h"

/* =============== INCLUDES =============== */
/* ============ PROJECT ============ */
/* ============ THIRD-PARTY ============ */
/* ============ CORE ============ */

/* =============== INTERNAL STATE =============== */
/* ============ STATIC VARS ============ */
/* ============ SINGLETONS ============ */

/* =============== INTERNAL HELPERS =============== */
/* ============ CALLBACKS ============ */
/* ========= ESP-NOW / WIFI ========= */
/* ------ OnDataRecv ------ */
/* --- Packet Validation --- */

/* ============ LOGIC ============ */
/* ========= PROCESSING ========= */

/* =============== PUBLIC API =============== */
/* ============ LIFECYCLE ============ */
/* ========= begin / update ========= */
```

> Sections must appear in the order shown. Omit any section that has no content.
> All internal helpers must be `static`.

---

## Namespace Rules

- Every non-trivial module lives in a namespace
- Never use `using namespace` in headers
- Always fully qualify cross-module calls (e.g. `Veto::get_state()`)
- Feature-guarded modules must provide a no-op stub class or inline functions in the `#else` branch so call sites compile regardless of the flag

---

## Hardware Ownership Rules

- Hardware objects are owned by exactly one module or class
- Other modules receive references or pointers — never construct hardware inside utility, policy, or logic-only modules

---

## Documentation

- Protocols, standards, and design notes belong in `docs/`
- Do not embed large documentation blocks in headers or source files