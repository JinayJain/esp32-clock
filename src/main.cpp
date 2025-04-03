#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>

#include "config.h"
#include "hardware.h"
#include "lv_util.h"
#include "app/app.h"

SemaphoreHandle_t guiMutex;

void guiTask(void *pvParameters);

static uint32_t tickCallback(void) { return millis(); }

void setup()
{
    Serial.begin(115200);

    lv_init();
    initDisplay();
    initTouchscreen();
    lv_tick_set_cb(tickCallback);

    guiMutex = xSemaphoreCreateMutex();

    {
        GUILock lock;

        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), LV_PART_MAIN);
    }

    TaskHandle_t guiTaskHandle;
    TaskHandle_t appTaskHandle;

    xTaskCreatePinnedToCore(
        guiTask,
        "guiTask",
        LVGL_STACK_SIZE,
        NULL,
        5,
        &guiTaskHandle,
        LVGL_CORE);

    xTaskCreatePinnedToCore(
        appTask,
        "appTask",
        APP_STACK_SIZE,
        NULL,
        5,
        &appTaskHandle,
        APP_CORE);
}

void guiTask(void *pvParameters)
{
    const TickType_t xFrequency = pdMS_TO_TICKS(5);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {

        lockGui();
        lv_task_handler();
        unlockGui();

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Main loop is not used in this project
void loop()
{
    vTaskDelete(NULL);
}