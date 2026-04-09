# AmbiSense

Real-time ambient weather and room sensor display. ESP32 Hub fetches live weather via Wi-Fi, broadcasts to ESP32-2432S028 Display over ESP-NOW. Dashboard shows temperature, humidity, pressure, wind, sunrise/sunset, plus local room metrics from DHT22. Theme toggle, optional seconds display, configurable date format. Built for offline resilience—Display keeps showing last known data if Hub goes down.

## Hardware

### Hub
- ESP32 (WROOM-32)
- DHT22 (temperature/humidity)
- DS3231 (real-time clock)
- Wi-Fi antenna
- USB power or 5V battery

### Display
- ESP32-2432S028 CYD (320×240 IPS TFT, built-in capacitive touch, XPT2046 controller)
- USB power or barrel jack (5V)

## Quick Start

### 1. Flash Hub

```bash
cd hub/
pio run -t upload
pio device monitor -b 115200
```

Look for:
```
[NET] ESP-NOW ready
[NET] Connecting to your_ssid...
[NET] Connected. IP=192.168.x.x
```

**Note:** On first boot, Hub uses default placeholder credentials (`YOUR_SSID_HERE` / `YOUR_PASSWORD_HERE`). Configure real credentials via the Display settings screen (Wi-Fi/NTP tab) to connect to your network.

### 2. Flash Display

```bash
cd display/
pio run -t upload
pio device monitor -b 115200
```

Look for:
```
[DISP] LVGL initialized
[NET] ESP-NOW ready
```

Both auto-discover over ESP-NOW (no pairing). Display syncs to Hub's Wi-Fi channel and should show weather within 1 second.

## Usage

### Dashboard

The display shows real-time weather with temperature, humidity, pressure, wind speed, sunrise/sunset times, and local room sensor data. A status dot in the bottom-right corner indicates connection status:
- **Green** — Hub is online and broadcasting
- **Red** — No packets received for >5 seconds

### Settings

Tap the **Settings** button (top-right) to open the config screen with two tabs:

**Tab 1: Wi-Fi/NTP**
- Update Hub's Wi-Fi SSID and password
- Set NTP server (default: `pool.ntp.org`)
- Force immediate NTP sync (useful if Hub time is wrong)

**Tab 2: Settings**
- Toggle dark/light theme (saved)
- Show/hide seconds in time display (saved)
- Date format: text (12 Mar 2026) or numeric (12/03/2026) (saved)

Changes are saved to NVS automatically.

## Configuration

**Shared settings** (`include/config/Config.h` in both projects):
- Timezone: `GMT_OFFSET_SEC` (currently UTC+7 for Bangkok)
- Daylight offset: `DAYLIGHT_OFFSET_SEC` (0 = no DST)
- Weather refresh interval: `WEATHER_INTERVAL_MS` (30 minutes)
- Broadcast rate: `BROADCAST_INTERVAL_MS` (250ms)

**Hub only** (`hub/src/Network.cpp` or via Display config screen):
- Wi-Fi SSID
- Wi-Fi password
- NTP server address

**Hub storage** (NVS):
- Credentials stored under namespace `"ambisense"` (persistent)
- Survives power cycles and Hub resets

**Display storage** (NVS):
- Saved Hub credentials under namespace `"ambisense_disp"`
- UI preferences (theme, seconds, date format)
- All persistent

## Offline Behavior

If Hub goes offline:
- Display continues showing last received weather and room data
- Status dot turns red after 5 seconds
- No data loss; everything preserved in memory
- Automatically syncs when Hub comes back online

## Architecture

**Hub** (`hub/`):
- Network — Wi-Fi management, ESP-NOW broadcast, config loading/saving
- Weather — Fetches from Open-Meteo API (free, no key), ~30min refresh
- Sensors — DHT22 polling (2s interval)
- RTCManager — DS3231 interface, NTP sync

**Display** (`display/`):
- Network — ESP-NOW RX, channel auto-sync, offline detection
- UI — LVGL dashboard (theme, date/time formatting, config screen)
- DisplayManager — LVGL initialization and frame rate control

See `docs/ARCHITECTURE.md` in root for detailed module breakdown.

## Troubleshooting

**Display shows offline (red dot)**
- Check Hub serial output: `pio device monitor -b 115200`
- Verify Hub is powered and Wi-Fi connected
- Both devices should be on same 2.4GHz Wi-Fi band
- Hard reset both if stuck: power cycle + reflash if needed

**Weather not updating**
- Check Hub serial: look for HTTP errors or API failures
- Verify Wi-Fi connection (`[NET] Connected`)
- Check internet connectivity on your network

**Time incorrect on Display**
- Verify Hub completed NTP sync (check serial for `[RTC] Sync complete`)
- Ensure NTP server is reachable (test with `ping pool.ntp.org`)
- Timezone hardcoded to UTC+7; edit `include/config/Config.h` if needed

**Config changes not saving**
- Try erasing NVS and reflashing: `pio run -t erase`
- Check serial output for NVS errors
- Verify both projects' `Config.h` have matching namespace names

**Display won't connect to Hub**
- Verify both on same 2.4GHz Wi-Fi network
- Hard reset both: power cycle and reflash
- Check ESP-NOW init in serial logs (both should print `[NET] ESP-NOW ready`)

## Performance

- ESP-NOW latency: <50ms typical
- Update rate: 250ms (weather broadcast), 2s (sensor polling)
- Display refresh: ~60 FPS (LVGL)
- Power consumption: ~2W total (Hub + Display active)
- Range: ~100m line-of-sight (ESP-NOW typical, Wi-Fi dependent)

## Hardware Notes

- **Display**: 320×240 IPS TFT, capacitive touch, built-in SD card slot (optional)
- **RTC Accuracy**: DS3231 ±2 ppm; drifts ~5 seconds per year without NTP
- **DHT22 Range**: -40°C to +80°C, ±2°C accuracy typical
- **API**: Open-Meteo (free, no key needed) — if you want to use OpenWeatherMap, edit `hub/src/Weather.cpp`

## Customization

**Edit before flashing:**

Hub (`include/config/`):
- `Config.h` — Shared protocol (intervals, packet types)
- `HardwareConfig.h` — Pin definitions (DHT22, RTC I2C pins)
- `LocationConfig.h` — Coordinates for weather API, timezone, location name

Display (`include/config/`):
- `Config.h` — Shared protocol (same as Hub)
- `HardwareConfig.h` — Pin definitions (display SPI, touch pins)
- `LocationConfig.h` — Timezone offset (synced with Hub; no separate weather fetching)

**Key settings to update:**

- **Timezone**: `LocationConfig.h` → `LOCATION_GMT_OFFSET_SEC` (e.g., 7*3600 for UTC+7)
- **Location**: `LocationConfig.h` → `LOCATION_LAT`, `LOCATION_LON`, `LOCATION_NAME` (used for weather API)
- **Weather API**: `Weather.cpp` in Hub (if using OpenWeatherMap instead of Open-Meteo, add API key)
- **Hardware pins**: `HardwareConfig.h` in each project (if using different GPIO pins)

Both projects load credentials (Wi-Fi, NTP) from NVS at runtime, so no hardcoding needed after first config setup via Display settings screen.

## Links

- [LVGL Documentation](https://docs.lvgl.io/)
- [ESP-NOW Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html)
- [Open-Meteo API](https://open-meteo.com/)
- [Arduino ESP32 Core](https://github.com/espressif/arduino-esp32)
- [DS3231 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS3231.pdf)

## Notes

This project was developed with human oversight and AI-assisted code generation.
