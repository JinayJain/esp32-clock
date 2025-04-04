#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <ArduinoJson.h>
struct CalendarEvent
{
    String title;
    String location;
    time_t startTime;
    time_t endTime;
    bool isActive;
};

struct CalendarTaskData
{
    lv_obj_t *eventLabel;
};

void calendarTask(void *pvParameters);

class GoogleCalendarClient
{
public:
    static CalendarEvent getUpcomingEvent();

private:
    static String accessToken;
    static uint32_t lastTokenRefresh;

    static String getToken();
    static bool shouldRefreshToken();
    static CalendarEvent parseCalendarEvents(const String &response);
    static time_t parseISODateTime(const char *dateTime);
    static CalendarEvent findSoonestEvent(const CalendarEvent *events, int count);
    static void createCalendarFilter(JsonDocument &filter);
};
