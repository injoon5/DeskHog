#include "ClockCard.h"
#include "Style.h"
#include "../fonts/fonts.h"
#include <time.h>
#include <sys/time.h>
#include <cstring>
#include <cstdio>
#include <WiFi.h>

ClockCard::ClockCard(lv_obj_t* parent, const String& timezone_config) 
    : _card(nullptr), _time_label(nullptr), _date_label(nullptr), 
      _ampm_label(nullptr), _weekday_label(nullptr), _left_container(nullptr), _error_label(nullptr),
      _progress_bar(nullptr), _last_update(0), _last_ntp_sync(0), _ntp_initialized(false), 
      _time_synced(false), _error_shown(false) {
    
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
    lv_obj_set_style_pad_gap(_left_container, 5, 0);  // Reduce gap between items
    
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
    
    // Create progress bar at bottom (3px height, white)
    _progress_bar = lv_obj_create(_card);
    lv_obj_set_size(_progress_bar, 0, 3);  // Start with 0 width, 3px height
    lv_obj_set_pos(_progress_bar, 0, 132); // Position at bottom (135-3=132)
    lv_obj_set_style_bg_color(_progress_bar, lv_color_white(), 0);
    lv_obj_set_style_border_width(_progress_bar, 0, 0);
    lv_obj_set_style_pad_all(_progress_bar, 0, 0);
    lv_obj_set_style_radius(_progress_bar, 0, 0);
    
    // Initialize NTP and initial time update
    initializeNTP();
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
    
    // Sync NTP every 5 minutes if connected
    if (current_time - _last_ntp_sync >= 300000) { // 5 minutes
        if (isWiFiConnected()) {
            initializeNTP();
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
        
        // Initialize NTP if not done yet
        if (!_ntp_initialized) {
            initializeNTP();
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

void ClockCard::initializeNTP() {
    if (!isWiFiConnected()) {
        Serial.println("WiFi not connected, skipping NTP init");
        return;
    }
    
    Serial.printf("Initializing NTP for %s (UTC%+d)\n", 
                  _timezone_config.name.c_str(), 
                  _timezone_config.utc_offset_hours);
    
    // Use direct GMT offset in seconds instead of POSIX strings
    // ESP32 configTime(gmtOffset_sec, daylightOffset_sec, server1, server2, server3)
    long gmtOffset_sec = _timezone_config.utc_offset_hours * 3600;
    long daylightOffset_sec = 0;  // Start with no DST, add later if needed
    
    // Adjust for current DST if needed (simplified - just for summer months)
    time_t now;
    time(&now);
    struct tm * timeinfo = gmtime(&now);
    bool is_summer = (timeinfo->tm_mon >= 3 && timeinfo->tm_mon <= 9); // Apr-Oct roughly
    
    if (_timezone_config.name == "New York" && is_summer) {
        daylightOffset_sec = 3600; // EDT is UTC-4, so add 1 hour to EST offset
    } else if (_timezone_config.name == "London" && is_summer) {
        daylightOffset_sec = 3600; // BST is UTC+1
    } else if (_timezone_config.name == "Los Angeles" && is_summer) {
        daylightOffset_sec = 3600; // PDT is UTC-7, so add 1 hour to PST offset
    } else if (_timezone_config.name == "Sydney" && !is_summer) {
        daylightOffset_sec = 3600; // AEDT in their summer (our winter)
    }
    
    Serial.printf("Using GMT offset: %ld sec, DST offset: %ld sec\n", gmtOffset_sec, daylightOffset_sec);
    
    // Configure NTP with direct offsets
    configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
    
    _ntp_initialized = true;
    _last_ntp_sync = lv_tick_get();
    
    Serial.printf("NTP configured with %ld second offset from UTC\n", gmtOffset_sec + daylightOffset_sec);
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