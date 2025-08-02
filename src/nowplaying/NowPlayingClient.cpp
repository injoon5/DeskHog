#include "NowPlayingClient.h"

const char* NowPlayingClient::API_KEY = "acf2b63ef5ed315b1f94439b8fcc68e4";
const char* NowPlayingClient::BASE_URL = "https://ws.audioscrobbler.com/2.0/";

NowPlayingClient::NowPlayingClient(const String& username) : _username(username) {
}

bool NowPlayingClient::isReady() const {
    return isWiFiConnected();
}

bool NowPlayingClient::fetchNowPlayingData(NowPlayingData& data) {
    if (!isWiFiConnected()) {
        Serial.println("WiFi not connected");
        return false;
    }
    
    String url = buildLastFmUrl();
    Serial.printf("Fetching now playing from: %s\n", url.c_str());
    
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
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
        Serial.printf("JSON parsing failed: %s\n", error.c_str());
        return false;
    }
    
    // Check if there are tracks
    JsonArray tracks = doc["recenttracks"]["track"];
    if (tracks.size() == 0) {
        Serial.println("No tracks found");
        return false;
    }
    
    // Get the most recent track and sanitize all text
    JsonObject mostRecent = tracks[0];
    String title = mostRecent["name"].as<String>();
    String artist = mostRecent["artist"]["#text"].as<String>();
    String album = mostRecent["album"]["#text"].as<String>();
    
    data.title = sanitizeText(title);
    data.artist = sanitizeText(artist);
    data.album = sanitizeText(album);
    
    // Check if the track is currently playing
    if (mostRecent.containsKey("@attr") && mostRecent["@attr"].containsKey("nowplaying")) {
        data.isPlaying = true;
        data.playedAt = "Now playing";
    } else {
        data.isPlaying = false;
        if (mostRecent.containsKey("date")) {
            data.playedAt = sanitizeText(mostRecent["date"]["#text"].as<String>());
        } else {
            data.playedAt = "Recently played";
        }
    }
    
    data.valid = true;
    return true;
}

String NowPlayingClient::buildLastFmUrl() const {
    return String(BASE_URL) + "?method=user.getrecenttracks&user=" + _username + 
           "&api_key=" + API_KEY + "&format=json&limit=1";
}

bool NowPlayingClient::isWiFiConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

String NowPlayingClient::sanitizeText(const String& text) const {
    String result = "";
    result.reserve(text.length());
    
    for (int i = 0; i < text.length(); i++) {
        unsigned char c = text.charAt(i);
        // Basic Latin (0x20-0x7F), Latin-1 Supplement (0xA0-0xFF)
        if ((c >= 0x20 && c <= 0x7F) || (c >= 0xA0 && c <= 0xFF)) {
            result += (char)c;
        } else if (c >= 0xC0) { // Start of multi-byte UTF-8
            // Skip the multi-byte UTF-8 sequence and replace with '?'
            result += '?';
            // Skip continuation bytes (0x80-0xBF)
            while (i + 1 < text.length() && (text.charAt(i + 1) & 0xC0) == 0x80) {
                i++;
            }
        } else {
            result += '?';
        }
    }
    return result;
}

bool NowPlayingClient::isEnglishText(const String& text) const {
    // This function is now used for filtering, but we'll sanitize all text anyway
    return true; // Always return true since we sanitize text
}