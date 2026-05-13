# AmbiSense

![AmbiSense Display](https://github.com/ken47-1/AmbiSense/blob/main/IMG_20260514_014947.jpg?raw=true)
![AmbiSense Hub](https://github.com/ken47-1/AmbiSense/blob/main/IMG_20260514_015018.jpg?raw=true)

Real-time ambient weather and room sensor display. ESP32 Hub fetches live weather via Wi-Fi, resolves city name from GPS coordinates, and broadcasts to ESP32-2432S028 Display over ESP-NOW. Dashboard shows temperature, humidity, pressure, wind, sunrise/sunset, plus local room metrics from DHT22. Theme toggle, optional seconds display, configurable date format. Auto-centering UI ensures perfect alignment even when values change length.

## Hardware

### Hub
- ESP32 (WROOM-32)
- DHT22 (temperature/humidity)
- DS3231 (real-time clock)
- Wi-Fi antenna
- USB power or battery

### Display
- ESP32-2432S028 CYD (320×240 IPS TFT, capacitive touch)
- USB power or barrel jack (5V)

## Quick Start

### 1. Flash Hub

```bash
cd hub/
pio run -t upload
pio device monitor -b 115200
```

Expected output:
```
[MAIN] AmbiSense Hub booting...
[RTC] DS3231 INIT
[GEO] LocationResolver INIT
[NET] ESP-NOW INIT
[NET] Config loaded: SSID: YOUR_SSID_HERE NTP: asia.pool.ntp.org
[NET] Connecting to YOUR_SSID_HERE...
[MAIN] Boot complete.

[NET] Connected to IP: 192.168.x.xxx
[RTC] NTP sync starting...
[RTC] NTP sync done (UTC): 2026-05-14 01:52:11
[RTC] Local time: 01:52:11
[GEO] Location resolved: New York
[WEATHER] HTTP code: 200 (attempt 1/3)
[WEATHER] Data fetched successfully
```

**Note:** First boot uses placeholder credentials (`YOUR_SSID_HERE`). Configure real credentials via Display settings screen.

### 2. Flash Display

```bash
cd display/
pio run -t upload
pio device monitor -b 115200
```

Expected output:
```
[MAIN] AmbiSense Display booting...
[DISP] DisplayManager initialized.
[NET] Scanning on ALL Channels...
[NET] ESP-NOW INIT
[UI] Dashboard built.
[UI] Initialized.
[MAIN] Boot complete.
[NET] LOCKED to Hub Channel: 4
[NET] Connection Restored in 3180ms on Channel 4
[NET] Hub MAC learned
```

Both auto-discover over ESP-NOW. Display locks to Hub's Wi-Fi channel automatically.

## Usage

### Dashboard

Real-time display shows:
- Outdoor weather (temperature, condition, feels like)
- Humidity and pressure
- Wind speed and direction
- Sunrise/sunset times
- Room temperature and humidity (from DHT22)
- Digital clock with date/day

Bottom-right status dot indicates connection:
- **Green** — Hub online, weather valid
- **Gold** — Hub online, weather stale (no internet)
- **Red** — Hub offline (timestamp invalid)

### Settings

Tap **⚙️** (bottom-right) to configure:

**Wi-Fi/NTP Tab**
- Update Hub Wi-Fi credentials
- Set NTP server (default: `pool.ntp.org`)
- Force NTP sync (corrects RTC time)

**Settings Tab**
- Dark/Light theme (saved, persists across reboots)
- Show/Hide seconds (saved)
- Date format: Text (07 Mar 2024) or Numeric (07/03/2024) (saved)

All changes persist across power cycles.

## UI Features

### TabView Configuration Screen
- Clean tab bar with active/inactive color states (gray indicator line removed)
- Horizontal swipe disabled to prevent accidental tab switching
- Vertical scrolling preserved for content overflow
- Keyboard popup with dynamic bottom padding — text fields automatically scroll into view

### Theme Switching
- Dark/Light themes with full UI reconstruction
- Active tab restored after theme change
- Synchronized with LVGL engine defaults for textareas and switches
- Asynchronous screen deletion prevents crash on rebuild

### Dashboard Auto-Centering
Every UI row recalculates position on each update using actual text widths and empirically-determined gaps (measured in Paint). Ensures perfect centering even when values change length (e.g., "25.0°C" → "26.3°C").

## Offline Behavior

Hub offline detection uses packet timestamp:
- Display shows latest data if timestamp valid
- Shows placeholders (`--°C`, `Unknown`) if timestamp invalid
- Status dot turns red immediately
- No data loss; recovers automatically when Hub returns

## Configuration

### Shared (`include/config/Config.h` in both projects)

| Setting | Value | Description |
|---------|-------|-------------|
| `GMT_OFFSET_SEC` | 7*3600 | UTC+7 (Bangkok) |
| `DAYLIGHT_OFFSET_SEC` | 0 | No DST |
| `WEATHER_INTERVAL_MS` | 1,800,000 | 30 minutes |
| `BROADCAST_INTERVAL_MS` | 250 | 4Hz ESP-NOW |
| `DHT_INTERVAL_MS` | 2000 | 2 seconds |

### Hub (`hub/include/config/LocationConfig.h`)

```cpp
constexpr float LOCATION_LAT = 40.6972846;   // Your latitude
constexpr float LOCATION_LON = -74.1443122;  // Your longitude
```

Hub automatically resolves city name via OpenStreetMap Nominatim (no API key). Retry logic with configurable interval and max attempts.

### Display (`display/include/config/HardwareConfig.h`)

- TFT SPI pins (CS, DC, CLK, MOSI, MISO)
- Touch SPI pins (CS, CLK, MOSI, MISO)
- Backlight GPIO (14)

## Architecture

**Hub Modules**
- `Network` — Non-blocking WiFi state machine, ESP-NOW broadcast, NVS config
- `Weather` — Open-Meteo API (FreeRTOS task, 30min refresh)
- `LocationResolver` — Nominatim reverse geocoding with retry logic (non-blocking)
- `Sensors` — DHT22 polling (2s)
- `RTCManager` — DS3231 + NTP sync (UTC storage, gmtime)

**Display Modules**
- `Network` — ESP-NOW RX, channel sync, offline detection
- `UI` — LVGL dashboard with auto-centering, TabView config screen, dual themes
- `DisplayManager` — LVGL init, touch handling, frame control

**Packet Structure** (`DataPacket`, ~100 bytes)
- Location: city name, validity flag
- Weather: temp, humidity, pressure, wind, sunrise/sunset, WMO code
- Room: temp, humidity, validity flag
- System: timestamp, sequence, channel, WiFi status

See `ARCHITECTURE.md` for complete protocol details.

## Troubleshooting

| Symptom | Likely Fix |
|---------|-------------|
| Display shows offline (red dot) | Check Hub power and serial output; both on same 2.4GHz band |
| Weather not updating | Verify Hub Wi-Fi; check Open-Meteo API in serial logs |
| Time incorrect | Hub needs NTP sync (check serial); verify timezone in Config.h |
| City shows "Unknown" | Check LOCATION_LAT/LON; Hub needs internet at boot; wait for retry |
| UI elements misaligned | Auto-centering handles this; update gap values if needed |
| Config not saving | NVS may need erase: `pio run -t erase` |
| Keyboard covers text fields | Dynamic padding should handle this; if not, check `_onTaEvent` callback |
| Theme switch crashes | Should be fixed with async deletion; update if still occurs |

## Performance

| Metric | Value |
|--------|-------|
| ESP-NOW latency | <50ms |
| Broadcast rate | 250ms (4Hz) |
| Sensor polling | 2 seconds |
| Weather refresh | 30 minutes |
| Display FPS | ~60 |
| Power (total) | ~2W |

## Hardware Notes

- **Display**: 320×240 IPS, capacitive touch (XPT2046), SPI interface
- **RTC**: DS3231 ±2ppm (~5 seconds/year drift without NTP)
- **DHT22 Range**: -40°C to +80°C, ±2°C accuracy
- **Weather API**: Open-Meteo (free, no key) — edit `Weather.cpp` for OpenWeatherMap

## Customization

**Timezone**: Edit `GMT_OFFSET_SEC` in `Config.h`
**Weather interval**: Change `WEATHER_INTERVAL_MS`
**Theme colors**: Modify `DARK` / `LIGHT` palettes in `UI.cpp`
**Location**: Update `LOCATION_LAT` / `LOCATION_LON` in `LocationConfig.h`
**TabView height**: Change second parameter in `lv_tabview_create(..., 40)` (currently 40px)

Credentials load from NVS at runtime — no hardcoding needed after first config via Display settings.

## Links

- [LVGL Documentation](https://docs.lvgl.io/)
- [ESP-NOW Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html)
- [Open-Meteo API](https://open-meteo.com/)
- [Nominatim Geocoding](https://nominatim.openstreetmap.org/)

## Credits

Developed with human oversight and AI-assisted code generation.