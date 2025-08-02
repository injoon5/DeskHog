#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>

struct NowPlayingData {
    String title;
    String artist;
    String album;
    String playedAt;
    bool isPlaying;
    bool valid;
    
    NowPlayingData() : valid(false), isPlaying(false) {}
};

class NowPlayingClient {
public:
    NowPlayingClient(const String& username);
    
    bool fetchNowPlayingData(NowPlayingData& data);
    bool isReady() const;
    
private:
    static const char* API_KEY;
    static const char* BASE_URL;
    String _username;
    
    HTTPClient _http;
    
    String buildLastFmUrl() const;
    bool isWiFiConnected() const;
    bool isEnglishText(const String& text) const;
    String sanitizeText(const String& text) const;
};