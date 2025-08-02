#include "NowPlayingCard.h"
#include "Style.h"
#include "../hardware/Input.h"
#include <Arduino.h>
#include <WiFi.h>

NowPlayingCard::NowPlayingCard(lv_obj_t* parent, EventQueue& eventQueue, const String& username_config) 
    : _event_queue(eventQueue), _username_config(username_config), _nowPlayingClient(nullptr), _last_update(0), 
      _error_shown(false), _has_data(false), _retry_count(0), _pending_update(false), _pending_error_hide(false), _pending_error_show(false) {
    
    _card = lv_obj_create(parent);
    lv_obj_set_size(_card, 240, 135);
    lv_obj_set_pos(_card, 0, 0);
    lv_obj_set_style_bg_color(_card, lv_color_black(), 0);
    lv_obj_set_style_border_width(_card, 0, 0);
    lv_obj_set_style_pad_all(_card, 0, 0);
    
    // Title container (0, 35), size (240, 30)
    _title_container = lv_obj_create(_card);
    lv_obj_set_size(_title_container, 240, 30);
    lv_obj_set_pos(_title_container, 0, 10);
    lv_obj_set_style_bg_opa(_title_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_title_container, 0, 0);
    lv_obj_set_style_pad_all(_title_container, 0, 0);
    
    _title_label = lv_label_create(_title_container);
    lv_obj_set_style_text_font(_title_label, Style::mediumValueFont(), 0);
    lv_obj_set_style_text_color(_title_label, lv_color_white(), 0);
    lv_obj_set_style_text_align(_title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(_title_label, 230);
    lv_obj_align(_title_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_long_mode(_title_label, LV_LABEL_LONG_SCROLL);
    lv_label_set_text(_title_label, "Loading...");
    
    // Album container (0, 60), size (240, 15)
    _album_container = lv_obj_create(_card);
    lv_obj_set_size(_album_container, 240, 25);
    lv_obj_set_pos(_album_container, 0, 40);
    lv_obj_set_style_bg_opa(_album_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_album_container, 0, 0);
    lv_obj_set_style_pad_all(_album_container, 0, 0);
    
    _album_label = lv_label_create(_album_container);
    lv_obj_set_style_text_font(_album_label, Style::valueFont(), 0);
    lv_obj_set_style_text_color(_album_label, lv_color_hex(0xD8D8D8), 0);
    lv_obj_set_style_text_align(_album_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(_album_label, 230);
    lv_obj_align(_album_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_long_mode(_album_label, LV_LABEL_LONG_SCROLL);
    lv_label_set_text(_album_label, "");
    
    // Artist container (0, 85), size (240, 15)
    _artist_container = lv_obj_create(_card);
    lv_obj_set_size(_artist_container, 240, 25);
    lv_obj_set_pos(_artist_container, 0, 70);
    lv_obj_set_style_bg_opa(_artist_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_artist_container, 0, 0);
    lv_obj_set_style_pad_all(_artist_container, 0, 0);
    
    _artist_label = lv_label_create(_artist_container);
    lv_obj_set_style_text_font(_artist_label, Style::valueFont(), 0);
    lv_obj_set_style_text_color(_artist_label, lv_color_white(), 0);
    lv_obj_set_style_text_align(_artist_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(_artist_label, 230);
    lv_obj_align(_artist_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_long_mode(_artist_label, LV_LABEL_LONG_SCROLL);
    lv_label_set_text(_artist_label, "");
    
    // Status container (0, 110), size (240, 15)
    _status_container = lv_obj_create(_card);
    lv_obj_set_size(_status_container, 240, 20);
    lv_obj_set_pos(_status_container, 0, 110);
    lv_obj_set_style_bg_opa(_status_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_status_container, 0, 0);
    lv_obj_set_style_pad_all(_status_container, 0, 0);
    
    _status_label = lv_label_create(_status_container);
    lv_obj_set_style_text_font(_status_label, Style::labelFont(), 0);
    lv_obj_set_style_text_color(_status_label, lv_color_hex(0x90EE90), 0);
    lv_obj_set_style_text_align(_status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(_status_label, 230);
    lv_obj_align(_status_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(_status_label, "");
    
    // Error label (hidden by default)
    _error_label = lv_label_create(_card);
    lv_obj_set_style_text_font(_error_label, Style::valueFont(), 0);
    lv_obj_set_style_text_color(_error_label, lv_color_hex(0xFF6B6B), 0);
    lv_obj_set_style_text_align(_error_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(_error_label, 220);
    lv_obj_align(_error_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(_error_label, LV_OBJ_FLAG_HIDDEN);
    
    // Subscribe to now playing events
    _event_queue.subscribe([this](const Event& event) {
        if (event.type == EventType::NOW_PLAYING_DATA_RECEIVED && event.cardId == "nowplaying") {
            this->onEvent(event);
        }
    });
    
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
    // Handle pending UI updates from Core 0 events
    if (_pending_update) {
        updateNowPlayingDisplay(_pending_data);
        _pending_update = false;
    }
    
    if (_pending_error_hide) {
        hideError();
        _pending_error_hide = false;
    }
    
    if (_pending_error_show) {
        showError(_pending_error_message);
        _pending_error_show = false;
    }
    
    uint32_t now = millis();
    
    // Check if it's time to update
    if (now - _last_update >= UPDATE_INTERVAL) {
        requestNowPlayingUpdate();
        return true;
    }
    
    return false;
}

void NowPlayingCard::onEvent(const Event& event) {
    if (event.type == EventType::NOW_PLAYING_DATA_RECEIVED) {
        if (event.success && !event.data.isEmpty()) {
            // Parse the data format: "title|artist|album|playedAt|isPlaying"
            NowPlayingData data;
            int pipes[4];
            int pipeCount = 0;
            
            // Find all pipe positions
            for (int i = 0; i < event.data.length() && pipeCount < 4; i++) {
                if (event.data.charAt(i) == '|') {
                    pipes[pipeCount++] = i;
                }
            }
            
            if (pipeCount >= 4) {
                data.title = event.data.substring(0, pipes[0]);
                data.artist = event.data.substring(pipes[0] + 1, pipes[1]);
                data.album = event.data.substring(pipes[1] + 1, pipes[2]);
                data.playedAt = event.data.substring(pipes[2] + 1, pipes[3]);
                data.isPlaying = event.data.substring(pipes[3] + 1) == "1";
                
                Serial.printf("NowPlayingCard: Data received: %s by %s (Playing: %s)\n", 
                              data.title.c_str(), data.artist.c_str(), data.isPlaying ? "Yes" : "No");
                
                // Store data for UI update on Core 1
                _pending_data = data;
                _pending_update = true;
                _pending_error_hide = true;
                _has_data = true;
            } else {
                Serial.printf("NowPlayingCard: Invalid data format received (expected 5 fields, got %d)\n", pipeCount + 1);
                if (!_has_data) {
                    _pending_error_message = "Invalid data";
                    _pending_error_show = true;
                }
            }
        } else {
            Serial.println("NowPlayingCard: Failed to receive data");
            if (!_has_data) {
                _pending_error_message = "Failed to fetch data";
                _pending_error_show = true;
            }
        }
    }
}

void NowPlayingCard::requestNowPlayingUpdate() {
    // Check WiFi connection status
    if (!isWiFiConnected()) {
        if (_retry_count < MAX_RETRIES) {
            _retry_count++;
            Serial.printf("NowPlayingCard: WiFi not connected, retry %d/%d\n", _retry_count, MAX_RETRIES);
            
            // Show connection status
            if (!_has_data) {
                if (_retry_count == 1) {
                    lv_label_set_text(_title_label, "Connecting...");
                    lv_label_set_text(_album_label, "");
                    lv_label_set_text(_artist_label, "");
                    lv_label_set_text(_status_label, "");
                } else {
                    String retryMsg = "Retry " + String(_retry_count) + "/" + String(MAX_RETRIES);
                    lv_label_set_text(_title_label, retryMsg.c_str());
                    lv_label_set_text(_album_label, "");
                    lv_label_set_text(_artist_label, "");
                    lv_label_set_text(_status_label, "");
                }
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
    
    // WiFi is connected, reset retry count
    _retry_count = 0;
    
    Serial.printf("NowPlayingCard: Requesting now playing for %s\n", _username_config.c_str());
    
    // Show loading state
    if (!_has_data) {
        lv_label_set_text(_title_label, "Loading...");
        lv_label_set_text(_album_label, "");
        lv_label_set_text(_artist_label, "");
        lv_label_set_text(_status_label, "");
    }
    
    // Publish event to request now playing data (will be handled by Core 0)
    Event nowPlayingEvent = Event::createCardEvent(EventType::NOW_PLAYING_REQUEST, "nowplaying", _username_config);
    _event_queue.publishEvent(nowPlayingEvent);
    
    _last_update = millis(); // Update timestamp when request is made
}

void NowPlayingCard::updateNowPlayingDisplay(const NowPlayingData& data) {
    // Update labels directly - scrolling will handle long text
    lv_label_set_text(_title_label, data.title.c_str());
    lv_label_set_text(_album_label, data.album.c_str());
    lv_label_set_text(_artist_label, data.artist.c_str());
    
    // Set status text and color based on playing state
    if (data.isPlaying) {
        lv_label_set_text(_status_label, "Now Playing");
        lv_obj_set_style_text_color(_status_label, lv_color_hex(0x90EE90), 0); // Light green
    } else {
        lv_label_set_text(_status_label, data.playedAt.c_str());
        lv_obj_set_style_text_color(_status_label, lv_color_hex(0xD8D8D8), 0); // Light gray
    }
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
    lv_obj_add_flag(_status_label, LV_OBJ_FLAG_HIDDEN);
    
    _error_shown = true;
}

void NowPlayingCard::hideError() {
    if (_error_shown) {
        lv_obj_add_flag(_error_label, LV_OBJ_FLAG_HIDDEN);
        
        // Show other labels
        lv_obj_clear_flag(_title_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(_album_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(_artist_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(_status_label, LV_OBJ_FLAG_HIDDEN);
        
        _error_shown = false;
    }
}


bool NowPlayingCard::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}