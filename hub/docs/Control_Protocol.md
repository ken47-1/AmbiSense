# Control Protocol Reference
App ⇄ Robot Communication Contract

> Documents ALL valid commands and feedback frames — no logic.
> Update this file when the app or firmware protocol changes.

---

## Command Format

```
[Command]           Discrete / system (no prefix)
[Prefix][Command]   Prefixed command
```

| Prefix | Domain |
|--------|--------|
| (none) | Movement / system |
| `%`    | Speed / step mode |
| `@`    | Joystick input |

---

## Commands (App → Robot)

### Movement
Single-character, no prefix.

| Command | Action |
|---------|--------|
| `W` | Forward |
| `S` | Backward |
| `A` | Strafe left |
| `D` | Strafe right |
| `Q` | Forward-left |
| `E` | Forward-right |
| `Z` | Backward-left |
| `C` | Backward-right |
| `J` | Spin left (CCW) |
| `L` | Spin right (CW) |

---

### Joystick Input (`@` prefix)

Format: `@<id>X<xval>Y<yval>;`

| Field | Description |
|-------|-------------|
| `<id>` | Numeric joystick ID (`1`, `2`, ...) |
| `X`, `Y` | Signed integers in `[-JOYSTICK_INPUT_MAX, +JOYSTICK_INPUT_MAX]` |

Joystick roles:

| ID | X | Y |
|----|---|---|
| 1 | Strafe | Forward / Backward |
| 2 | Rotation | Reserved |

- Input is stateless — distance from center scales to current speed setting
- Multiple packets may be combined in one loop
- Joystick intent is additive with button input

---

### Speed Control (`%` prefix)

| Command | Action |
|---------|--------|
| `%+` | Increase speed |
| `%-` | Decrease speed |
| `%F` | Fine step mode |
| `%N` | Normal step mode |
| `%R` | Rough step mode |

---

### System

| Command | Action |
|---------|--------|
| `X` | Soft stop (non-latching) |
| `!` | Emergency stop (latching fault) |
| `?` | Reset fault |
| `1` | Autonomous mode ON |
| `0`  | Autonomous mode OFF |

---

## Feedback (Robot → App)

| Frame | Description |
|-------|-------------|
| `*G[value]*` | Speed gauge — value is `0–1000` |
| `*%[mode]*` | Step mode — `Fine`, `Normal`, or `Rough` |

---

## Notes

- All commands are ASCII
- Emergency stop overrides all motion and latches until reset with `?`
- Soft stop (`X`) does not latch — also sent as idle heartbeat to feed watchdog
- Watchdog asserts input loss if no valid command arrives within timeout
- Autonomous mode ON (`1`) and OFF (`0`) are stateless — safe to resend
- Autonomous mode exits immediately on any manual input