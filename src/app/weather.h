#pragma once

#include <lvgl.h>

struct WeatherTaskData
{
    lv_obj_t *temperatureLabel;
};

void weatherTask(void *pvParameters);
