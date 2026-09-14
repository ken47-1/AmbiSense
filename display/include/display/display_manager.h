/* ==================== display_manager.h ==================== */
#pragma once

/* =============== INCLUDES =============== */

/* ============ CONFIG ============ */
#include "config/HardwareConfig.h"

/* ============ THIRD-PARTY ============ */
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>

/* ============ CORE ============ */
#include <SPI.h>

namespace AmbiSense::Display {

/* =============== TYPES =============== */
/* ============ STRUCTS ============ */
struct TouchPoint {
    int16_t x;
    int16_t y;
};

/* =============== API =============== */
class DisplayManager {
public:
    DisplayManager();
    void begin();
    void update();

    /* ---- Brightness Control ---- */
    static void    setBrightness(uint8_t percent);
    static uint8_t getBrightness();

    /* ---- Touch ---- */
    bool       isTouched() { return _touch.touched(); }
    TouchPoint getTouch();

private:
    static void _flushCb(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* px);
    static void _touchReadCb(lv_indev_drv_t* drv, lv_indev_data_t* data);
    static void _applyBrightness(uint8_t percent);

    static TFT_eSPI            _tft;
    static SPIClass            _touchSpi;
    static XPT2046_Touchscreen _touch;
    static lv_disp_drv_t       _disp_drv;
    static lv_disp_draw_buf_t  _draw_buf;
    static lv_indev_drv_t      _indev_drv;
    static lv_color_t*         _buf;
    static uint8_t             _currentPercent;
};

} // namespace AmbiSense::Display