#include "spotify.h"
#include "secrets.h"
#include "lv_util.h"

#include <lvgl.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <base64.h>

#define SPOTIFY_UPDATE_INTERVAL_SEC 10

String SpotifyClient::accessToken;
uint32_t SpotifyClient::lastTokenRefresh = 0;

void spotifyTask(void *pvParameters)
{
    SpotifyTaskData *data = (SpotifyTaskData *)pvParameters;
    lv_obj_t *songLabel = data->songLabel;

    const TickType_t xFrequency = pdMS_TO_TICKS(SPOTIFY_UPDATE_INTERVAL_SEC * 1000);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        NowPlaying nowPlaying = SpotifyClient::getCurrentlyPlaying();

        String displayText;
        if (nowPlaying.isPlaying)
        {
            displayText = String(LV_SYMBOL_AUDIO) + "  " + nowPlaying.song;
        }
        else
        {
            displayText = "";
        }

        // Update UI with song info
        {
            GUILock lock;
            lv_label_set_text(songLabel, displayText.c_str());
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }

    vTaskDelete(NULL);
}

NowPlaying SpotifyClient::getCurrentlyPlaying()
{
    if (shouldRefreshToken())
    {
        accessToken = getToken();
        lastTokenRefresh = millis();
    }

    return getPlayingState(accessToken);
}

bool SpotifyClient::shouldRefreshToken()
{
    return accessToken.isEmpty() || (millis() - lastTokenRefresh >= 3600000);
}

String SpotifyClient::getToken()
{
    HTTPClient http;
    http.begin("https://accounts.spotify.com/api/token");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Authorization", "Basic " + base64::encode(String(SPOTIFY_CLIENT_ID) + ":" + String(SPOTIFY_CLIENT_SECRET)));

    String body = "grant_type=refresh_token&refresh_token=" + String(SPOTIFY_REFRESH_TOKEN);
    int httpResponseCode = http.POST(body);
    if (httpResponseCode > 0)
    {
        String response = http.getString();
        StaticJsonDocument<512> doc;
        deserializeJson(doc, response);
        return doc["access_token"].as<String>();
    }

    return "";
}

NowPlaying SpotifyClient::getPlayingState(const String &access_token)
{
    HTTPClient http;
    http.begin("https://api.spotify.com/v1/me/player/currently-playing");
    http.addHeader("Authorization", "Bearer " + access_token);

    NowPlaying now_playing = {"", "", "", false};

    int httpResponseCode = http.GET();
    if (httpResponseCode == 200)
    {
        String response = http.getString();
        parseSpotifyResponse(response, now_playing);
    }
    else if (httpResponseCode == 204)
    {
        now_playing.artist = "Spotify Inactive";
    }

    http.end();
    return now_playing;
}

void SpotifyClient::parseSpotifyResponse(const String &response, NowPlaying &now_playing)
{
    StaticJsonDocument<256> filter;
    createSpotifyFilter(filter);

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response, DeserializationOption::Filter(filter));
    if (error)
    {
        Serial.print(F("deserializeJson() deserialization failed: "));
        Serial.println(error.c_str());
        return;
    }

    now_playing.artist = getArtistsString(doc["item"]["artists"]);
    now_playing.song = doc["item"]["name"].as<String>();
    now_playing.albumImageUrl = getAlbumImageUrl(doc["item"]["album"]["images"]);
    now_playing.isPlaying = doc["is_playing"].as<bool>();
}

void SpotifyClient::createSpotifyFilter(JsonDocument &filter)
{
    filter["is_playing"] = true;
    filter["currently_playing_type"] = true;
    filter["progress_ms"] = true;
    filter["context"]["uri"] = true;

    JsonObject filter_item = filter.createNestedObject("item");
    filter_item["duration_ms"] = true;
    filter_item["name"] = true;
    filter_item["uri"] = true;

    JsonObject filter_item_artists_0 = filter_item["artists"].createNestedObject();
    filter_item_artists_0["name"] = true;
    filter_item_artists_0["uri"] = true;

    JsonObject filter_item_album = filter_item.createNestedObject("album");
    filter_item_album["name"] = true;
    filter_item_album["uri"] = true;

    JsonObject filter_item_album_images_0 = filter_item_album["images"].createNestedObject();
    filter_item_album_images_0["height"] = true;
    filter_item_album_images_0["width"] = true;
    filter_item_album_images_0["url"] = true;
}

String SpotifyClient::getArtistsString(JsonArray artists)
{
    String artistString = "";
    for (int i = 0; i < artists.size(); i++)
    {
        if (i > 0)
            artistString += ", ";
        artistString += artists[i]["name"].as<String>();
    }
    return artistString;
}

String SpotifyClient::getAlbumImageUrl(JsonArray images)
{
    for (int i = 0; i < images.size(); i++)
    {
        if (images[i]["height"].as<int>() == 64)
            return images[i]["url"].as<String>();
    }
    return "";
}