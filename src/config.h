#pragma once

#define TFT_HOR_RES 240
#define TFT_VER_RES 320

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

#define LVGL_CORE 0
#define APP_CORE 1