#include "calendar.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <time.h>

#include "lv_util.h"
#include "config.h"
#include "secrets.h"

#define CALENDAR_UPDATE_INTERVAL_MIN 1 // Increased from 1 to 5 minutes to reduce API calls
#define GOOGLE_OAUTH_URL "https://oauth2.googleapis.com/token"
#define GOOGLE_CALENDAR_API_URL "https://www.googleapis.com/calendar/v3/calendars/primary/events"
#define MAX_CALENDAR_EVENTS 3 // Reduced from 5 to 3 events

// Function declarations
String formatEventTime(time_t eventTime);
String urlEncode(const String &str);

// Statics initialization
String GoogleCalendarClient::accessToken = "";
uint32_t GoogleCalendarClient::lastTokenRefresh = 0;

time_t GoogleCalendarClient::parseISODateTime(const char *dateTime)
{
    // Format: YYYY-MM-DDTHH:MM:SSZ
    struct tm tm = {};
    char temp[5] = {0};

    // Parse year
    strncpy(temp, dateTime, 4);
    tm.tm_year = atoi(temp) - 1900;

    // Parse month
    strncpy(temp, dateTime + 5, 2);
    temp[2] = '\0';
    tm.tm_mon = atoi(temp) - 1;

    // Parse day
    strncpy(temp, dateTime + 8, 2);
    temp[2] = '\0';
    tm.tm_mday = atoi(temp);

    // Parse hour
    strncpy(temp, dateTime + 11, 2);
    temp[2] = '\0';
    tm.tm_hour = atoi(temp);

    // Parse minute
    strncpy(temp, dateTime + 14, 2);
    temp[2] = '\0';
    tm.tm_min = atoi(temp);

    // Parse second
    strncpy(temp, dateTime + 17, 2);
    temp[2] = '\0';
    tm.tm_sec = atoi(temp);

    return mktime(&tm);
}

bool GoogleCalendarClient::shouldRefreshToken()
{
    return accessToken.isEmpty() || (millis() - lastTokenRefresh > 3540000); // ~59 minutes
}

String GoogleCalendarClient::getToken()
{
    if (!shouldRefreshToken())
    {
        return accessToken;
    }

    HTTPClient http;
    http.begin(GOOGLE_OAUTH_URL);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String postData = "client_id=" + String(GOOGLE_CLIENT_ID) +
                      "&client_secret=" + String(GOOGLE_CLIENT_SECRET) +
                      "&refresh_token=" + String(GOOGLE_REFRESH_TOKEN) +
                      "&grant_type=refresh_token";

    int httpCode = http.POST(postData);

    if (httpCode == HTTP_CODE_OK)
    {
        String response = http.getString();

        StaticJsonDocument<512> doc; // Reduced from 1024 to 512 bytes
        DeserializationError error = deserializeJson(doc, response);

        if (!error)
        {
            accessToken = doc["access_token"].as<String>();
            lastTokenRefresh = millis();
        }
    }

    http.end();
    return accessToken;
}

void GoogleCalendarClient::createCalendarFilter(JsonDocument &filter)
{
    JsonObject items_0 = filter["items"].createNestedObject();
    items_0["summary"] = true;
    items_0["location"] = true;

    JsonObject start = items_0.createNestedObject("start");
    start["dateTime"] = true;

    JsonObject end = items_0.createNestedObject("end");
    end["dateTime"] = true;
}

CalendarEvent GoogleCalendarClient::parseCalendarEvents(const String &response)
{
    CalendarEvent noEvent = {"No upcoming events", "", 0, 0, false};

    // Check for empty response
    if (response.length() < 10)
    {
        Serial.println("Empty calendar response");
        return noEvent;
    }

    // Create filter to reduce memory usage
    StaticJsonDocument<192> filter; // Reduced from 256 to 192 bytes
    createCalendarFilter(filter);

    // Use smaller document with filter
    DynamicJsonDocument doc(1024); // Reduced from 2048 to 1024 bytes
    DeserializationError error = deserializeJson(doc, response, DeserializationOption::Filter(filter));

    if (error)
    {
        Serial.print(F("Failed to parse calendar response: "));
        Serial.println(error.c_str());
        return noEvent;
    }

    JsonArray items = doc["items"];

    if (items.size() == 0)
    {
        return noEvent;
    }

    CalendarEvent events[MAX_CALENDAR_EVENTS];
    int count = 0;

    time_t now = time(NULL);
    time_t twoHoursFromNow = now + (2 * 60 * 60);

    for (JsonObject item : items)
    {
        if (count >= MAX_CALENDAR_EVENTS)
            break;

        if (!item.containsKey("start") || !item.containsKey("end"))
            continue;

        const char *startDateTime = NULL;
        if (item["start"].containsKey("dateTime"))
        {
            startDateTime = item["start"]["dateTime"];
        }

        if (!startDateTime)
            continue;

        const char *endDateTime = NULL;
        if (item["end"].containsKey("dateTime"))
        {
            endDateTime = item["end"]["dateTime"];
        }

        if (!endDateTime)
            continue;

        time_t eventStart = parseISODateTime(startDateTime);
        time_t eventEnd = parseISODateTime(endDateTime);

        // Skip past events
        if (eventEnd < now)
            continue;

        // Only include events within the next 2 hours
        if (eventStart > twoHoursFromNow)
            continue;

        events[count].title = item["summary"].as<String>();
        events[count].location = item.containsKey("location") ? item["location"].as<String>() : "";
        events[count].startTime = eventStart;
        events[count].endTime = eventEnd;
        events[count].isActive = (eventStart <= now && now <= eventEnd);

        count++;
    }

    if (count == 0)
    {
        return noEvent;
    }

    return findSoonestEvent(events, count);
}

CalendarEvent GoogleCalendarClient::findSoonestEvent(const CalendarEvent *events, int count)
{
    if (count == 0)
    {
        CalendarEvent noEvent = {"No upcoming events", "", 0, 0, false};
        return noEvent;
    }

    time_t now = time(NULL);
    int soonestIndex = 0;

    // First, check for active events
    for (int i = 0; i < count; i++)
    {
        if (events[i].isActive)
        {
            return events[i];
        }
    }

    // If no active events, find the soonest upcoming one
    for (int i = 1; i < count; i++)
    {
        if (events[i].startTime < events[soonestIndex].startTime)
        {
            soonestIndex = i;
        }
    }

    return events[soonestIndex];
}

CalendarEvent GoogleCalendarClient::getUpcomingEvent()
{
    String token = getToken();
    CalendarEvent noEvent = {"No upcoming events", "", 0, 0, false};

    if (token.isEmpty())
    {
        return noEvent;
    }

    HTTPClient http;

    time_t now = time(NULL);
    time_t twoHoursFromNow = now + (2 * 60 * 60);

    char timeMin[30];
    struct tm *timeinfo = gmtime(&now);
    strftime(timeMin, sizeof(timeMin), "%Y-%m-%dT%H:%M:%SZ", timeinfo);

    char timeMax[30];
    timeinfo = gmtime(&twoHoursFromNow);
    strftime(timeMax, sizeof(timeMax), "%Y-%m-%dT%H:%M:%SZ", timeinfo);

    // Pre-build URL to reduce string concatenation operations
    char url[256];
    snprintf(url, sizeof(url), "%s?timeMin=%s&timeMax=%s&singleEvents=true&orderBy=startTime&maxResults=5",
             GOOGLE_CALENDAR_API_URL,
             urlEncode(timeMin).c_str(),
             urlEncode(timeMax).c_str());

    http.begin(url);
    http.addHeader("Authorization", "Bearer " + token);

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        String response = http.getString();
        http.end();
        return parseCalendarEvents(response);
    }

    http.end();
    return noEvent;
}

String formatEventTime(time_t eventTime)
{
    char buffer[20];
    struct tm *timeinfo = localtime(&eventTime);

    int hour = timeinfo->tm_hour;
    if (hour > 12)
    {
        hour -= 12;
    }
    else if (hour == 0)
    {
        hour = 12;
    }

    snprintf(buffer, sizeof(buffer), "%d:%02d %s",
             hour,
             timeinfo->tm_min,
             timeinfo->tm_hour >= 12 ? "PM" : "AM");

    return String(buffer);
}

String urlEncode(const String &str)
{
    String encoded = "";
    char c;
    char code0;
    char code1;
    for (int i = 0; i < str.length(); i++)
    {
        c = str.charAt(i);
        if (c == ' ')
        {
            encoded += '+';
        }
        else if (isalnum(c))
        {
            encoded += c;
        }
        else
        {
            code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9)
            {
                code1 = (c & 0xf) - 10 + 'A';
            }
            c = (c >> 4) & 0xf;
            code0 = c + '0';
            if (c > 9)
            {
                code0 = c - 10 + 'A';
            }
            encoded += '%';
            encoded += code0;
            encoded += code1;
        }
    }
    return encoded;
}

void calendarTask(void *pvParameters)
{
    CalendarTaskData *calendarData = (CalendarTaskData *)pvParameters;

    // Add delay before first request to allow system to stabilize
    vTaskDelay(pdMS_TO_TICKS(5000));

    while (1)
    {
        Serial.println("Updating calendar events...");

        CalendarEvent event = GoogleCalendarClient::getUpcomingEvent();

        if (event.isActive)
        {
            char eventText[128];
            snprintf(eventText, sizeof(eventText), LV_SYMBOL_BELL " Now: %s (until %s)",
                     event.title.c_str(),
                     formatEventTime(event.endTime).c_str());

            GUILock lock;
            lv_label_set_text(calendarData->eventLabel, eventText);
        }
        else if (event.startTime > 0)
        {
            char eventText[128];
            snprintf(eventText, sizeof(eventText), LV_SYMBOL_BELL " %s at %s",
                     event.title.c_str(),
                     formatEventTime(event.startTime).c_str());

            GUILock lock;
            lv_label_set_text(calendarData->eventLabel, eventText);
        }
        else
        {
            GUILock lock;
            lv_label_set_text(calendarData->eventLabel, "No upcoming events");
        }

        vTaskDelay(pdMS_TO_TICKS(CALENDAR_UPDATE_INTERVAL_MIN * 60 * 1000));
    }

    vTaskDelete(NULL);
}
