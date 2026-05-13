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
    if (_rtc.lostPower()) Serial.println("[RTC] Power lost");
    _running = true;
    Serial.println("[RTC] DS3231 INIT");
}

/* ========= update ========= */
void RTCManager::update(uint32_t nowMs, bool wifiConnected, const char* ntpServer) {
    if (!_running) return;
    
    // Use the passed-in snapshot instead of calling millis() again
    if (nowMs - _lastSyncCheck < SYNC_CHECK_INTERVAL_MS) return;
    _lastSyncCheck = nowMs;

    if (_syncInProgress) {
        time_t sysTime = time(nullptr);
        if (sysTime > 1000000000UL) { // Valid Unix timestamp check
            _finishSync();
        } else if (nowMs - _syncStarted > NTP_SYNC_TIMEOUT_DELAY_MS) {
            _ntpStatus.retries++;
            _syncInProgress = false;
            if (_ntpStatus.retries >= NTP_SYNC_MAX_RETRIES) {
                _ntpStatus.state = SyncState::FAILED;
                Serial.printf("[RTC] NTP sync failed after %d retries", NTP_SYNC_MAX_RETRIES);
            } else {
                Serial.printf("[RTC] NTP timeout, retry %d/%d\n", _ntpStatus.retries, NTP_SYNC_MAX_RETRIES);
            }
        }
        return;
    }

    if (!wifiConnected || ntpServer == nullptr) return;

    /* ------ Daily sync at TARGET_SYNC_HOUR/TARGET_SYNC_MINUTE ------ */
    DateTime now      = _rtc.now();
    bool shouldSync   = (now.hour()   == TARGET_SYNC_HOUR   &&
                         now.minute() == TARGET_SYNC_MINUTE &&
                         now.day()    != _lastSyncDay);

    /* --- Also sync on first boot --- */
    if (!_ntpStatus.everSynced && _ntpStatus.state != SyncState::FAILED) shouldSync = true;

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
    time_t now = time(nullptr);  // This is already UTC
    struct tm* ti = gmtime(&now);  // Use gmtime, NOT localtime
    
    _rtc.adjust(DateTime(
        ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
        ti->tm_hour, ti->tm_min, ti->tm_sec
    ));
    
    _lastSyncDay = _rtc.now().day();
    _syncInProgress = false;
    _ntpStatus.state = SyncState::DONE;
    _ntpStatus.everSynced = true;
    _ntpStatus.retries = 0;
    
    // For display/debug, convert to local
    struct tm* local_ti = localtime(&now);
    Serial.printf("[RTC] NTP sync done (UTC): %04d-%02d-%02d %02d:%02d:%02d\n",
                  ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
                  ti->tm_hour, ti->tm_min, ti->tm_sec);
    Serial.printf("[RTC] Local time: %02d:%02d:%02d\n",
                  local_ti->tm_hour, local_ti->tm_min, local_ti->tm_sec);
}