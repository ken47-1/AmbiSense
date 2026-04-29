# AmbiSense Architecture

## Overview

AmbiSense is a dual-device local-first weather and room sensor display system:
- **Hub** (ESP32 WROOM-32): Fetches live weather via Wi-Fi, aggregates room sensors, broadcasts via ESP-NOW
- **Display** (ESP32-2432S028 CYD): Receives broadcast, renders LVGL dashboard, allows user config

Both devices are independent; can develop/test separately. Communication via ESP-NOW (no router required).

---

## Hub Architecture

### Hardware
- ESP32 (WROOM-32)
- DHT22 (temperature/humidity sensor)
- DS3231 (real-time clock, accurate timekeeping)
- Wi-Fi antenna
- USB power or 5V battery

### Modules

**Network** (`include/Network.h` / `src/Network.cpp`)
- Manages Wi-Fi connection and ESP-NOW broadcast
- Loads/saves credentials from NVS (Preferences API, namespace `"ambisense"`)
- Receives ConfigPacket from Display to update Wi-Fi/NTP at runtime
- Receives CmdPacket for commands (e.g., force NTP sync)
- Builds and broadcasts DataPacket every 250ms (even if offline)
- Handles ESP-NOW RX callbacks (CONFIG, CMD, ACK packets)
- Periodic reconnect attempt if WiFi drops (every 30s)

**Weather** (`include/Weather.h` / `src/Weather.cpp`)
- Fetches current weather from Open-Meteo API (free, no API key)
- Handles retries with exponential backoff (initial: 2s, max 3 retries)
- Caches weather data; refreshes on 30-minute interval (`WEATHER_INTERVAL_MS`)
- Returns WeatherData struct with temp, humidity, pressure, wind, sunrise/sunset, WMO code
- Runs in FreeRTOS task (background thread) to avoid blocking
- Thread-safe access via mutex

**Sensors** (`include/Sensors.h` / `src/Sensors.cpp`)
- Polls DHT22 every 2 seconds (`DHT_INTERVAL_MS`)
- Reads temperature, humidity
- Caches last valid reading; marks invalid if read fails
- Validity check: `temp > -50°C` AND `humidity > 0%`

**RTCManager** (`include/RTCManager.h` / `src/RTCManager.cpp`)
- Interfaces with DS3231 real-time clock
- Provides accurate timestamp independent of WiFi
- NTP sync state machine: `IDLE → SYNCING → DONE / FAILED`
- Syncs with NTP once WiFi+internet available
- Daily scheduled sync at configurable time (default 12:00)
- Source of truth for all timestamps in DataPacket

**Main Loop** (`src/main.cpp`)
1. `sensors.update()` — Poll DHT22 if interval elapsed
2. `rtc.update(network.isConnected(), network.getNTPServer())` — Check/perform NTP sync
3. `network.update()` — Maintain WiFi, broadcast DataPacket every 250ms

### Data Flow (Hub)

```
Weather API (Open-Meteo)
    ↓
Weather module (cached, ~30min refresh, FreeRTOS task)
    ↓
DHT22 sensor (polled every 2s)
    ↓
DS3231 RTC (accurate time, NTP-synced)
    ↓
DataPacket assembly in Network::update()
├── Weather (temp, humidity, pressure, wind, sunrise/sunset, WMO code)
├── Room sensors (temp, humidity from DHT22)
├── RTC timestamp (from DS3231)
└── Status flags (WiFi connected, data valid, channel, seq)
    ↓
ESP-NOW broadcast (every 250ms, broadcast address)
    ↓
Display RX
```

### Packet Structure

All packets defined in `include/config/Config.h` (shared):

**DataPacket** (Hub → Display, broadcast every 250ms, ~60 bytes)
```
uint8_t  type              // PACKET_TYPE_DATA (0x01)
uint8_t  seq               // Rolling sequence (0–255)
uint8_t  channel           // Wi-Fi channel (for Display auto-sync)
uint8_t  wifiConnected     // 1 = Hub on Wi-Fi, 0 = offline
uint32_t timestamp         // Unix timestamp from RTC

// Weather
uint8_t  weatherValid
int      weatherCode       // WMO code
float    outsideTemp       // °C
float    apparentTemp      // "feels like"
int      outsideHumi       // %
int      outsidePressure   // hPa
float    windSpeed         // m/s
int      windDir           // degrees (0–360)
char     sunrise[6]        // HH:MM
char     sunset[6]         // HH:MM

// Room
uint8_t  roomValid
float    roomTemp          // °C
float    roomHumi          // %
```

**ConfigPacket** (Display → Hub, on-demand, ~165 bytes)
- SSID (32 bytes), password (63 bytes), NTP server (63 bytes), seq

**CmdPacket** (Display → Hub, on-demand)
- Command ID (e.g., CMD_FORCE_NTP_SYNC), seq

**AckPacket** (bidirectional)
- Echoes seq of received packet

---

## Display Architecture

### Hardware
- ESP32-2432S028 CYD (Cheap Yellow Display)
  - Built-in ESP32
  - 2.8" 320×240 ILI9341 IPS TFT LCD
  - XPT2046 capacitive touch controller
  - SPI interface
- USB power or barrel jack (5V)

### Modules

**DisplayManager** (`include/DisplayManager.h` / `src/DisplayManager.cpp`)
- LVGL initialization and lifecycle
- TFT_eSPI driver for LCD rendering
- XPT2046 touch input handling
- Provides `isTouched()` and `getTouch()` for UI event handling
- Frame rate: ~60 FPS via LVGL

**Network** (`include/Network.h` / `src/Network.cpp`)
- ESP-NOW RX callback
- Parses DataPacket from Hub
- Channel auto-sync: locks to Hub's Wi-Fi channel on first packet
- Offline detection: 5-second timeout triggers status indicator
- Sends ConfigPacket to update credentials
- Sends CmdPacket for commands (e.g., force NTP sync)
- Loads/saves credentials from NVS (Preferences API, namespace `"ambisense"`)

**UI** (`include/UI.h` / `src/UI.cpp`)
- LVGL v9.5 dashboard rendering (320×240 IPS TFT)
- Two screens:
  - **DASHBOARD**: Weather card, room sensors, time/date, status dot
  - **CONFIG**: Two tabs
    - Tab 1: Wi-Fi/NTP input fields, force sync button
    - Tab 2: Dark/light theme toggle, show/hide seconds, date format dropdown
- Dual themes (dark/light) with persistent palettes
- Non-blocking updates; all rendering deferred to `update()` calls
- Touch input callbacks for buttons and text fields
- Palettes: DARK (default) and LIGHT

**WeatherTypes** (`include/WeatherTypes.h` / `src/WeatherTypes.cpp`)
- Shared struct definitions (WeatherData, Palette, etc.)
- Weather enum and helper functions (icon lookup, etc.)

**Main Loop** (`src/main.cpp`)
1. `display.update()` — Handle LVGL, process touch input
2. `network.update()` — Receive DataPackets, detect offline
3. Every 100ms: `ui.update(timestamp, roomTemp, roomHumi, roomValid, weather, hubOnline)` — Refresh dashboard

### Data Flow (Display)

```
ESP-NOW RX (broadcast from Hub, every 250ms)
    ↓
DataPacket received in Network RX callback
    ↓
Network stores packet, sets _lastPacketMs, calls onDataReceived() callback
    ↓
Main loop extracts fields into g_* globals (g_weather, g_roomTemp, etc.)
    ↓
UI::update() called every 100ms with current state
├── Format time/date (HH:MM[:SS], optional seconds)
├── Select weather icon (based on WMO code)
├── Display weather (icon, temp, condition, pressure, wind)
├── Display room sensors (temp, humidity)
├── Update theme colors
└── Calculate status dot (Green=online+weather, Gold=online+no weather, Red=offline)
    ↓
LVGL render queue
    ↓
LCD (320×240 IPS)
```

### UI State

**Palette** struct with theme colors:
- Dark theme (default): black BG, white text, orange/blue/green accents
- Light theme: light gray BG, dark text, adjusted colors

**Screen enum**: DASHBOARD, CONFIG

**Global state** (main.cpp):
- `g_weather` — Current weather data
- `g_roomTemp`, `g_roomHumi`, `g_roomValid` — Room sensors
- `g_timestamp` — Current Unix timestamp
- `g_hubWifi` — Hub's Wi-Fi connection status
- `g_lastPktMs` — Timestamp of last received DataPacket (offline detection)

---

## Communication Protocol

### Packet Types

- **PACKET_TYPE_DATA** (0x01) — Hub broadcasts every 250ms
- **PACKET_TYPE_CONFIG** (0x02) — Display sends credential update
- **PACKET_TYPE_ACK** (0x03) — Either device echoes seq
- **PACKET_TYPE_CMD** (0x04) — Display sends commands (e.g., force NTP sync)

### Commands

- **CMD_FORCE_NTP_SYNC** (0x01) — Hub immediately syncs RTC with NTP

### ESP-NOW Details

- Broadcast address: `{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}`
- No encryption
- Display auto-locks to Hub's Wi-Fi channel on first packet
- Works even if Hub has no internet
- Typical latency <50ms

### Offline Detection

- Display tracks `_lastPacketMs` (timestamp of last received DataPacket)
- If no packet for >5 seconds: `_isConnected = false`
- Triggers `onStatusChanged(false)` callback
- UI displays red status dot and clears weather/room data

---

## Configuration Files

Each project has three config files:

### Shared: `Config.h`
Protocol definitions, intervals, packet types (shared by both Hub and Display)

- `DEBUG_NETWORK = 0` — Log ESP-NOW packets
- **TIME**: `GMT_OFFSET_SEC = 7*3600` (UTC+7, Bangkok), `DAYLIGHT_OFFSET_SEC = 0` (no DST)
- **WEATHER**: `WEATHER_INTERVAL_MS = 1,800,000` (30 min), `WEATHER_RETRY_DELAY_MS = 2000`, `WEATHER_MAX_RETRIES = 3`
- **INTERVALS**: `DHT_INTERVAL_MS = 2000`, `BROADCAST_INTERVAL_MS = 250`, `SCAN_HOP_INTERVAL_MS = 1000`, `SYNC_CHECK_INTERVAL_MS = 1000`
- **ESP-NOW**: Broadcast address, packet types, command IDs
- **PACKET STRUCTS**: DataPacket, ConfigPacket, CmdPacket, AckPacket

### Device-Specific: `HardwareConfig.h`

**Hub** pins:
- DHT22: GPIO 25
- DS3231 I2C: SDA GPIO 33, SCL GPIO 32

**Display** pins:
- SPI: CS GPIO 5, DC GPIO 4, CLK GPIO 18, MOSI GPIO 23, MISO GPIO 19
- Touch SPI: CS GPIO 32, CLK GPIO 25, MOSI GPIO 33, MISO GPIO 27
- Power: GPIO 14 (backlight), GPIO 12 (power)

### User-Editable: `LocationConfig.h`

- `LOCATION_LAT`, `LOCATION_LON` — Coordinates for weather API
- `LOCATION_NAME` — Display string (e.g., "Bangkok")
- `LOCATION_GMT_OFFSET_SEC` — Timezone (default UTC+7)
- `LOCATION_DAYLIGHT_OFFSET_SEC` — DST offset (0 for Thailand)

---

## Design Patterns

### Non-Blocking Architecture

- No `delay()` anywhere
- All timing based on `millis()`
- Sensor reads polled on demand
- Commands processed asynchronously
- Weather fetch runs in FreeRTOS task (doesn't block main loop)

### Graceful Offline Fallback

- Display shows last known values if Hub goes offline
- 5-second timeout before "offline" indicator
- No UI freeze or hang
- Syncs automatically when Hub comes back

### NVS Storage

- Hub: credentials in namespace `"ambisense"`
- Display: credentials in namespace `"ambisense"`
- Loaded once on boot, updated on config change
- Persistent across power cycles

### Channel Auto-Sync

- Display locks to Hub's Wi-Fi channel on first DataPacket
- Simplifies ESP-NOW discovery (no scanning)
- Survives Wi-Fi channel changes

### Broadcast-Only ESP-NOW

- Simpler than point-to-point (no pairing)
- Hub sends continuously; Display receives
- Works even if Display hasn't been introduced to Hub yet

### FreeRTOS Task for Weather

- Weather fetching runs in background task
- Prevents blocking main loop on network latency
- Thread-safe access via mutex

---

## Performance

| Metric | Value |
|--------|-------|
| ESP-NOW latency | <50ms |
| DataPacket broadcast | 250ms (4 Hz) |
| DHT22 polling | 2 seconds |
| Weather refresh | ~30 minutes |
| Display refresh | ~60 FPS (LVGL) |
| UI update | Every 100ms |
| Offline timeout | 5 seconds |
| Power (Hub) | ~1W idle, ~1.5W active |
| Power (Display) | ~0.8W idle, ~1.2W active |
| Total system | ~2W continuous |

---

## Troubleshooting

**Display shows offline (red dot)**
- Hub may not be broadcasting
- Check Hub serial output for errors
- Verify both on same 2.4GHz Wi-Fi band (for channel alignment)
- Hard reset both devices if stuck

**Weather not updating**
- Check Hub: `pio device monitor -b 115200` — look for API errors
- Verify Wi-Fi connection on Hub
- Ensure NTP sync completed (check Hub serial for sync messages)

**Time incorrect**
- Verify Hub's NTP sync (check serial for `NTP sync complete`)
- Ensure NTP server is reachable
- Check timezone in `LocationConfig.h` (default UTC+7)

**Config changes not persisting**
- NVS save may fail due to flash wear
- Try erasing NVS and reflashing: `pio run -t erase`
- Check serial output for NVS errors

**Display won't connect to Hub**
- Verify both on same 2.4GHz Wi-Fi network
- Hard reset both: power cycle and reflash if needed
- Check ESP-NOW init in serial logs (both should print init messages)

---

## Current Status

- Both Hub and Display fully functional
- Clock synchronizes via NTP, sensors read and transmit
- UI renders live with dual themes
- Offline mode preserves last known values indefinitely
- Ready for deployment
