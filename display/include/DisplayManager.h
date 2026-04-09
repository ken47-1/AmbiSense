/* ========== DisplayManager.h ========== */
#pragma once

/* ===== INCLUDES ===== */
/* --- THIRD-PARTY --- */
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>
/* --- PROJECT --- */
#include "config/HardwareConfig.h"

/* ===== API ===== */
class DisplayManager {
public:
    DisplayManager();
    void begin();
    void update();

    bool isTouched()   { return _touch.touched(); }
    TS_Point getTouch() { return _touch.getPoint(); }

private:
    static void _flushCb(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* px);
    static void _touchReadCb(lv_indev_drv_t* drv, lv_indev_data_t* data);

    static TFT_eSPI            _tft;
    static SPIClass            _touchSpi;
    static XPT2046_Touchscreen _touch;
    static lv_disp_drv_t      _disp_drv;
    static lv_disp_draw_buf_t  _draw_buf;
    static lv_indev_drv_t      _indev_drv;
    static lv_color_t*         _buf;
};