#include <lvgl.h>
#include <Arduino.h>
#include <time.h>

#include "app.h"
#include "hardware.h"
#include "lv_util.h"
#include "weather.h"
#include "spotify.h"
#include "calendar.h"
#include "config.h"

static lv_obj_t *timeLabel;
static lv_obj_t *dateLabel;
static lv_obj_t *temperatureLabel;
static lv_obj_t *songLabel;
static lv_obj_t *calendarLabel;

static void updateTimeLabel(const char *text)
{
    GUILock lock;
    lv_label_set_text(timeLabel, text);
}

static void updateDateLabel(const char *text)
{
    GUILock lock;
    lv_label_set_text(dateLabel, text);
}

static void setupUI()
{
    GUILock lock;

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), LV_PART_MAIN);

    lv_obj_t *centerContainer = lv_obj_create(lv_scr_act());
    lv_obj_set_style_border_width(centerContainer, 0, LV_PART_MAIN);
    lv_obj_set_size(centerContainer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(centerContainer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_align(centerContainer, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_flex_flow(centerContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(centerContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    dateLabel = lv_label_create(centerContainer);
    lv_obj_set_style_text_font(dateLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(dateLabel, lv_color_hex(0xf1f3f3), LV_PART_MAIN);
    lv_label_set_text(dateLabel, "Date");

    timeLabel = lv_label_create(centerContainer);
    lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(timeLabel, lv_color_hex(0x60e083), LV_PART_MAIN);
    lv_label_set_text(timeLabel, "Loading...");

    calendarLabel = lv_label_create(centerContainer);
    lv_obj_set_style_text_font(calendarLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(calendarLabel, lv_color_hex(0xf1be44), LV_PART_MAIN);
    lv_label_set_text(calendarLabel, "Calendar");
    lv_obj_set_width(calendarLabel, LV_PCT(90));
    lv_label_set_long_mode(calendarLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(calendarLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    temperatureLabel = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(temperatureLabel, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(temperatureLabel, lv_color_hex(0x7c9181), LV_PART_MAIN);
    lv_label_set_text(temperatureLabel, "Weather");
    lv_obj_align(temperatureLabel, LV_ALIGN_TOP_RIGHT, -10, 10);

    songLabel = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(songLabel, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(songLabel, lv_color_hex(0x7c9181), LV_PART_MAIN);
    lv_label_set_text(songLabel, "Spotify");
    lv_obj_set_width(songLabel, LV_PCT(100));
    lv_label_set_long_mode(songLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(songLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(songLabel, LV_ALIGN_BOTTOM_MID, 0, -10);
}

void appTask(void *pvParameters)
{
    setupUI();

    updateDateLabel("Connecting to WiFi...");
    connectWiFi();

    updateDateLabel("Syncing time...");
    syncTime();

    WeatherTaskData weatherData = {
        .temperatureLabel = temperatureLabel,
    };

    SpotifyTaskData spotifyData = {
        .songLabel = songLabel,
    };

    CalendarTaskData calendarData = {
        .eventLabel = calendarLabel,
    };

    xTaskCreatePinnedToCore(calendarTask, "calendar", 8192, &calendarData, 3, NULL, APP_CORE);
    xTaskCreatePinnedToCore(weatherTask, "weather", 8192, &weatherData, 3, NULL, APP_CORE);
    xTaskCreatePinnedToCore(spotifyTask, "spotify", 8192, &spotifyData, 3, NULL, APP_CORE);

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        struct tm timeinfo;
        getLocalTime(&timeinfo);

        char timeBuff[32];
        strftime(timeBuff, sizeof(timeBuff), "%l:%M %p", &timeinfo);
        updateTimeLabel(timeBuff);

        char dateBuff[32];
        strftime(dateBuff, sizeof(dateBuff), "%a, %b %d %Y", &timeinfo);
        updateDateLabel(dateBuff);

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}
