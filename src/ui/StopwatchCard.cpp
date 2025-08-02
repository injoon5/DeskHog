#include "StopwatchCard.h"
#include "Style.h"
#include <Arduino.h>
#include <cstring>
#include <cstdio>

StopwatchCard::StopwatchCard(lv_obj_t* parent) 
    : _card(nullptr), _left_container(nullptr), _right_container(nullptr),
      _time_label(nullptr), _start_stop_label(nullptr), _clear_label(nullptr),
      _total_seconds(0), _last_update(0), _last_button_press(0), _button_press_count(0), _is_running(false) {
    
    // Initialize time string
    strcpy(_time_str, "0:00");
    
    // Create main card container
    _card = lv_obj_create(parent);
    lv_obj_set_size(_card, 240, 135);
    lv_obj_set_style_bg_color(_card, lv_color_black(), 0);
    lv_obj_set_style_border_width(_card, 0, 0);
    lv_obj_set_style_pad_all(_card, 0, 0);
    lv_obj_set_style_radius(_card, 0, 0);
    
    // Create left container (180x135)
    _left_container = lv_obj_create(_card);
    lv_obj_set_size(_left_container, 180, 135);
    lv_obj_set_pos(_left_container, 0, 0);
    lv_obj_set_style_bg_opa(_left_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_left_container, 0, 0);
    lv_obj_set_style_pad_all(_left_container, 0, 0);
    lv_obj_set_style_radius(_left_container, 0, 0);
    lv_obj_set_flex_flow(_left_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_left_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Create time label in left container (centered)
    _time_label = lv_label_create(_left_container);
    lv_label_set_text(_time_label, _time_str);
    lv_obj_set_style_text_color(_time_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_time_label, Style::largestValueFont(), 0);
    lv_obj_set_style_text_align(_time_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Create right container (60x135)
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
    
    // Create "Start" label in right container
    _start_stop_label = lv_label_create(_right_container);
    lv_label_set_text(_start_stop_label, "Start");
    lv_obj_set_style_text_color(_start_stop_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_start_stop_label, Style::valueFont(), 0);
    lv_obj_set_style_text_align(_start_stop_label, LV_TEXT_ALIGN_RIGHT, 0);
    
    // Create "C" (clear) label in right container
    _clear_label = lv_label_create(_right_container);
    lv_label_set_text(_clear_label, "C");
    lv_obj_set_style_text_color(_clear_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_clear_label, Style::valueFont(), 0);
    lv_obj_set_style_text_align(_clear_label, LV_TEXT_ALIGN_RIGHT, 0);
    
    updateDisplay();
}

StopwatchCard::~StopwatchCard() {
    // LVGL will handle cleanup when parent is deleted
}

bool StopwatchCard::handleButtonPress(uint8_t button_index) {
    // Only handle center button (button_index == 1)
    if (button_index != 1) {
        return false; // Allow navigation with up/down buttons
    }
    
    uint32_t current_time = lv_tick_get();
    
    // Check if this is within the double-press timeout window
    if (current_time - _last_button_press <= BUTTON_DOUBLE_PRESS_TIMEOUT) {
        _button_press_count++;
        
        // For second press, just increment counter - don't execute action yet
        // Actions will be executed in the update() method after timeout
    } else {
        // First press or timeout passed, check if we had pending presses
        if (_button_press_count == 1) {
            // Process the previous single press as toggle start/stop
            toggleState();
        } else if (_button_press_count >= 2) {
            // Process the previous double press as clear
            clearTime();
        }
        
        // Start new press sequence
        _button_press_count = 1;
    }
    
    _last_button_press = current_time;
    return true; // Always handle center button
}

bool StopwatchCard::update() {
    uint32_t current_time = lv_tick_get();
    
    // Update every 1000ms (1 second)
    if (current_time - _last_update >= 1000) {
        // Check for pending button press timeout
        if (_button_press_count > 0 && 
            current_time - _last_button_press > BUTTON_DOUBLE_PRESS_TIMEOUT) {
            
            if (_button_press_count == 1) {
                // Process single press as toggle start/stop
                toggleState();
            } else if (_button_press_count >= 2) {
                // Process double press as clear
                clearTime();
            }
            
            _button_press_count = 0;
        }
        
        // Increment timer if running
        if (_is_running) {
            _total_seconds++;
            updateDisplay();
        }
        
        _last_update = current_time;
    }
    
    return true; // Continue receiving updates
}

void StopwatchCard::updateDisplay() {
    formatTime();
    lv_label_set_text(_time_label, _time_str);
    
    // Change color based on state
    if (!_is_running && _total_seconds > 0) {
        // Stopped and not 0:00 - gray color
        lv_obj_set_style_text_color(_time_label, lv_color_hex(0xAAAAAA), 0);
    } else {
        // Running or at 0:00 - white color
        lv_obj_set_style_text_color(_time_label, lv_color_white(), 0);
    }
}

void StopwatchCard::formatTime() {
    uint32_t minutes = _total_seconds / 60;
    uint32_t seconds = _total_seconds % 60;
    
    // Format as mm:ss
    snprintf(_time_str, sizeof(_time_str), "%u:%02u", minutes, seconds);
}

void StopwatchCard::toggleState() {
    _is_running = !_is_running;
    
    // Update the start/stop label
    if (_is_running) {
        lv_label_set_text(_start_stop_label, "Stop");
    } else {
        lv_label_set_text(_start_stop_label, "Start");
    }
}

void StopwatchCard::clearTime() {
    _total_seconds = 0;
    _is_running = false;
    
    // Update labels
    lv_label_set_text(_start_stop_label, "Start");
    updateDisplay();
}