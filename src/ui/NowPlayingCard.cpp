#include "NowPlayingCard.h"
#include "Style.h"
#include "../hardware/Input.h"
#include <Arduino.h>

NowPlayingCard::NowPlayingCard(lv_obj_t* parent, const String& username_config) 
    : _username_config(username_config), _nowPlayingClient(nullptr), _last_update(0), 
      _error_shown(false), _has_data(false), _retry_count(0) {
    
    _card = lv_obj_create(parent);
    lv_obj_set_size(_card, 240, 135);
    lv_obj_set_pos(_card, 0, 0);
    lv_obj_set_style_bg_color(_card, lv_color_black(), 0);
    lv_obj_set_style_border_width(_card, 0, 0);
    lv_obj_set_style_pad_all(_card, 0, 0);
    
    // Title container (0, 35), size (240, 30)
    _title_container = lv_obj_create(_card);
    lv_obj_set_size(_title_container, 240, 30);
    lv_obj_set_pos(_title_container, 0, 25);
    lv_obj_set_style_bg_opa(_title_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_title_container, 0, 0);
    lv_obj_set_style_pad_all(_title_container, 0, 0);
    
    _title_label = lv_label_create(_title_container);
    lv_obj_set_style_text_font(_title_label, Style::mediumValueFont(), 0);
    lv_obj_set_style_text_color(_title_label, lv_color_white(), 0);
    lv_obj_set_style_text_align(_title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(_title_label, 240);
    lv_obj_align(_title_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(_title_label, "Loading...");
    
    // Album container (0, 60), size (240, 15)
    _album_container = lv_obj_create(_card);
    lv_obj_set_size(_album_container, 240, 25);
    lv_obj_set_pos(_album_container, 0, 55);
    lv_obj_set_style_bg_opa(_album_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_album_container, 0, 0);
    lv_obj_set_style_pad_all(_album_container, 0, 0);
    
    _album_label = lv_label_create(_album_container);
    lv_obj_set_style_text_font(_album_label, Style::valueFont(), 0);
    lv_obj_set_style_text_color(_album_label, lv_color_hex(0xD8D8D8), 0);
    lv_obj_set_style_text_align(_album_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(_album_label, 240);
    lv_obj_align(_album_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(_album_label, "");
    
    // Artist container (0, 85), size (240, 15)
    _artist_container = lv_obj_create(_card);
    lv_obj_set_size(_artist_container, 240, 25);
    lv_obj_set_pos(_artist_container, 0, 85);
    lv_obj_set_style_bg_opa(_artist_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_artist_container, 0, 0);
    lv_obj_set_style_pad_all(_artist_container, 0, 0);
    
    _artist_label = lv_label_create(_artist_container);
    lv_obj_set_style_text_font(_artist_label, Style::valueFont(), 0);
    lv_obj_set_style_text_color(_artist_label, lv_color_white(), 0);
    lv_obj_set_style_text_align(_artist_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(_artist_label, 240);
    lv_obj_align(_artist_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(_artist_label, "");
    
    // Error label (hidden by default)
    _error_label = lv_label_create(_card);
    lv_obj_set_style_text_font(_error_label, Style::valueFont(), 0);
    lv_obj_set_style_text_color(_error_label, lv_color_hex(0xFF6B6B), 0);
    lv_obj_set_style_text_align(_error_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(_error_label, 220);
    lv_obj_align(_error_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(_error_label, LV_OBJ_FLAG_HIDDEN);
    
    // Create client with username
    _nowPlayingClient = new NowPlayingClient(_username_config);
    
    // Request initial update
    requestNowPlayingUpdate();
}

NowPlayingCard::~NowPlayingCard() {
    if (_nowPlayingClient) {
        delete _nowPlayingClient;
    }
}

bool NowPlayingCard::handleButtonPress(uint8_t button_index) {
    // Handle center button press to refresh data
    if (button_index == Input::BUTTON_CENTER) {
        Serial.println("NowPlayingCard: Center button pressed, refreshing data");
        _retry_count = 0; // Reset retry count for manual refresh
        requestNowPlayingUpdate();
        return true; // We handled the button press
    }
    
    return false; // Let CardNavigationStack handle navigation buttons
}

bool NowPlayingCard::update() {
    uint32_t now = millis();
    
    // Check if it's time to update
    if (now - _last_update >= UPDATE_INTERVAL) {
        requestNowPlayingUpdate();
        return true;
    }
    
    return false;
}

void NowPlayingCard::requestNowPlayingUpdate() {
    if (!_nowPlayingClient) {
        Serial.println("NowPlayingCard: No client set");
        return;
    }
    
    if (!_nowPlayingClient->isReady()) {
        if (_retry_count < MAX_RETRIES) {
            _retry_count++;
            Serial.printf("NowPlayingCard: WiFi not ready, retry %d/%d\n", _retry_count, MAX_RETRIES);
            
            // Show loading message on first attempt, or retry message on subsequent attempts
            if (_retry_count == 1) {
                lv_label_set_text(_title_label, "Connecting...");
            } else {
                String retryMsg = "Retry " + String(_retry_count) + "/" + String(MAX_RETRIES);
                lv_label_set_text(_title_label, retryMsg.c_str());
            }
            
            // Schedule retry after 2 seconds
            _last_update = millis() - UPDATE_INTERVAL + 2000;
            return;
        } else {
            // Max retries reached, show error
            if (!_error_shown) {
                showError("No Connection");
            }
            return;
        }
    }
    
    // WiFi is ready, reset retry count
    _retry_count = 0;
    
    Serial.printf("NowPlayingCard: Requesting now playing for %s\n", _username_config.c_str());
    
    NowPlayingData data;
    if (_nowPlayingClient->fetchNowPlayingData(data)) {
        Serial.printf("NowPlayingCard: Data fetched successfully: %s by %s\n", 
                      data.title.c_str(), data.artist.c_str());
        updateNowPlayingDisplay(data);
        hideError();
        _has_data = true;
        _last_update = millis(); // Update timestamp on successful fetch
    } else {
        Serial.printf("NowPlayingCard: Failed to fetch data for %s\n", _username_config.c_str());
        if (!_has_data) {
            showError("Failed to fetch data");
        }
    }
}

void NowPlayingCard::updateNowPlayingDisplay(const NowPlayingData& data) {
    // Update title with truncation
    String truncatedTitle = truncateText(data.title, Style::mediumValueFont(), 220);
    lv_label_set_text(_title_label, truncatedTitle.c_str());
    
    // Update album with truncation
    String truncatedAlbum = truncateText(data.album, Style::valueFont(), 220);
    lv_label_set_text(_album_label, truncatedAlbum.c_str());
    
    // Update artist with truncation
    String truncatedArtist = truncateText(data.artist, Style::mediumValueFont(), 220);
    lv_label_set_text(_artist_label, truncatedArtist.c_str());
}

void NowPlayingCard::showError(const String& message) {
    if (_has_data) {
        return; // Don't show error if we have previous data
    }
    
    lv_label_set_text(_error_label, message.c_str());
    lv_obj_clear_flag(_error_label, LV_OBJ_FLAG_HIDDEN);
    
    // Hide other labels
    lv_obj_add_flag(_title_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_album_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_artist_label, LV_OBJ_FLAG_HIDDEN);
    
    _error_shown = true;
}

void NowPlayingCard::hideError() {
    if (_error_shown) {
        lv_obj_add_flag(_error_label, LV_OBJ_FLAG_HIDDEN);
        
        // Show other labels
        lv_obj_clear_flag(_title_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(_album_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(_artist_label, LV_OBJ_FLAG_HIDDEN);
        
        _error_shown = false;
    }
}

String NowPlayingCard::truncateText(const String& text, const lv_font_t* font, int maxWidth) {
    if (text.length() == 0) return text;
    
    // Simple truncation - measure text width and truncate if needed
    lv_point_t size;
    lv_text_get_size(&size, text.c_str(), font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);

    if (size.x <= maxWidth) {
        return text;
    }
    
    // Binary search for the right length
    String truncated = text;
    int left = 0, right = text.length();
    
    while (left < right) {
        int mid = (left + right + 1) / 2;
        String candidate = text.substring(0, mid) + "...";
        
        lv_text_get_size(&size, candidate.c_str(), font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        
        if (size.x <= maxWidth) {
            left = mid;
            truncated = candidate;
        } else {
            right = mid - 1;
        }
    }
    
    return truncated;
}