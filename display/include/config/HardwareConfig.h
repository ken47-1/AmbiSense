/* ========== HardwareConfig.h ========== */
#pragma once

/* ===== DISPLAY ===== */
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define BUF_HEIGHT    160

/* ===== TOUCH (XPT2046 — CYD2USB, dedicated VSPI bus) ===== */
#define TOUCH_CS   33
#define TOUCH_IRQ  36
#define TOUCH_MOSI 32
#define TOUCH_MISO 39
#define TOUCH_CLK  25