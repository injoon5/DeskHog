#include "WeatherCard.h"
#include "Style.h"
#include "../fonts/fonts.h"
#include <WiFi.h>

WeatherCard::WeatherCard(lv_obj_t* parent, const String& city_config) 
    : _card(nullptr), _temp_label(nullptr), _main_label(nullptr), 
      _feels_like_label(nullptr), _humidity_label(nullptr), 
      _left_container(nullptr), _right_container(nullptr), _error_label(nullptr),
      _city_config(city_config), _weatherClient(nullptr),
      _last_update(0), _error_shown(false), _has_data(false) {
    
    // Create main card container
    _card = lv_obj_create(parent);
    lv_obj_set_size(_card, 240, 135);
    lv_obj_set_style_bg_color(_card, lv_color_black(), 0);
    lv_obj_set_style_border_width(_card, 0, 0);
    lv_obj_set_style_pad_all(_card, 0, 0);
    lv_obj_set_style_radius(_card, 0, 0);
    
    // Create left container for temperature (width: 150)
    _left_container = lv_obj_create(_card);
    lv_obj_set_size(_left_container, 150, 135);
    lv_obj_set_pos(_left_container, 0, 0);
    lv_obj_set_style_bg_opa(_left_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_left_container, 0, 0);
    lv_obj_set_style_pad_all(_left_container, 0, 0);
    lv_obj_set_style_radius(_left_container, 0, 0);
    lv_obj_set_flex_flow(_left_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_left_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Create temperature label in left container
    _temp_label = lv_label_create(_left_container);
    lv_label_set_text(_temp_label, "--°C");
    lv_obj_set_style_text_color(_temp_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_temp_label, Style::largestValueFont(), 0);
    lv_obj_set_style_text_align(_temp_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Create right container for weather details (width: 90)
    _right_container = lv_obj_create(_card);
    lv_obj_set_size(_right_container, 90, 135);
    lv_obj_set_pos(_right_container, 150, 0);
    lv_obj_set_style_bg_opa(_right_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_right_container, 0, 0);
    lv_obj_set_style_pad_all(_right_container, 0, 0);
    lv_obj_set_style_radius(_right_container, 0, 0);
    lv_obj_set_flex_flow(_right_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_right_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(_right_container, 5, 0);
    
    // Create weather main label (e.g., "Clear")
    _main_label = lv_label_create(_right_container);
    lv_label_set_text(_main_label, "---");
    lv_obj_set_style_text_color(_main_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_main_label, Style::mediumValueFont(), 0);
    lv_obj_set_style_text_align(_main_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Create feels like label
    _feels_like_label = lv_label_create(_right_container);
    lv_label_set_text(_feels_like_label, "--°C");
    lv_obj_set_style_text_color(_feels_like_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_feels_like_label, Style::mediumValueFont(), 0);
    lv_obj_set_style_text_align(_feels_like_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Create humidity label
    _humidity_label = lv_label_create(_right_container);
    lv_label_set_text(_humidity_label, "--%");
    lv_obj_set_style_text_color(_humidity_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_humidity_label, Style::mediumValueFont(), 0);
    lv_obj_set_style_text_align(_humidity_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Create error label (initially hidden)
    _error_label = lv_label_create(_card);
    lv_label_set_text(_error_label, "Cannot fetch weather");
    lv_obj_set_style_text_color(_error_label, lv_color_make(255, 100, 100), 0);
    lv_obj_set_style_text_font(_error_label, &font_label, 0);
    lv_obj_set_style_text_align(_error_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(_error_label, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_add_flag(_error_label, LV_OBJ_FLAG_HIDDEN);
    
    // Request initial weather data
    requestWeatherUpdate();
}

WeatherCard::~WeatherCard() {
    // LVGL will handle cleanup when parent is deleted
}

bool WeatherCard::handleButtonPress(uint8_t button_index) {
    // Don't handle any button presses, allow navigation
    return false;
}

bool WeatherCard::update() {
    uint32_t current_time = lv_tick_get();
    
    // Update every 10 minutes (600000ms)
    if (current_time - _last_update >= 600000) {
        requestWeatherUpdate();
        _last_update = current_time;
    }
    
    return true; // Continue receiving updates
}

void WeatherCard::updateWeatherDisplay(const WeatherData& weatherData) {
    if (!weatherData.valid) {
        if (!_error_shown) {
            showError("Weather unavailable");
        }
        return;
    }
    
    if (_error_shown) {
        hideError();
    }
    
    // Update temperature
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "%.0f°C", weatherData.temperature);
    lv_label_set_text(_temp_label, temp_str);
    
    // Update weather main
    lv_label_set_text(_main_label, weatherData.main.c_str());
    
    // Update feels like
    char feels_str[24];
    snprintf(feels_str, sizeof(feels_str), "%.0f°C", weatherData.feels_like);
    lv_label_set_text(_feels_like_label, feels_str);
    
    // Update humidity
    char humidity_str[16];
    snprintf(humidity_str, sizeof(humidity_str), "%d%%", weatherData.humidity);
    lv_label_set_text(_humidity_label, humidity_str);
    
    _has_data = true;
}

void WeatherCard::showError(const String& message) {
    if (_error_label) {
        lv_label_set_text(_error_label, message.c_str());
        lv_obj_clear_flag(_error_label, LV_OBJ_FLAG_HIDDEN);
        _error_shown = true;
    }
}

void WeatherCard::hideError() {
    if (_error_label) {
        lv_obj_add_flag(_error_label, LV_OBJ_FLAG_HIDDEN);
        _error_shown = false;
    }
}

void WeatherCard::requestWeatherUpdate() {
    if (!_weatherClient) {
        Serial.println("WeatherCard: No weather client set");
        return;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        if (!_error_shown) {
            showError("No Connection");
        }
        return;
    }
    
    Serial.printf("WeatherCard: Requesting weather for %s\n", _city_config.c_str());
    
    WeatherData weatherData;
    if (_weatherClient->fetchWeatherData(_city_config, weatherData)) {
        Serial.printf("WeatherCard: Weather fetched successfully for %s: %.1f°C, %s\n", 
                      weatherData.city.c_str(), weatherData.temperature, weatherData.main.c_str());
        updateWeatherDisplay(weatherData);
    } else {
        Serial.printf("WeatherCard: Failed to fetch weather for %s\n", _city_config.c_str());
        WeatherData emptyData; // Will have valid = false
        updateWeatherDisplay(emptyData);
    }
}