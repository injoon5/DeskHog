#include "TimerCard.h"
#include "Style.h"
#include <Arduino.h>
#include <cstring>
#include <cstdio>

TimerCard::TimerCard(lv_obj_t* parent) 
    : _card(nullptr), _left_container(nullptr), _right_container(nullptr),
      _time_label(nullptr), _left_text_label(nullptr), _add_1m_label(nullptr), _add_10m_label(nullptr), _clear_label(nullptr),
      _total_seconds(0), _last_update(0), _last_button_press(0), _button_press_count(0) {
    
    // Initialize time string
    strcpy(_time_str, "0:00");
    
    // Create main card container
    _card = lv_obj_create(parent);
    lv_obj_set_size(_card, 240, 135);
    lv_obj_set_style_bg_color(_card, lv_color_black(), 0);
    lv_obj_set_style_border_width(_card, 0, 0);
    lv_obj_set_style_pad_all(_card, 0, 0);
    lv_obj_set_style_radius(_card, 0, 0);
    
    // Create left container (200x135)
    _left_container = lv_obj_create(_card);
    lv_obj_set_size(_left_container, 180, 135);
    lv_obj_set_pos(_left_container, 0, 0);
    lv_obj_set_style_bg_opa(_left_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_left_container, 0, 0);
    lv_obj_set_style_pad_all(_left_container, 0, 0);
    lv_obj_set_style_radius(_left_container, 0, 0);
    lv_obj_set_flex_flow(_left_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_left_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(_left_container, 5, 0);
    
    // Create time label in left container
    _time_label = lv_label_create(_left_container);
    lv_label_set_text(_time_label, _time_str);
    lv_obj_set_style_text_color(_time_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_time_label, Style::largestValueFont(), 0);
    lv_obj_set_style_text_align(_time_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Create "left" text label in left container
    _left_text_label = lv_label_create(_left_container);
    lv_label_set_text(_left_text_label, "left");
    lv_obj_set_style_text_color(_left_text_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_left_text_label, Style::valueFont(), 0);
    lv_obj_set_style_text_align(_left_text_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Create right container (50x135)
    _right_container = lv_obj_create(_card);
    lv_obj_set_size(_right_container, 60, 135);
    lv_obj_set_pos(_right_container, 180, 0);
    lv_obj_set_style_bg_opa(_right_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_right_container, 0, 0);
    lv_obj_set_style_pad_all(_right_container, 0, 0);
    lv_obj_set_style_radius(_right_container, 0, 0);
    lv_obj_set_flex_flow(_right_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_right_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(_right_container, 10, 0);
    
    // Create "+1m" label in right container
    _add_1m_label = lv_label_create(_right_container);
    lv_label_set_text(_add_1m_label, "+1m");
    lv_obj_set_style_text_color(_add_1m_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_add_1m_label, Style::valueFont(), 0);
    lv_obj_set_style_text_align(_add_1m_label, LV_TEXT_ALIGN_RIGHT, 0);
    
    // Create "+10m" label in right container
    _add_10m_label = lv_label_create(_right_container);
    lv_label_set_text(_add_10m_label, "+10m");
    lv_obj_set_style_text_color(_add_10m_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_add_10m_label, Style::valueFont(), 0);
    lv_obj_set_style_text_align(_add_10m_label, LV_TEXT_ALIGN_RIGHT, 0);
    
    // Create "C" (clear) label in right container
    _clear_label = lv_label_create(_right_container);
    lv_label_set_text(_clear_label, "C");
    lv_obj_set_style_text_color(_clear_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_clear_label, Style::valueFont(), 0);
    lv_obj_set_style_text_align(_clear_label, LV_TEXT_ALIGN_RIGHT, 0);
    
    updateDisplay();
}

TimerCard::~TimerCard() {
    // LVGL will handle cleanup when parent is deleted
}

bool TimerCard::handleButtonPress(uint8_t button_index) {
    // Only handle center button (button_index == 1)
    if (button_index != 1) {
        return false; // Allow navigation with up/down buttons
    }
    
    uint32_t current_time = lv_tick_get();
    
    // Check if this is within the multi-press timeout window
    if (current_time - _last_button_press <= BUTTON_DOUBLE_PRESS_TIMEOUT) {
        _button_press_count++;
        
        // For second and third press, just increment counter - don't execute action yet
        // Actions will be executed in the update() method after timeout
    } else {
        // First press or timeout passed, check if we had pending presses
        if (_button_press_count == 1) {
            // Process the previous single press as +1m
            _total_seconds += 60; // Add 1 minute
            updateDisplay();
        } else if (_button_press_count == 2) {
            // Process the previous double press as +10m
            _total_seconds += 10 * 60; // Add 10 minutes
            updateDisplay();
        } else if (_button_press_count >= 3) {
            // Process the previous triple press as clear
            _total_seconds = 0; // Clear timer
            updateDisplay();
        }
        
        // Start new press sequence
        _button_press_count = 1;
    }
    
    _last_button_press = current_time;
    return true; // Always handle center button
}

bool TimerCard::update() {
    uint32_t current_time = lv_tick_get();
    
    // Update every 1000ms (1 second)
    if (current_time - _last_update >= 1000) {
        // Check for pending button press timeout
        if (_button_press_count > 0 && 
            current_time - _last_button_press > BUTTON_DOUBLE_PRESS_TIMEOUT) {
            
            if (_button_press_count == 1) {
                // Process single press as +1m
                _total_seconds += 60; // Add 1 minute
            } else if (_button_press_count == 2) {
                // Process double press as +10m
                _total_seconds += 10 * 60; // Add 10 minutes
            } else if (_button_press_count >= 3) {
                // Process triple press as clear
                _total_seconds = 0; // Clear timer
            }
            
            _button_press_count = 0;
            updateDisplay();
        }
        
        // Decrement timer if running
        if (_total_seconds > 0) {
            decrementTimer();
        }
        
        _last_update = current_time;
    }
    
    return true; // Continue receiving updates
}

void TimerCard::updateDisplay() {
    formatTime();
    lv_label_set_text(_time_label, _time_str);
}

void TimerCard::formatTime() {
    uint32_t minutes = _total_seconds / 60;
    uint32_t seconds = _total_seconds % 60;
    
    // Format as mm:ss
    snprintf(_time_str, sizeof(_time_str), "%u:%02u", minutes, seconds);
}

void TimerCard::decrementTimer() {
    if (_total_seconds > 0) {
        _total_seconds--;
        updateDisplay();
        
        // Optional: Add visual or audio feedback when timer reaches 0
        if (_total_seconds == 0) {
            // Timer finished
        }
    }
}