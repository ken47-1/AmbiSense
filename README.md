# AmbiSense

![AmbiSense Display and Hub](https://github.com/ken47-1/AmbiSense/blob/main/IMG_20260501_231704.jpg?raw=true)

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
[NET] ESP-NOW ready
[NET] Connected. IP=192.168.x.x
[GEO] Location: Bangkok
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
[UI] Dashboard built.
[NET] ESP-NOW ready
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

Tap **⚙️** (top-right) to configure:

**Wi-Fi/NTP Tab**
- Update Hub Wi-Fi credentials
- Set NTP server (default: `asia.pool.ntp.org`)
- Force NTP sync (corrects RTC time)

**Settings Tab**
- Dark/Light theme (saved)
- Show/Hide seconds (saved)
- Date format: Text (12 Mar 2026) or Numeric (12/03/2026) (saved)

All changes persist across power cycles.

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
constexpr float LOCATION_LAT = 13.736717;   // Your latitude
constexpr float LOCATION_LON = 100.523186;  // Your longitude
```

Hub automatically resolves city name via OpenStreetMap Nominatim (no API key).

### Display (`display/include/config/HardwareConfig.h`)

- TFT SPI pins (CS, DC, CLK, MOSI, MISO)
- Touch SPI pins (CS, CLK, MOSI, MISO)
- Backlight GPIO (14)

## Architecture

**Hub Modules**
- `Network` — Wi-Fi, ESP-NOW broadcast, NVS config
- `Weather` — Open-Meteo API (FreeRTOS task, 30min refresh)
- `LocationResolver` — Nominatim reverse geocoding (once at boot)
- `Sensors` — DHT22 polling (2s)
- `RTCManager` — DS3231 + NTP sync

**Display Modules**
- `Network` — ESP-NOW RX, channel sync, offline detection
- `UI` — LVGL dashboard with auto-centering, dual themes
- `DisplayManager` — LVGL init, touch handling, frame control

**Auto-Centering**
Every UI row recalculates position on each update using actual text widths and empirically-determined gaps (measured in Paint). Ensures perfect centering even when values change length (e.g., "25.0°C" → "26.3°C").

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
| City shows "Unknown" | Check LOCATION_LAT/LON; Hub needs internet at boot |
| UI elements misaligned | Auto-centering handles this; update gap values if needed |
| Config not saving | NVS may need erase: `pio run -t erase` |

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

Credentials load from NVS at runtime — no hardcoding needed after first config via Display settings.

## Links

- [LVGL Documentation](https://docs.lvgl.io/)
- [ESP-NOW Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html)
- [Open-Meteo API](https://open-meteo.com/)
- [Nominatim Geocoding](https://nominatim.openstreetmap.org/)

## Credits

Developed with human oversight and AI-assisted code generation.