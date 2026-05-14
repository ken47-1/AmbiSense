/* ==================== DisplayManager.cpp ==================== */
#include "DisplayManager.h"

/* =============== INCLUDES =============== */
/* ============ CORE ============ */
#include <Arduino.h>

/* =============== INTERNAL STATE =============== */
/* ============ STATIC VARS ============ */
TFT_eSPI            DisplayManager::_tft;
SPIClass            DisplayManager::_touchSpi = SPIClass(VSPI);
XPT2046_Touchscreen DisplayManager::_touch(TOUCH_CS, TOUCH_IRQ);
lv_disp_drv_t       DisplayManager::_disp_drv;
lv_disp_draw_buf_t  DisplayManager::_draw_buf;
lv_indev_drv_t      DisplayManager::_indev_drv;
lv_color_t*         DisplayManager::_buf = nullptr;

/* =============== PUBLIC API =============== */
/* ============ LIFECYCLE ============ */
DisplayManager::DisplayManager() {}

void DisplayManager::begin() {
    size_t buf_size = SCREEN_WIDTH * BUF_HEIGHT;
    _buf = (lv_color_t *)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    if (_buf == nullptr) {
        Serial.println("[DISP] ERROR: Could not allocate display buffer!");
        return;
    }

    _touchSpi.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    _touch.begin(_touchSpi);
    _touch.setRotation(3);

    lv_init();

    _tft.begin();
    _tft.setRotation(3);

    lv_disp_draw_buf_init(&_draw_buf, _buf, nullptr, buf_size);

    lv_disp_drv_init(&_disp_drv);
    _disp_drv.hor_res  = SCREEN_WIDTH;
    _disp_drv.ver_res  = SCREEN_HEIGHT;
    _disp_drv.flush_cb = _flushCb;
    _disp_drv.draw_buf = &_draw_buf;
    lv_disp_drv_register(&_disp_drv);

    lv_indev_drv_init(&_indev_drv);
    _indev_drv.type    = LV_INDEV_TYPE_POINTER;
    _indev_drv.read_cb = _touchReadCb;
    lv_indev_drv_register(&_indev_drv);

    Serial.println("[DISP] DisplayManager initialized.");
}

void DisplayManager::update() {
    static uint32_t last_tick = 0;
    uint32_t current_ms = millis();
    
    lv_tick_inc(current_ms - last_tick);
    last_tick = current_ms;
    lv_timer_handler();
}

/* =============== INTERNAL HELPERS =============== */
/* ============ CALLBACKS ============ */
void DisplayManager::_flushCb(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* px) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    _tft.startWrite();
    _tft.setAddrWindow(area->x1, area->y1, w, h);
    _tft.pushColors((uint16_t*)&px->full, w * h, true);
    _tft.endWrite();
    lv_disp_flush_ready(drv);
}

void DisplayManager::_touchReadCb(lv_indev_drv_t* drv, lv_indev_data_t* data) {
    if (_touch.touched()) {
        TS_Point p = _touch.getPoint();
        int16_t x = map(p.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, SCREEN_WIDTH);
        int16_t y = map(p.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, SCREEN_HEIGHT);
        data->point.x = constrain(x, 0, SCREEN_WIDTH - 1);
        data->point.y = constrain(y, 0, SCREEN_HEIGHT - 1);
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}