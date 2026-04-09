/* ==================== RTCManager.cpp ==================== */
#include "RTCManager.h"

/* =============== INCLUDES =============== */
/* ============ CORE ============ */
#include <Arduino.h>
#include <time.h>

/* =============== PUBLIC API =============== */
/* ============ LIFECYCLE ============ */
/* ========= constructor ========= */
RTCManager::RTCManager()
    : _running(false)
    , _lastSyncDay(-1)
    , _lastSyncCheck(0)
    , _syncStarted(0)
    , _syncInProgress(false)
    , _ntpServer(nullptr)
{
    _ntpStatus = { SyncState::IDLE, 0, false };
}

/* ========= begin ========= */
void RTCManager::begin() {
    Wire.begin(RTC_SDA, RTC_SCL);
    if (!_rtc.begin()) {
        Serial.println("[RTC] DS3231 not found");
        return;
    }
    if (_rtc.lostPower()) Serial.println("[RTC] Lost power — time may be stale");
    _running = true;
    Serial.println("[RTC] DS3231 ready");
}

/* ========= update ========= */
void RTCManager::update(bool wifiConnected, const char* ntpServer) {
    if (!_running) return;
    if (millis() - _lastSyncCheck < SYNC_CHECK_INTERVAL_MS) return;
    _lastSyncCheck = millis();

    /* ------ Handle in-progress sync ------ */
    if (_syncInProgress) {
        time_t now = time(nullptr);
        if (now > 1000000000UL) {
            /* NTP responded with a valid time */
            _finishSync();
        } else if (millis() - _syncStarted > 10000) {
            /* Timeout */
            _ntpStatus.retries++;
            _syncInProgress = false;
            if (_ntpStatus.retries >= 5) {
                _ntpStatus.state = SyncState::FAILED;
                Serial.println("[RTC] NTP sync failed after 5 retries");
            } else {
                Serial.printf("[RTC] NTP timeout, retry %d/5\n", _ntpStatus.retries);
            }
        }
        return;
    }

    if (!wifiConnected || ntpServer == nullptr) return;

    /* ------ Daily sync at TARGET_SYNC_HOUR ------ */
    DateTime now      = _rtc.now();
    bool shouldSync   = (now.hour()  == TARGET_SYNC_HOUR   &&
                         now.minute() == TARGET_SYNC_MINUTE &&
                         now.day()   != _lastSyncDay);

    /* --- Also sync on first boot --- */
    // if (!_ntpStatus.everSynced && _ntpStatus.state != SyncState::FAILED) shouldSync = true;

    if (shouldSync) _startSync(ntpServer);
}

/* ========= getTime ========= */
DateTime RTCManager::getTime() {
    return _rtc.now();
}

/* =============== INTERNAL HELPERS =============== */
/* ============ LOGIC ============ */
/* ------ _startSync ------ */
void RTCManager::_startSync(const char* ntpServer) {
    Serial.println("[RTC] NTP sync starting...");
    _ntpStatus.state = SyncState::SYNCING;
    _syncInProgress  = true;
    _syncStarted     = millis();
    _ntpServer       = ntpServer;
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, ntpServer);
}

/* ------ _finishSync ------ */
void RTCManager::_finishSync() {
    time_t     now = time(nullptr);
    struct tm* ti  = localtime(&now);

    _rtc.adjust(DateTime(
        ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
        ti->tm_hour, ti->tm_min, ti->tm_sec
    ));

    _lastSyncDay            = _rtc.now().day();
    _syncInProgress         = false;
    _ntpStatus.state        = SyncState::DONE;
    _ntpStatus.everSynced   = true;
    _ntpStatus.retries      = 0;

    Serial.printf("[RTC] NTP sync done: %04d-%02d-%02d %02d:%02d:%02d\n",
                  ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
                  ti->tm_hour, ti->tm_min, ti->tm_sec);
}
