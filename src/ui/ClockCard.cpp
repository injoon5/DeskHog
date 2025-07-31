#include "ClockCard.h"
#include "Style.h"
#include "../fonts/fonts.h"
#include <time.h>
#include <sys/time.h>
#include <cstring>
#include <cstdio>

ClockCard::ClockCard(lv_obj_t* parent) 
    : _card(nullptr), _time_label(nullptr), _date_label(nullptr), 
      _ampm_label(nullptr), _left_container(nullptr), _last_update(0) {
    
    // Initialize time strings
    strcpy(_time_str, "12:00");
    strcpy(_date_str, "Jan 1");
    strcpy(_ampm_str, "AM");
    
    // Create main card container
    _card = lv_obj_create(parent);
    lv_obj_set_size(_card, 240, 135);
    lv_obj_set_style_bg_color(_card, lv_color_black(), 0);
    lv_obj_set_style_border_width(_card, 0, 0);
    lv_obj_set_style_pad_all(_card, 0, 0);
    lv_obj_set_style_radius(_card, 0, 0);
    
    // Create left container for date and AM/PM
    _left_container = lv_obj_create(_card);
    lv_obj_set_size(_left_container, 60, 135);
    lv_obj_set_pos(_left_container, 5, 0);
    lv_obj_set_style_bg_opa(_left_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_left_container, 0, 0);
    lv_obj_set_style_pad_all(_left_container, 0, 0);
    lv_obj_set_style_radius(_left_container, 0, 0);
    lv_obj_set_flex_flow(_left_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_left_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Create date label (top of left container)
    _date_label = lv_label_create(_left_container);
    lv_label_set_text(_date_label, _date_str);
    lv_obj_set_style_text_color(_date_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_date_label, &font_value, 0);
    lv_obj_set_style_text_align(_date_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Create AM/PM label (bottom of left container)
    _ampm_label = lv_label_create(_left_container);
    lv_label_set_text(_ampm_label, _ampm_str);
    lv_obj_set_style_text_color(_ampm_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_ampm_label, &font_value, 0);
    lv_obj_set_style_text_align(_ampm_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Create time label (large, positioned to the right)
    _time_label = lv_label_create(_card);
    lv_label_set_text(_time_label, _time_str);
    lv_obj_set_style_text_color(_time_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_time_label, &font_value_large, 0);
    lv_obj_set_style_text_align(_time_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Position time label to the right of the left container
    lv_obj_align_to(_time_label, _left_container, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    
    // Initial time update
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
    
    return true; // Continue receiving updates
}

void ClockCard::updateTime() {
    formatTime();
    formatDate();
    
    lv_label_set_text(_time_label, _time_str);
    lv_label_set_text(_date_label, _date_str);
    lv_label_set_text(_ampm_label, _ampm_str);
}

void ClockCard::formatTime() {
    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    
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