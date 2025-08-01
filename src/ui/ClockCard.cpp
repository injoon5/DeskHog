#include "ClockCard.h"
#include "Style.h"
#include "../fonts/fonts.h"
#include <time.h>
#include <sys/time.h>
#include <cstring>
#include <cstdio>
#include <WiFi.h>

ClockCard::ClockCard(lv_obj_t* parent, EventQueue& eventQueue, const String& timezone_config) 
    : _event_queue(eventQueue), _card(nullptr), _time_label(nullptr), _date_label(nullptr), 
      _ampm_label(nullptr), _weekday_label(nullptr), _left_container(nullptr), _error_label(nullptr),
      _progress_bar(nullptr), _last_update(0), _last_ntp_sync(0), _ntp_initialized(false), 
      _time_synced(false), _error_shown(false), _retry_count(0) {
    
    // Initialize timezone configuration
    _timezone_config = getTimezoneConfig(timezone_config);
    
    // Initialize time strings
    strcpy(_time_str, "12:00");
    strcpy(_date_str, "Jan 1");
    strcpy(_ampm_str, "AM");
    strcpy(_weekday_str, "Mon");
    
    // Create main card container
    _card = lv_obj_create(parent);
    lv_obj_set_size(_card, 240, 135);
    lv_obj_set_style_bg_color(_card, lv_color_black(), 0);
    lv_obj_set_style_border_width(_card, 0, 0);
    lv_obj_set_style_pad_all(_card, 0, 0);
    lv_obj_set_style_radius(_card, 0, 0);
    
    // Create left container for date and weekday (vertical stack)
    _left_container = lv_obj_create(_card);
    lv_obj_set_size(_left_container, 90, 135);   // Narrower left area
    lv_obj_set_pos(_left_container, 0, 0);
    lv_obj_set_style_bg_opa(_left_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_left_container, 0, 0);
    lv_obj_set_style_pad_all(_left_container, 0, 0);
    lv_obj_set_style_radius(_left_container, 0, 0);
    lv_obj_set_flex_flow(_left_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_left_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(_left_container, 0, 0);  // Reduce gap between items

    // Create weekday label in left container (top)
    _weekday_label = lv_label_create(_left_container);
    lv_label_set_text(_weekday_label, _weekday_str);
    lv_obj_set_style_text_color(_weekday_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_weekday_label, Style::mediumValueFont(), 0);
    lv_obj_set_style_text_align(_weekday_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Create date label in left container (bottom)
    _date_label = lv_label_create(_left_container);
    lv_label_set_text(_date_label, _date_str);
    lv_obj_set_style_text_color(_date_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_date_label, Style::mediumValueFont(), 0);
    lv_obj_set_style_text_align(_date_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Create right container for AM/PM and time (vertical stack)
    lv_obj_t* right_container = lv_obj_create(_card);
    lv_obj_set_size(right_container, 150, 135);  // Wider right area (240-90=150)
    lv_obj_set_pos(right_container, 90, 0);      // Position after left container
    lv_obj_set_style_bg_opa(right_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right_container, 0, 0);
    lv_obj_set_style_pad_all(right_container, 0, 0);
    lv_obj_set_style_radius(right_container, 0, 0);
    lv_obj_set_flex_flow(right_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(right_container, -3, 0);

    // Create AM/PM label in right container (top of vertical stack)
    _ampm_label = lv_label_create(right_container);
    lv_label_set_text(_ampm_label, _ampm_str);
    lv_obj_set_style_text_color(_ampm_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_ampm_label, Style::mediumValueFont(), 0);
    lv_obj_set_style_text_align(_ampm_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Create time label in right container (bottom of vertical stack)
    _time_label = lv_label_create(right_container);
    lv_label_set_text(_time_label, _time_str);
    lv_obj_set_style_text_color(_time_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_time_label, Style::largestValueFont(), 0);
    lv_obj_set_style_text_align(_time_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Create error label (initially hidden)
    _error_label = lv_label_create(_card);
    lv_label_set_text(_error_label, "Cannot sync time");
    lv_obj_set_style_text_color(_error_label, lv_color_make(255, 100, 100), 0);
    lv_obj_set_style_text_font(_error_label, &font_label, 0);
    lv_obj_set_style_text_align(_error_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(_error_label, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_add_flag(_error_label, LV_OBJ_FLAG_HIDDEN);
    
    // Create progress bar at bottom (2px height, white)
    _progress_bar = lv_obj_create(_card);
    lv_obj_set_size(_progress_bar, 0, 2);  // Start with 0 width, 2px height
    lv_obj_set_pos(_progress_bar, 0, 133); // Position at bottom (135-2=133)
    lv_obj_set_style_bg_color(_progress_bar, lv_color_white(), 0);
    lv_obj_set_style_border_width(_progress_bar, 0, 0);
    lv_obj_set_style_pad_all(_progress_bar, 0, 0);
    lv_obj_set_style_radius(_progress_bar, 0, 0);
    
    // Subscribe to time sync events
    _event_queue.subscribe([this](const Event& event) {
        if (event.type == EventType::TIME_SYNC_COMPLETE && event.cardId == "clock") {
            this->onEvent(event);
        }
    });
    
    // Request initial time sync
    requestTimeSync();
    updateTime();
}

ClockCard::~ClockCard() {
    // LVGL will handle cleanup when parent is deleted
}

bool ClockCard::handleButtonPress(uint8_t button_index) {
    // Don't handle any button presses, allow navigation
    return false;
}

bool ClockCard::update() {
    uint32_t current_time = lv_tick_get();
    
    // Update every 1000ms (1 second)
    if (current_time - _last_update >= 1000) {
        updateTime();
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

void ClockCard::updateTime() {
    // Check WiFi connection status first
    if (!isWiFiConnected()) {
        if (!_error_shown) {
            showError("No Connection");
        }
        return;
    } else {
        if (_error_shown) {
            hideError();
        }
        
        // Request NTP sync if not done yet
        if (!_ntp_initialized) {
            requestTimeSync();
        }
    }
    
    formatTime();
    formatDate();
    formatWeekday();
    updateProgressBar();
    
    lv_label_set_text(_time_label, _time_str);
    lv_label_set_text(_date_label, _date_str);
    lv_label_set_text(_ampm_label, _ampm_str);
    lv_label_set_text(_weekday_label, _weekday_str);
}

void ClockCard::formatTime() {
    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Debug output
    Serial.printf("Raw time: %ld, tm_hour: %d, tm_min: %d, tm_mday: %d, tm_mon: %d\n", 
                  now, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_mday, timeinfo.tm_mon + 1);
    
    int hour = timeinfo.tm_hour;
    int minute = timeinfo.tm_min;
    
    // Convert to 12-hour format
    bool is_pm = hour >= 12;
    if (hour == 0) {
        hour = 12; // Midnight case
    } else if (hour > 12) {
        hour -= 12; // PM case
    }
    
    // Format time string
    snprintf(_time_str, sizeof(_time_str), "%d:%02d", hour, minute);
    
    // Set AM/PM
    strcpy(_ampm_str, is_pm ? "PM" : "AM");
}

void ClockCard::formatDate() {
    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Month names
    const char* months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    
    // Format date string (e.g., "Jul 31")
    snprintf(_date_str, sizeof(_date_str), "%s %d", 
             months[timeinfo.tm_mon], timeinfo.tm_mday);
}

void ClockCard::formatWeekday() {
    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Weekday names (tm_wday: Sunday = 0, Monday = 1, ...)
    const char* weekdays[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    
    // Format weekday string (e.g., "Mon")
    strcpy(_weekday_str, weekdays[timeinfo.tm_wday]);
}

void ClockCard::onEvent(const Event& event) {
    if (event.type == EventType::TIME_SYNC_COMPLETE) {
        _ntp_initialized = event.success;
        if (event.success) {
            Serial.println("ClockCard: Time sync completed successfully");
        } else {
            Serial.println("ClockCard: Time sync failed");
        }
        updateTime();
    }
}

void ClockCard::requestTimeSync() {
    // Check WiFi connection status
    if (!isWiFiConnected()) {
        if (_retry_count < MAX_RETRIES) {
            _retry_count++;
            Serial.printf("ClockCard: WiFi not connected, retry %d/%d\n", _retry_count, MAX_RETRIES);
            
            // Schedule retry after 2 seconds
            _last_ntp_sync = lv_tick_get() - 300000 + 2000; // Retry in 2 seconds
            return;
        } else {
            // Max retries reached
            Serial.println("ClockCard: Max retries reached, WiFi not connected");
            return;
        }
    }
    
    // WiFi is connected, reset retry count
    _retry_count = 0;
    
    Serial.printf("ClockCard: Requesting time sync for %s (UTC%+d)\n", 
                  _timezone_config.name.c_str(), 
                  _timezone_config.utc_offset_hours);
    
    // Publish event to request time sync (will be handled by Core 0)
    Event timeSyncEvent = Event::createCardEvent(EventType::TIME_SYNC_REQUEST, "clock", 
        _timezone_config.name + "|" + String(_timezone_config.utc_offset_hours));
    _event_queue.publishEvent(timeSyncEvent);
}

bool ClockCard::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void ClockCard::showError(const String& message) {
    if (_error_label) {
        lv_label_set_text(_error_label, message.c_str());
        lv_obj_clear_flag(_error_label, LV_OBJ_FLAG_HIDDEN);
        _error_shown = true;
        
        // Keep time/date labels visible - just show error at top
        // Don't hide the time display
    }
}

void ClockCard::hideError() {
    if (_error_label) {
        lv_obj_add_flag(_error_label, LV_OBJ_FLAG_HIDDEN);
        _error_shown = false;
        
        // Time/date labels were never hidden, so no need to show them
    }
}

void ClockCard::updateProgressBar() {
    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Calculate seconds since midnight
    int seconds_since_midnight = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
    int total_seconds_in_day = 24 * 3600;
    
    // Calculate percentage of day passed
    float day_progress = (float)seconds_since_midnight / total_seconds_in_day;
    
    // Update progress bar width (240px is full width)
    int progress_width = (int)(day_progress * 240);
    lv_obj_set_width(_progress_bar, progress_width);
}