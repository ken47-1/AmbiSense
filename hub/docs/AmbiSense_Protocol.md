# AmbiSense ESP-NOW Protocol

## Overview

The Hub and Display communicate over ESP-NOW (Wi-Fi layer 2, no router required). All packets are raw byte structs sent via `esp_now_send`. The shared packet definitions live in `include/config/Config.h`, which is identical on both devices.

---

## Packet Types

| ID     | Name            | Direction         | Description                        |
|--------|-----------------|-------------------|------------------------------------|
| `0x01` | `DATA`          | Hub → Display     | Sensor data broadcast, every 1s    |
| `0x02` | `CONFIG`        | Display → Hub     | Wi-Fi / NTP credentials            |
| `0x03` | `ACK`           | Hub → Display     | Acknowledges CONFIG or CMD receipt |
| `0x04` | `CMD`           | Display → Hub     | Command (e.g. force NTP sync)      |

The first byte of every packet is always `type`, so the receiver can dispatch before casting.

---

## Packet Structs

All structs are `__attribute__((packed))`.

### DataPacket (Hub → Display)

```c
struct DataPacket {
    uint8_t  type;          // 0x01
    uint32_t timestamp;     // Unix timestamp from DS3231 (UTC+7)
    float    temp;          // Room temperature, °C (-99.0 if sensor invalid)
    float    humidity;      // Room humidity, % (-99.0 if sensor invalid)
    uint8_t  seq;           // Rolling sequence number (0–255, wraps)
    uint8_t  wifiConnected; // 1 if Hub is connected to Wi-Fi, 0 otherwise
};
```

**Size:** 13 bytes

Broadcast to `FF:FF:FF:FF:FF:FF` every `BROADCAST_INTERVAL_MS` (1000 ms).  
The Display learns the Hub's MAC from the first received DataPacket and registers it as a peer for directed ACKs.

Sensor validity: `temp > -50.0 && humidity > 0.0` — if false, display shows `ERR`.

### ConfigPacket (Display → Hub)

```c
struct ConfigPacket {
    uint8_t type;           // 0x02
    char    ssid[33];       // Wi-Fi SSID, null-terminated
    char    password[64];   // Wi-Fi password, null-terminated
    char    ntp[64];        // NTP server hostname, null-terminated
    uint8_t seq;            // Sequence number for ACK matching
};
```

**Size:** 163 bytes

Sent when the user submits the config screen on the Display. The Hub saves the credentials to NVS, sends an ACK, and reconnects to Wi-Fi.

### AckPacket (Bidirectional)

```c
struct AckPacket {
    uint8_t type;           // 0x03
    uint8_t ack_seq;        // Echoes back seq from the packet being acknowledged
};
```

**Size:** 2 bytes

Sent by the Hub in response to `CONFIG` and `CMD` packets. The Display currently logs ACKs but does not retry on failure.

### CmdPacket (Display → Hub)

```c
struct CmdPacket {
    uint8_t type;           // 0x04
    uint8_t cmd;            // Command ID
    uint8_t seq;            // Sequence number for ACK matching
};
```

**Size:** 3 bytes

#### Command IDs

| ID     | Name                | Effect                                      |
|--------|---------------------|---------------------------------------------|
| `0x01` | `CMD_FORCE_NTP_SYNC`| Hub calls `rtc.forceSync()` on next update  |

---

## Status Dot Logic (Display)

The top-right indicator dot reflects combined connectivity state:

| Colour  | Meaning                                              |
|---------|------------------------------------------------------|
| Green   | Display online + Hub online + weather data valid     |
| Gold    | Display online + weather valid, Hub offline          |
| Orange  | Hub online (reports via DataPacket), Display offline |
| Red     | Both offline                                         |

---

## Sequence Numbers

- `DataPacket.seq` — rolling 0–255, wraps. Display detects drops and logs them. No retransmit.
- `ConfigPacket.seq` / `CmdPacket.seq` — used by Hub's ACK. Display increments a separate `_txSeq` counter per transmission.

---

## NVS Keys

Both devices use the namespace `"ambisense"`:

| Key    | Type   | Description           |
|--------|--------|-----------------------|
| `ssid` | String | Wi-Fi SSID            |
| `pass` | String | Wi-Fi password        |
| `ntp`  | String | NTP server hostname   |

---

## Timing

| Constant                 | Value  | Description                        |
|--------------------------|--------|------------------------------------|
| `BROADCAST_INTERVAL_MS`  | 1000   | Hub data broadcast rate            |
| `DHT_INTERVAL_MS`        | 2000   | DHT22 minimum read interval        |
| `SYNC_CHECK_INTERVAL_MS` | 1000   | RTCManager NTP check interval      |
| UI update rate           | 100 ms | Display redraws at 10 Hz           |
