#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

extern SemaphoreHandle_t guiMutex;

inline void lockGui()
{
    xSemaphoreTake(guiMutex, portMAX_DELAY);
}

inline void unlockGui()
{
    xSemaphoreGive(guiMutex);
}

class GUILock
{
public:
    GUILock()
    {
        lockGui();
    }

    ~GUILock()
    {
        unlockGui();
    }
};
