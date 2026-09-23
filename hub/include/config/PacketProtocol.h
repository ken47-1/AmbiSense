/* ==================== PacketProtocol.h ==================== */
/* Shared protocol definitions between the AmbiSense Hub and Display */
#pragma once

/* =============== INCLUDES =============== */
/* ============ CORE ============ */
#include <stdint.h>

/* =============== ESP-NOW =============== */
constexpr uint8_t ESPNOW_BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/* =============== PACKET TYPES =============== */
constexpr uint8_t PACKET_TYPE_DATA   = 0x01;
constexpr uint8_t PACKET_TYPE_CONFIG = 0x02;
constexpr uint8_t PACKET_TYPE_ACK    = 0x03;
constexpr uint8_t PACKET_TYPE_CMD    = 0x04;

/* =============== COMMAND IDs =============== */
constexpr uint8_t CMD_FORCE_NTP_SYNC = 0x01;

/* =============== PACKET STRUCTS =============== */
/* ------ Hub -> Display ------ */
struct __attribute__((packed)) DataPacket {
    uint8_t  type;
    uint8_t  seq;
    uint8_t  channel;
    uint8_t  wifiConnected;
    uint32_t timestamp;
    uint8_t  locationValid;
    char     city[33];
    uint8_t  weatherValid;
    uint8_t  weatherCode;
    float    outsideTemp;
    float    apparentTemp;
    uint8_t  outsideHumi;
    uint16_t outsidePress;
    float    windSpeed;
    int16_t  windDirection;
    char     sunrise[8];
    char     sunset[8];
    uint8_t  roomValid;
    float    roomTemp;
    float    roomHumi;
};

static_assert(sizeof(DataPacket) < 240, "DataPacket too large for ESP-NOW");

/* ------ Display -> Hub ------ */
struct __attribute__((packed)) ConfigPacket {
    uint8_t type;
    char    ssid[33];
    char    password[64];
    char    ntp[64];
    uint8_t seq;
};

/* ------ Bidirectional ------ */
struct __attribute__((packed)) AckPacket {
    uint8_t type;
    uint8_t ack_seq;
};

/* ------ Display -> Hub ------ */
struct __attribute__((packed)) CmdPacket {
    uint8_t type;
    uint8_t cmd;
    uint8_t seq;
};
