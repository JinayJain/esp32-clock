#include "weather.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "lv_util.h"
#include "config.h"

#define WEATHER_UPDATE_INTERVAL_MIN 2
#define OPENMETEO_API_URL "http://api.open-meteo.com/v1/forecast"

const char *getWeatherDescription(int weatherCode)
{
    switch (weatherCode)
    {
    case 0:
        return "Clear";
    case 1:
    case 2:
    case 3:
        return "Partly cloudy";
    case 45:
    case 48:
        return "Fog";
    case 51:
    case 53:
    case 55:
        return "Drizzle";
    case 56:
    case 57:
        return "Freezing drizzle";
    case 61:
    case 63:
    case 65:
        return "Rain";
    case 66:
    case 67:
        return "Freezing rain";
    case 71:
    case 73:
    case 75:
        return "Snow";
    case 77:
        return "Snow grains";
    case 80:
    case 81:
    case 82:
        return "Rain showers";
    case 85:
    case 86:
        return "Snow showers";
    case 95:
        return "Thunderstorm";
    case 96:
    case 99:
        return "Thunderstorm with hail";
    default:
        return "Unknown";
    }
}

void createWeatherFilter(JsonDocument &filter)
{
    JsonObject current = filter.createNestedObject("current");
    current["temperature_2m"] = true;
    current["weather_code"] = true;
}

void weatherTask(void *pvParameters)
{
    WeatherTaskData *weatherData = (WeatherTaskData *)pvParameters;
    HTTPClient http;

    while (1)
    {
        Serial.println("Updating weather...");

        char url[256];
        snprintf(url, sizeof(url),
                 "%s?latitude=%f&longitude=%f&current=temperature_2m,weather_code&temperature_unit=fahrenheit",
                 OPENMETEO_API_URL, LATITUDE, LONGITUDE);

        http.begin(url);
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK)
        {
            String payload = http.getString();

            StaticJsonDocument<128> filter;
            createWeatherFilter(filter);

            DynamicJsonDocument doc(512);
            DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));

            if (!error)
            {
                float temperature = doc["current"]["temperature_2m"];
                int weatherCode = doc["current"]["weather_code"];
                const char *weatherDesc = getWeatherDescription(weatherCode);

                char weatherText[64];
                snprintf(weatherText, sizeof(weatherText), "%.0f°F (%s)", temperature, weatherDesc);

                GUILock lock;
                lv_label_set_text(weatherData->temperatureLabel, weatherText);
            }
        }

        http.end();
        vTaskDelay(pdMS_TO_TICKS(WEATHER_UPDATE_INTERVAL_MIN * 60 * 1000));
    }

    vTaskDelete(NULL);
}
