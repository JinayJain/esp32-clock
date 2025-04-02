#include "app.h"
#include "lv_util.h"
#include "config.h"
#include <time.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

App::App()
{
}

void App::setup()
{
    LVGLMutexGuard guard;

    lv_obj_t *mainContainer = lv_obj_create(lv_scr_act());
    lv_obj_set_size(mainContainer, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(mainContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mainContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    timeLabel = lv_label_create(mainContainer);
    lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_32, LV_PART_MAIN);

    dateLabel = lv_label_create(mainContainer);
    lv_obj_set_style_text_font(dateLabel, &lv_font_montserrat_14, LV_PART_MAIN);

    temperatureLabel = lv_label_create(mainContainer);
    lv_obj_set_style_text_font(temperatureLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_label_set_text(temperatureLabel, "Fetching weather...");

    fetchWeatherData();
}

void App::loop()
{
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        char timeStr[64];
        char dateStr[64];

        strftime(timeStr, sizeof(timeStr), "%l:%M %p", &timeinfo);
        strftime(dateStr, sizeof(dateStr), "%A, %B %e, %Y", &timeinfo);

        LVGLMutexGuard guard;

        lv_label_set_text(timeLabel, timeStr);
        lv_label_set_text(dateLabel, dateStr);
    }

    if (millis() - lastWeatherUpdate >= weatherUpdateInterval)
    {
        fetchWeatherData();
        lastWeatherUpdate = millis();
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
}

bool App::fetchWeatherData()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected");
        return false;
    }

    HTTPClient http;
    char url[256];
    snprintf(url, sizeof(url),
             "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,weather_code&temperature_unit=fahrenheit",
             latitude, longitude);

    Serial.print("Fetching weather from: ");
    Serial.println(url);

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK)
    {
        Serial.print("HTTP error: ");
        Serial.println(httpCode);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
        Serial.print("JSON parsing error: ");
        Serial.println(error.c_str());
        return false;
    }

    // Extract temperature and weather code
    temperature = doc["current"]["temperature_2m"].as<float>();
    weatherCode = doc["current"]["weather_code"].as<int>();

    // Update the temperature label with weather description
    LVGLMutexGuard guard;
    char weatherStr[64];
    snprintf(weatherStr, sizeof(weatherStr), "%.1f°F (%s)", temperature, getWeatherDescription(weatherCode));
    lv_label_set_text(temperatureLabel, weatherStr);

    Serial.print("Updated weather: ");
    Serial.println(weatherStr);

    return true;
}

const char *App::getWeatherDescription(int code)
{
    // WMO Weather interpretation codes (WW)
    // https://open-meteo.com/en/docs
    switch (code)
    {
    case 0:
        return "Clear";
    case 1:
        return "Mainly Clear";
    case 2:
        return "Partly Cloudy";
    case 3:
        return "Overcast";
    case 45:
        return "Fog";
    case 48:
        return "Rime Fog";
    case 51:
        return "Light Drizzle";
    case 53:
        return "Drizzle";
    case 55:
        return "Heavy Drizzle";
    case 56:
        return "Light Freezing Drizzle";
    case 57:
        return "Freezing Drizzle";
    case 61:
        return "Light Rain";
    case 63:
        return "Rain";
    case 65:
        return "Heavy Rain";
    case 66:
        return "Light Freezing Rain";
    case 67:
        return "Freezing Rain";
    case 71:
        return "Light Snow";
    case 73:
        return "Snow";
    case 75:
        return "Heavy Snow";
    case 77:
        return "Snow Grains";
    case 80:
        return "Light Showers";
    case 81:
        return "Showers";
    case 82:
        return "Heavy Showers";
    case 85:
        return "Light Snow Showers";
    case 86:
        return "Snow Showers";
    case 95:
        return "Thunderstorm";
    case 96:
    case 99:
        return "Thunderstorm with Hail";
    default:
        return "Unknown";
    }
}