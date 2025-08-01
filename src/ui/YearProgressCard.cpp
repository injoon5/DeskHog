#include "YearProgressCard.h"
#include "Style.h"
#include <time.h>
#include <sys/time.h>
#include <WiFi.h>

YearProgressCard::YearProgressCard(lv_obj_t* parent, EventQueue& eventQueue, const String& timezone_config) 
    : _event_queue(eventQueue), _card(nullptr), _year_label(nullptr), _progress_bar(nullptr), 
      _last_update(0), _last_ntp_sync(0), _current_year(0), _ntp_initialized(false), _retry_count(0) {
    
    // Initialize timezone configuration
    _timezone_config = getTimezoneConfig(timezone_config);
    
    // Initialize rectangle arrays
    for (int i = 0; i < 12; i++) {
        _top_rectangles[i] = nullptr;
        _bottom_rectangles[i] = nullptr;
    }
    
    // Create main card container
    _card = lv_obj_create(parent);
    lv_obj_set_size(_card, 240, 135);
    lv_obj_set_style_bg_color(_card, lv_color_black(), 0);
    lv_obj_set_style_border_width(_card, 0, 0);
    lv_obj_set_style_pad_all(_card, 0, 0);
    lv_obj_set_style_radius(_card, 0, 0);
    
    
    // Create top row of rectangles (12 rectangles, 2x15 size, #D9D9D9 color, spaced 18 apart)
    for (int i = 0; i < 12; i++) {
        _top_rectangles[i] = lv_obj_create(_card);
        lv_obj_set_size(_top_rectangles[i], 2, 15);
        lv_obj_set_pos(_top_rectangles[i], 21 + (i * 19), 0);
        lv_obj_set_style_bg_color(_top_rectangles[i], lv_color_hex(0xD9D9D9), 0);
        lv_obj_set_style_border_width(_top_rectangles[i], 0, 0);
        lv_obj_set_style_pad_all(_top_rectangles[i], 0, 0);
        lv_obj_set_style_radius(_top_rectangles[i], 0, 0);
    }
    
    // Create bottom row of rectangles (12 rectangles, 2x15 size, #D9D9D9 color, spaced 18 apart)
    for (int i = 0; i < 12; i++) {
        _bottom_rectangles[i] = lv_obj_create(_card);
        lv_obj_set_size(_bottom_rectangles[i], 2, 15);
        lv_obj_set_pos(_bottom_rectangles[i], 21 + (i * 19), 120); // 135 - 15 = 120
        lv_obj_set_style_bg_color(_bottom_rectangles[i], lv_color_hex(0xD9D9D9), 0);
        lv_obj_set_style_border_width(_bottom_rectangles[i], 0, 0);
        lv_obj_set_style_pad_all(_bottom_rectangles[i], 0, 0);
        lv_obj_set_style_radius(_bottom_rectangles[i], 0, 0);
    }
    
    // Create year label in center
    _year_label = lv_label_create(_card);
    lv_obj_set_style_text_color(_year_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_year_label, Style::largestValueFont(), 0);
    lv_obj_set_style_text_align(_year_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(_year_label, LV_ALIGN_CENTER, 0, 0);
    
    // Create progress bar (3px width, 135px height, red color)
    _progress_bar = lv_obj_create(_card);
    lv_obj_set_size(_progress_bar, 3, 135);
    lv_obj_set_style_bg_color(_progress_bar, lv_color_make(255, 0, 0), 0);
    lv_obj_set_style_border_width(_progress_bar, 0, 0);
    lv_obj_set_style_pad_all(_progress_bar, 0, 0);
    lv_obj_set_style_radius(_progress_bar, 0, 0);
    
    // Subscribe to time sync events
    _event_queue.subscribe([this](const Event& event) {
        if (event.type == EventType::TIME_SYNC_COMPLETE && event.cardId == "yearprogress") {
            this->onEvent(event);
        }
    });
    
    // Request initial time sync
    requestTimeSync();
    updateYearProgress();
}

YearProgressCard::~YearProgressCard() {
    // LVGL will handle cleanup when parent is deleted
}

bool YearProgressCard::handleButtonPress(uint8_t button_index) {
    // Don't handle any button presses, allow navigation
    return false;
}

bool YearProgressCard::update() {
    uint32_t current_time = lv_tick_get();
    
    // Update every second (1000ms)
    if (current_time - _last_update >= 1000) {
        updateYearProgress();
        _last_update = current_time;
    }
    
    // Request NTP sync every 5 minutes if connected
    if (current_time - _last_ntp_sync >= 300000) { // 5 minutes
        if (isWiFiConnected()) {
            requestTimeSync();
            _last_ntp_sync = current_time;
        }
    }
    
    return true; // Continue receiving updates
}

void YearProgressCard::updateYearProgress() {
    // Get current time
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    
    // Update year display
    _current_year = timeinfo->tm_year + 1900;
    char year_str[8];
    snprintf(year_str, sizeof(year_str), "%d", _current_year);
    lv_label_set_text(_year_label, year_str);
    
    // Calculate year progress and position progress bar
    float progress = calculateYearProgress();
    int progress_x = (int)(progress * 237); // 240 - 3 = 237 max position
    lv_obj_set_pos(_progress_bar, progress_x, 0);
}

float YearProgressCard::calculateYearProgress() {
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    
    // Calculate day of year (0-based)
    int day_of_year = timeinfo->tm_yday;
    
    // Check if it's a leap year
    int year = timeinfo->tm_year + 1900;
    bool is_leap_year = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    int total_days = is_leap_year ? 366 : 365;
    
    // Add hours, minutes, seconds for more precision
    float day_progress = (timeinfo->tm_hour * 3600 + timeinfo->tm_min * 60 + timeinfo->tm_sec) / 86400.0f;
    float total_progress = (day_of_year + day_progress) / total_days;
    
    return total_progress;
}

void YearProgressCard::onEvent(const Event& event) {
    if (event.type == EventType::TIME_SYNC_COMPLETE) {
        _ntp_initialized = event.success;
        if (event.success) {
            Serial.println("YearProgressCard: Time sync completed successfully");
        } else {
            Serial.println("YearProgressCard: Time sync failed");
        }
        updateYearProgress();
    }
}

void YearProgressCard::requestTimeSync() {
    // Check WiFi connection status
    if (!isWiFiConnected()) {
        if (_retry_count < MAX_RETRIES) {
            _retry_count++;
            Serial.printf("YearProgressCard: WiFi not connected, retry %d/%d\n", _retry_count, MAX_RETRIES);
            
            // Schedule retry after 2 seconds
            _last_ntp_sync = lv_tick_get() - 300000 + 2000; // Retry in 2 seconds
            return;
        } else {
            // Max retries reached
            Serial.println("YearProgressCard: Max retries reached, WiFi not connected");
            return;
        }
    }
    
    // WiFi is connected, reset retry count
    _retry_count = 0;
    
    Serial.printf("YearProgressCard: Requesting time sync for %s (UTC%+d)\n", 
                  _timezone_config.name.c_str(), 
                  _timezone_config.utc_offset_hours);
    
    // Publish event to request time sync (will be handled by Core 0)
    Event timeSyncEvent = Event::createCardEvent(EventType::TIME_SYNC_REQUEST, "yearprogress", 
        _timezone_config.name + "|" + String(_timezone_config.utc_offset_hours));
    _event_queue.publishEvent(timeSyncEvent);
}

bool YearProgressCard::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}