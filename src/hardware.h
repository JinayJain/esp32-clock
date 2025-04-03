#pragma once

#include <lvgl.h>

/**
 * Initializes the display hardware and configures LVGL display driver
 */
void initDisplay();

/**
 * Initializes the XPT2046 touchscreen and registers it with LVGL
 */
void initTouchscreen();

/**
 * LVGL touchpad read callback function
 * @param indev Pointer to the input device driver
 * @param data Pointer to the input data to be filled
 */
void touchpadRead(lv_indev_t *indev, lv_indev_data_t *data);

/**
 * Connects to WiFi using credentials from secrets.h
 * @return true if connection successful, false otherwise
 */
bool connectWiFi();

/**
 * Synchronizes device time with NTP servers
 * @return true if time sync successful, false otherwise
 */
bool syncTime();