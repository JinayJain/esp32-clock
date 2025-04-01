#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "secrets.h"
#include "config.h"
#include "llm.h"

// --- Global Objects ---
SPIClass touchscreenSpi(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
lv_obj_t *poemLabel;
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
        lv_mutex_lock(&lvglMutex);
        lv_tick_inc(5);
        lv_task_handler();
        lv_mutex_unlock(&lvglMutex);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void appTask(void *pvParameters)
{
    if (!connectToWiFi())
    {
        lv_mutex_lock(&lvglMutex);
        lv_label_set_text(poemLabel, "WiFi connection failed.");
        lv_obj_align(poemLabel, LV_ALIGN_CENTER, 0, 0);
        lv_mutex_unlock(&lvglMutex);
        vTaskDelete(NULL);
        return;
    }

    LLM llm(GEMINI_API_KEY, "https://generativelanguage.googleapis.com/v1beta/openai");

    std::vector<ChatMessage> messages = {{"user", "Write a haiku about a cat."}};

    LLMCompletionOptions options;
    options.model = "gemma-3-27b-it";
    options.temperature = 1.0;
    options.maxTokens = 1024;

    String poem = llm.chatCompletion(messages, options);

    lv_mutex_lock(&lvglMutex);
    if (poem.length() > 0)
    {
        lv_label_set_text(poemLabel, poem.c_str());
    }
    else
    {
        lv_label_set_text(poemLabel, "Error fetching poem.");
    }
    lv_obj_align(poemLabel, LV_ALIGN_CENTER, 0, 0);
    lv_mutex_unlock(&lvglMutex);

    vTaskDelete(NULL);
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

    lv_mutex_lock(&lvglMutex);
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
    poemLabel = lv_label_create(lv_scr_act());
    lv_obj_set_width(poemLabel, TFT_HOR_RES - 20);
    lv_label_set_long_mode(poemLabel, LV_LABEL_LONG_WRAP);
    lv_label_set_text(poemLabel, "Initializing...");
    lv_obj_set_style_text_color(poemLabel, lv_color_white(), 0);
    lv_obj_set_style_text_align(poemLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_line_space(poemLabel, 10, 0);
    lv_obj_align(poemLabel, LV_ALIGN_CENTER, 0, 0);
    lv_mutex_unlock(&lvglMutex);

    xTaskCreatePinnedToCore(
        lvglTask,
        "LVGL Task",
        8192,
        NULL,
        1,
        &lvglTaskHandle,
        0);

    xTaskCreatePinnedToCore(
        appTask,
        "App Task",
        10240,
        NULL,
        1,
        &appTaskHandle,
        1);
}

void loop()
{
    // End the task, as LVGL has its own loop
    vTaskDelete(NULL);
}
