#pragma once

#include <lvgl.h>

class App
{
public:
    App();
    void setup();
    void loop();

private:
    lv_obj_t *timeLabel;
    lv_obj_t *dateLabel;
    lv_obj_t *temperatureLabel;

    const float latitude = 39.7876;
    const float longitude = -75.6966;
    float temperature = 0.0;
    int weatherCode = 0;
    unsigned long lastWeatherUpdate = 0;
    const unsigned long weatherUpdateInterval = 10 * 60 * 1000; // 10 minutes in milliseconds

    bool fetchWeatherData();
    const char *getWeatherDescription(int code);
};