#include "WeatherClient.h"

const char* WeatherClient::API_KEY = "83e3f95ad3aa6558cf610776e64d18f0";
const char* WeatherClient::BASE_URL = "https://api.openweathermap.org/data/2.5/weather";

WeatherClient::WeatherClient() {
}

bool WeatherClient::isReady() const {
    return isWiFiConnected();
}

bool WeatherClient::fetchWeatherData(const String& city, WeatherData& data) {
    if (!isWiFiConnected()) {
        Serial.println("WiFi not connected");
        return false;
    }
    
    String url = buildWeatherUrl(city);
    Serial.printf("Fetching weather from: %s\n", url.c_str());
    
    _http.begin(url);
    _http.setTimeout(10000); // 10 second timeout
    
    int httpCode = _http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("HTTP request failed with code: %d\n", httpCode);
        _http.end();
        return false;
    }
    
    String response = _http.getString();
    _http.end();
    
    // Parse JSON response
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
        Serial.printf("JSON parsing failed: %s\n", error.c_str());
        return false;
    }
    
    // Extract weather data
    if (doc["cod"] != 200) {
        Serial.printf("API error: %s\n", doc["message"].as<String>().c_str());
        return false;
    }
    
    data.temperature = doc["main"]["temp"];
    data.feels_like = doc["main"]["feels_like"];
    data.humidity = doc["main"]["humidity"];
    data.main = doc["weather"][0]["main"].as<String>();
    data.description = doc["weather"][0]["description"].as<String>();
    data.city = doc["name"].as<String>();
    data.valid = true;
    
    return true;
}

String WeatherClient::buildWeatherUrl(const String& city) const {
    return String(BASE_URL) + "?q=" + city + "&units=metric&appid=" + API_KEY;
}

bool WeatherClient::isWiFiConnected() const {
    return WiFi.status() == WL_CONNECTED;
}