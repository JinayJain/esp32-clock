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

#include "secrets.h"
#include "config.h"
#include "llm.h"
#include "app.h"
#include "lv_util.h"

// --- Global Objects ---
SPIClass touchscreenSpi(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
TaskHandle_t lvglTaskHandle = NULL;
TaskHandle_t appTaskHandle = NULL;
lv_mutex_t lvglMutex;
// --------------------

// --- Global Variables ---
uint16_t touchScreenMinimumX = 200, touchScreenMaximumX = 3700, touchScreenMinimumY = 240, touchScreenMaximumY = 3800;
// --------------------

// --- Function Declarations ---
void appTask(void *pvParameters);
void lvglTask(void *pvParameters);
void touchpadRead(lv_indev_t *indev, lv_indev_data_t *data);
bool connectToWiFi();
bool syncTime();
// -------------------------

bool connectToWiFi()
{
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 30)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        return true;
    }

    Serial.println("\nWiFi connection failed!");
    return false;
}
bool syncTime()
{
    configTime(-4 * 3600, 0, "pool.ntp.org", "time.nist.gov");

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return false;
    }

    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    Serial.print("Time synchronized: ");
    Serial.println(timeStr);
    return true;
}

void touchpadRead(lv_indev_t *indev, lv_indev_data_t *data)
{
    if (touchscreen.touched())
    {
        TS_Point p = touchscreen.getPoint();
        // Dynamically calibrate touch boundaries
        if (p.x < touchScreenMinimumX)
            touchScreenMinimumX = p.x;
        if (p.x > touchScreenMaximumX)
            touchScreenMaximumX = p.x;
        if (p.y < touchScreenMinimumY)
            touchScreenMinimumY = p.y;
        if (p.y > touchScreenMaximumY)
            touchScreenMaximumY = p.y;
        data->point.x = map(p.x, touchScreenMinimumX, touchScreenMaximumX, 1, TFT_HOR_RES);
        data->point.y = map(p.y, touchScreenMinimumY, touchScreenMaximumY, 1, TFT_VER_RES);
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void lvglTask(void *pvParameters)
{
    while (1)
    {
        LVGLMutexGuard guard;

        lv_tick_inc(5);
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void appTask(void *pvParameters)
{
    if (!connectToWiFi() || !syncTime())
    {
        vTaskDelete(NULL);
        return;
    }

    App app;
    app.setup();

    while (1)
    {
        app.loop();
    }
}

void setup()
{
    Serial.begin(115200);

    lv_init();
    lv_mutex_init(&lvglMutex);

    touchscreenSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    touchscreen.begin(touchscreenSpi);
    touchscreen.setRotation(2);

    uint8_t *draw_buf1 = new uint8_t[DRAW_BUF_SIZE];
    if (!draw_buf1)
    {
        Serial.println("LVGL draw_buf allocation failed!");
        // Consider adding error handling here, like halting
        while (1)
            vTaskDelay(portMAX_DELAY);
    }

    lv_display_t *disp;
    disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf1, DRAW_BUF_SIZE);
    lv_display_set_rotation(disp, LV_DISP_ROTATION_90);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchpadRead);

    xTaskCreatePinnedToCore(
        lvglTask,
        "LVGL Task",
        8192,
        NULL,
        1,
        &lvglTaskHandle,
        LVGL_CORE);

    xTaskCreatePinnedToCore(
        appTask,
        "App Task",
        10240,
        NULL,
        1,
        &appTaskHandle,
        APP_CORE);
}

void loop()
{
    // End the task, as LVGL has its own loop
    vTaskDelete(NULL);
}
