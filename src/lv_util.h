#pragma once

#include <lvgl.h>
#include <Arduino.h>
extern lv_mutex_t lvglMutex;

class LVGLMutexGuard
{
public:
    LVGLMutexGuard()
    {
        lv_mutex_lock(&lvglMutex);
    }
    ~LVGLMutexGuard()
    {
        lv_mutex_unlock(&lvglMutex);
    }
};
