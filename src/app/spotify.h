#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <lvgl.h>

struct SpotifyTaskData
{
    lv_obj_t *songLabel;
};

void spotifyTask(void *pvParameters);

struct NowPlaying
{
    String artist;
    String song;
    String albumImageUrl;
    bool isPlaying;
};

class SpotifyClient
{
public:
    static NowPlaying getCurrentlyPlaying();

private:
    static String accessToken;
    static uint32_t lastTokenRefresh;

    static String getToken();
    static bool shouldRefreshToken();
    static NowPlaying getPlayingState(const String &access_token);
    static void parseSpotifyResponse(const String &response, NowPlaying &now_playing);
    static void createSpotifyFilter(JsonDocument &filter);
    static String getArtistsString(JsonArray artists);
    static String getAlbumImageUrl(JsonArray images);
};