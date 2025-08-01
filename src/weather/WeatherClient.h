#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>

struct WeatherData {
    float temperature;
    float feels_like;
    int humidity;
    String main;
    String description;
    String city;
    bool valid;
    
    WeatherData() : temperature(0.0), feels_like(0.0), humidity(0), valid(false) {}
};

class WeatherClient {
public:
    WeatherClient();
    
    bool fetchWeatherData(const String& city, WeatherData& data);
    bool isReady() const;
    
private:
    static const char* API_KEY;
    static const char* BASE_URL;
    
    HTTPClient _http;
    
    String buildWeatherUrl(const String& city) const;
    bool isWiFiConnected() const;
};