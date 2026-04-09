# AmbiSense Architecture

## Overview

AmbiSense is a dual-device system for real-time ambient weather and room sensor display:
- **Hub** (ESP32): Fetches live weather via Wi-Fi, aggregates room sensors, broadcasts via ESP-NOW
- **Display** (ESP32-2432S028 CYD): Receives broadcast, renders LVGL dashboard with theme/format options

Both devices independent; can develop/test separately. Communication via ESP-NOW binary protocol (no pairing needed).

---

## Hub Architecture

### Modules

**Network** (`include/Network.h` / `src/Network.cpp`)
- Manages Wi-Fi connection and ESP-NOW broadcast
- Loads/saves credentials from NVS (Preferences API, namespace `"ambisense"`)
- Receives ConfigPacket from Display to update Wi-Fi/NTP at runtime
- Builds and broadcasts DataPacket every 250ms (even if offline)
- Handles ESP-NOW RX callbacks (CONFIG, CMD, ACK packets)
- Periodic reconnect attempt if WiFi drops (30s interval)

**Weather** (`include/Weather.h` / `src/Weather.cpp`)
- Fetches current weather from API (Open-Meteo free, no key)
- Handles retries with exponential backoff (initial: 2s, max: 3 retries)
- Caches weather data; refreshes on 30-minute interval
- Returns WeatherData struct (temp, humidity, wind, pressure, sunrise/sunset, WMO code)

**Sensors** (`include/Sensors.h` / `src/Sensors.cpp`)
- Polls DHT22 every 2 seconds (non-blocking)
- Reads temperature, humidity
- Caches last valid reading; marks invalid if read fails

**RTCManager** (`include/RTCManager.h` / `src/RTCManager.cpp`)
- Interfaces with DS3231 real-time clock
- Provides accurate timestamp independent of WiFi
- Syncs with NTP once WiFi+internet available
- Tracks NTP sync state (IDLE, SYNCING, DONE, FAILED)
- Source of truth for all timestamps in DataPacket

**Main Loop** (`src/main.cpp`)
1. Call `Network::update()` — maintains WiFi, broadcasts every 250ms
2. Call `Weather::update()` — fetches if stale (30 min default)
3. Call `Sensors::update()` — polls if ready (2s interval)
4. Periodic: `RTCManager::update()` — NTP sync status check

### Data Flow (Hub)

```
Weather API (Open-Meteo)
    ↓
Weather module (cached, ~30min refresh)
    ↓
DHT22 sensor
    ↓
DS3231 RTC
    ↓
DataPacket assembly in Network::_buildDataPacket()
├── Weather (temp, humidity, wind, pressure, sunrise/sunset, WMO code)
├── Room sensors (temp, humidity from DHT22)
├── RTC timestamp (from DS3231)
└── Status flags (WiFi connected, data valid, channel, seq)
    ↓
ESP-NOW broadcast (250ms interval, broadcast address)
    ↓
Display RX
```

### Packet Structure

All packets in `include/config/Config.h` (shared):

**DataPacket** (Hub → Display, broadcast every 250ms)
```cpp
struct DataPacket {
    uint8_t  type;            // PACKET_TYPE_DATA (0x01)
    uint8_t  seq;             // Rolling sequence (0–255)
    uint8_t  channel;         // Wi-Fi channel (for Display auto-sync)
    uint8_t  wifiConnected;   // 1 = Hub on Wi-Fi, 0 = offline
    uint32_t timestamp;       // Unix timestamp from RTC
    
    // Weather
    uint8_t  weatherValid;
    int      weatherCode;     // WMO code
    float    outsideTemp;     // °C
    float    apparentTemp;    // "feels like"
    int      outsideHumi;     // %
    int      outsidePressure; // hPa
    float    windSpeed;       // m/s
    int      windDir;         // degrees (0–360)
    char     sunrise[6];      // HH:MM
    char     sunset[6];       // HH:MM
    
    // Room
    uint8_t  roomValid;
    float    roomTemp;        // °C
    float    roomHumi;        // %
};
```

**ConfigPacket** (Display → Hub, on-demand)
- SSID (32 bytes), password (63 bytes), NTP server (63 bytes), seq

**CmdPacket** (Display → Hub, on-demand)
- Command ID (e.g., CMD_FORCE_NTP_SYNC), seq

**AckPacket** (bidirectional)
- Echoes seq of received packet

---

## Display Architecture

### Modules

**DisplayManager** (`include/DisplayManager.h` / `src/DisplayManager.cpp`)
- LVGL initialization and lifecycle
- SDL2 simulator support (for local development)
- Frame rate control

**Network** (`include/Network.h` / `src/Network.cpp`)
- ESP-NOW RX callback
- Parses DataPacket from Hub
- Channel auto-sync (locks to Hub's Wi-Fi channel on first packet)
- Offline detection (5s timeout → red status dot)
- Sends ConfigPacket to update credentials
- Loads/saves credentials from NVS (Preferences API, namespace `"ambisense_disp"`)

**UI** (`include/UI.h` / `src/UI.cpp`)
- Renders LVGL dashboard (320×240 IPS TFT)
- Two screens: DASHBOARD (weather + room sensors) and CONFIG (Wi-Fi/NTP, theme, format)
- Theme toggle (dark/light with persistent palette)
- Settings: show/hide seconds, date format (TEXT or NUMERIC)
- Non-blocking updates; all rendering deferred to `update()` calls
- Touch input handling (XPT2046)

**WeatherTypes** (`include/WeatherTypes.h` / `src/WeatherTypes.cpp`)
- Shared struct definitions
- WeatherData struct (mirrors Hub's weather fields)

**Main Loop** (`src/main.cpp`)
1. LVGL poll (`lv_timer_handler()`, ~60 FPS)
2. If new DataPacket received, call `UI::update()`
3. Check XPT2046 touch input
4. If >5s no packet, trigger offline indicator

### Data Flow (Display)

```
ESP-NOW RX (broadcast from Hub)
    ↓
DataPacket received in Network RX callback
    ↓
Network stores packet, sets _lastPacketMs
    ↓
UI::update() called by main loop
├── Format time/date
├── Display weather (icon, temp, condition, pressure, wind)
├── Display room sensors (temp, humidity)
└── Update status dot (green=online, red if >5s timeout)
    ↓
LVGL render queue
    ↓
LCD (320×240 IPS)
```

### UI Screens

**Dashboard**
```
┌────────────────────────────────────┐
│ Weather Icon  Temp°C  [Settings]   │  Header
│ Condition, Feels Like              │
│ Humidity | Pressure | Wind         │
│ Sunrise | Sunset                   │
│                                    │
│ Room Sensors                       │
│ 🌡 Temp°C  💧 Humidity%           │
│                                    │
│ Time HH:MM[:SS]                    │
│ Date (12 Mar 2026 or 12/03/2026)   │
│                              [●]   │  Status dot
└────────────────────────────────────┘
```

**Config Screen** (2 tabs)
- Tab 1: Wi-Fi/NTP — Edit SSID, password, NTP server; force sync
- Tab 2: Settings — Toggle dark theme, show seconds, date format

### Configuration (Display)

Stored in NVS under namespace `"ambisense_disp"`:
- `ssid` — Saved Hub SSID
- `pass` — Saved Hub password
- `ntp` — Saved NTP server
- `darkTheme` — bool
- `showSeconds` — bool
- `dateFmtText` — bool (true = TEXT, false = NUMERIC)

All saved automatically on change (NVS persistence).

### UI State

```cpp
bool      _darkTheme;           // Dark or light palette
bool      _showSeconds;         // Show seconds in time
DateFormat _dateFmt;            // TEXT or NUMERIC
uint32_t  _lastPacketMs;        // Timestamp of last received packet (offline detection)
Palette*  theme;                // Current palette pointer (DARK or LIGHT)
```

---

## Communication Protocol

### Packet Types (from Config.h)

- `PACKET_TYPE_DATA` (0x01) — Hub broadcasts every 250ms
- `PACKET_TYPE_CONFIG` (0x02) — Display sends credential update
- `PACKET_TYPE_ACK` (0x03) — Either device echoes seq
- `PACKET_TYPE_CMD` (0x04) — Display sends commands (e.g., force NTP sync)

### ESP-NOW Details

- Broadcast address: `{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}`
- No encryption
- Display auto-locks to Hub's Wi-Fi channel on first packet
- Works even if Hub has no internet
- Typical latency <50ms

### Display → Hub Flow (Config Update)

1. User edits SSID/password in Display config screen
2. Display sends ConfigPacket with new credentials
3. Hub receives, calls `saveConfig()` to update NVS
4. Hub immediately calls `connectWiFi()`
5. Hub sends AckPacket back

---

## Design Patterns

### Non-Blocking Architecture

- No `delay()` anywhere
- All timing based on `millis()` or callbacks
- Sensors/network/UI all check-on-demand

### Graceful Offline Fallback

- Display shows last known values if Hub goes offline
- 5-second timeout before "offline" indicator (red dot)
- No UI freeze or hang
- Syncs automatically when Hub comes back

### NVS Storage

- Hub: credentials + system config in `"ambisense"` namespace
- Display: UI prefs + saved credentials in `"ambisense_disp"` namespace
- Loaded once on boot, updated on config change

### Channel Auto-Sync

- Display locks to Hub's Wi-Fi channel on first DataPacket
- Simplifies ESP-NOW discovery (no scanning)
- Survives Wi-Fi channel changes

### Broadcast-Only ESP-NOW

- Simpler than point-to-point (no pairing)
- Hub sends continuously; Display receives
- Works even if Display hasn't been introduced to Hub yet

---

## Configuration

**Shared settings** (`include/config/Config.h`):
- Timezone offset: `GMT_OFFSET_SEC` (UTC+7 for Bangkok, 0 for UTC)
- Daylight offset: `DAYLIGHT_OFFSET_SEC` (0 for no DST)
- Weather refresh: `WEATHER_INTERVAL_MS` (30 minutes)
- Broadcast rate: `BROADCAST_INTERVAL_MS` (250ms)
- DHT polling: `DHT_INTERVAL_MS` (2 seconds)
- NTP server: Loaded from NVS or hardcoded default

**Hub only**:
- API key (if using OpenWeatherMap instead of Open-Meteo)

**Display only**:
- Theme palette colors (hardcoded, edit if customizing UI)

---

## Performance

| Metric | Value |
|--------|-------|
| ESP-NOW latency | <50ms |
| DataPacket broadcast | 250ms (4 Hz) |
| DHT22 polling | 2 seconds |
| Weather refresh | ~30 minutes |
| Display refresh | ~60 FPS |
| Power (Hub) | ~1W idle, ~1.5W active |
| Power (Display) | ~0.8W idle, ~1.2W active |
| Total system | ~2W continuous |

---

## Troubleshooting

**Display shows offline (red dot)**
- Hub may not be broadcasting
- Check Hub serial output for errors
- Verify both on same Wi-Fi channel (for ESP-NOW)
- Hard reset both devices if stuck

**Weather not updating**
- Check Hub's serial output for API errors
- Verify API connectivity (Wi-Fi should show connected)
- Ensure NTP sync completed

**Config changes not persisting**
- NVS save may fail due to flash wear
- Try erasing both devices' flash and reflashing
- Check serial output for NVS errors

**Time incorrect**
- Verify Hub's NTP sync (check serial)
- Ensure NTP_SERVER is reachable
- Check timezone offset in Config.h

**Display won't connect to Hub**
- Hard reset both; power cycle and reflash if needed
- Verify both on same Wi-Fi network (for channel alignment)
- Check ESP-NOW initialization in serial output

---

## File Organization

```
hub/
├── src/
│   ├── main.cpp
│   ├── Network.cpp
│   ├── Weather.cpp
│   ├── Sensors.cpp
│   └── RTCManager.cpp
├── include/
│   ├── Network.h
│   ├── Weather.h
│   ├── Sensors.h
│   ├── RTCManager.h
│   └── config/
│       └── Config.h (shared)
└── docs/
    ├── Code_Layout_Standard.md
    └── AmbiSense_Protocol.md

display/
├── src/
│   ├── main.cpp
│   ├── DisplayManager.cpp
│   ├── Network.cpp
│   ├── UI.cpp
│   └── WeatherTypes.cpp
├── include/
│   ├── DisplayManager.h
│   ├── Network.h
│   ├── UI.h
│   ├── WeatherTypes.h
│   ├── fonts/
│   └── config/
│       └── Config.h (shared)
└── docs/
    ├── Code_Layout_Standard.md
    └── AmbiSense_Protocol.md
```
