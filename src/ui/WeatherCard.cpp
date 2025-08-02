#include "WeatherCard.h"
#include "Style.h"
#include "../fonts/fonts.h"
#include "../hardware/Input.h"
#include <WiFi.h>

WeatherCard::WeatherCard(lv_obj_t* parent, EventQueue& eventQueue, const String& city_config) 
    : _event_queue(eventQueue), _card(nullptr), _temp_label(nullptr), _main_label(nullptr), 
      _feels_like_label(nullptr), _humidity_label(nullptr), 
      _left_container(nullptr), _right_container(nullptr), _error_label(nullptr),
      _city_name_label(nullptr),
      _city_config(city_config), _weatherClient(nullptr),
      _last_update(0), _error_shown(false), _has_data(false), _retry_count(0) {
    
    // Create main card container
    _card = lv_obj_create(parent);
    lv_obj_set_size(_card, 240, 135);
    lv_obj_set_style_bg_color(_card, lv_color_black(), 0);
    lv_obj_set_style_border_width(_card, 0, 0);
    lv_obj_set_style_pad_all(_card, 0, 0);
    lv_obj_set_style_radius(_card, 0, 0);
    
    // Create left container (width: 130)
    _left_container = lv_obj_create(_card);
    lv_obj_set_size(_left_container, 130, 135);
    lv_obj_set_pos(_left_container, 0, 0);
    lv_obj_set_style_bg_opa(_left_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_left_container, 0, 0);
    lv_obj_set_style_pad_all(_left_container, 2, 0);
    lv_obj_set_style_radius(_left_container, 0, 0);
    lv_obj_set_flex_flow(_left_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_left_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(_left_container, 0, 0);
    
    // Create city name label directly in left container
    _city_name_label = lv_label_create(_left_container);
    lv_label_set_text(_city_name_label, "--");
    lv_obj_set_style_text_color(_city_name_label, lv_color_hex(0xD8D8D8), 0);
    lv_obj_set_style_text_font(_city_name_label, Style::valueFont(), 0);
    lv_obj_set_style_text_align(_city_name_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Create temperature label directly in left container
    _temp_label = lv_label_create(_left_container);
    lv_label_set_text(_temp_label, "--°C");
    lv_obj_set_style_text_color(_temp_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(_temp_label, Style::largestValueFont(), 0);
    lv_obj_set_style_text_align(_temp_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Create right container for weather details (width: 110)
    _right_container = lv_obj_create(_card);
    lv_obj_set_size(_right_container, 110, 135);
    lv_obj_set_pos(_right_container, 130, 0);
    lv_obj_set_style_bg_opa(_right_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_right_container, 0, 0);
    lv_obj_set_style_pad_all(_right_container, 0, 0);
    lv_obj_set_style_radius(_right_container, 0, 0);
    lv_obj_set_flex_flow(_right_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_right_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(_right_container, 2, 0);
    
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
    
    // Subscribe to weather events
    _event_queue.subscribe([this](const Event& event) {
        if (event.type == EventType::WEATHER_DATA_RECEIVED && event.cardId == "weather") {
            this->onEvent(event);
        }
    });
    
    // Set initial display
    lv_label_set_text(_temp_label, "--°C");
    lv_label_set_text(_main_label, "---");

    // Request initial weather data
    requestWeatherUpdate();
}

WeatherCard::~WeatherCard() {
    // LVGL will handle cleanup when parent is deleted
}

bool WeatherCard::handleButtonPress(uint8_t button_index) {
    // Handle center button press to refresh data
    if (button_index == Input::BUTTON_CENTER) {
        Serial.println("WeatherCard: Center button pressed, refreshing data");
        _retry_count = 0; // Reset retry count for manual refresh
        requestWeatherUpdate();
        return true; // We handled the button press
    }
    
    return false; // Let CardNavigationStack handle navigation buttons
}

bool WeatherCard::update() {
    uint32_t now = millis();
    
    // Check if it's time to update
    if (now - _last_update >= UPDATE_INTERVAL) {
        requestWeatherUpdate();
        return true;
    }
    
    return false;
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
    
    // Update city name
    lv_label_set_text(_city_name_label, weatherData.city.c_str());
    
    // Update temperature with font switching based on length
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "%.0f°C", weatherData.temperature);
    lv_label_set_text(_temp_label, temp_str);
    
    // Switch font based on string length (if over 5 characters, use medium font)
    if (strlen(temp_str) > 5) {
        lv_obj_set_style_text_font(_temp_label, Style::mediumValueFont(), 0);
    } else {
        lv_obj_set_style_text_font(_temp_label, Style::largestValueFont(), 0);
    }
    
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
    if (_has_data) {
        return; // Don't show error if we have previous data
    }
    
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

void WeatherCard::onEvent(const Event& event) {
    if (event.type == EventType::WEATHER_DATA_RECEIVED) {
        if (event.success && !event.data.isEmpty()) {
            // Parse the JSON-like data to WeatherData structure
            WeatherData weatherData;
            // Simple parsing - in a real implementation you'd use ArduinoJson
            // For now, assume the data format is "city|temp|main|feels_like|humidity"
            int firstPipe = event.data.indexOf('|');
            int secondPipe = event.data.indexOf('|', firstPipe + 1);
            int thirdPipe = event.data.indexOf('|', secondPipe + 1);
            int fourthPipe = event.data.indexOf('|', thirdPipe + 1);
            
            if (firstPipe != -1 && secondPipe != -1 && thirdPipe != -1 && fourthPipe != -1) {
                weatherData.city = event.data.substring(0, firstPipe);
                weatherData.temperature = event.data.substring(firstPipe + 1, secondPipe).toFloat();
                weatherData.main = event.data.substring(secondPipe + 1, thirdPipe);
                weatherData.feels_like = event.data.substring(thirdPipe + 1, fourthPipe).toFloat();
                weatherData.humidity = event.data.substring(fourthPipe + 1).toInt();
                weatherData.valid = true;
                
                Serial.printf("WeatherCard: Weather data received: %s: %.1f°C, %s\n", 
                              weatherData.city.c_str(), weatherData.temperature, weatherData.main.c_str());
                updateWeatherDisplay(weatherData);
                hideError();
                _has_data = true;
            } else {
                Serial.println("WeatherCard: Invalid weather data format received");
                if (!_has_data) {
                    showError("Invalid data");
                }
            }
        } else {
            Serial.println("WeatherCard: Failed to receive weather data");
            if (!_has_data) {
                showError("Failed to fetch weather");
            }
        }
    }
}

void WeatherCard::requestWeatherUpdate() {
    // Check WiFi connection status
    if (!isWiFiConnected()) {
        if (_retry_count < MAX_RETRIES) {
            _retry_count++;
            Serial.printf("WeatherCard: WiFi not connected, retry %d/%d\n", _retry_count, MAX_RETRIES);
            
            // Show connection status
            if (!_has_data) {
                if (_retry_count == 1) {
                    lv_label_set_text(_temp_label, "--°C");
                    lv_label_set_text(_main_label, "Connecting...");
                } else {
                    String retryMsg = "Retry " + String(_retry_count) + "/" + String(MAX_RETRIES);
                    lv_label_set_text(_main_label, retryMsg.c_str());
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
    
    Serial.printf("WeatherCard: Requesting weather for %s\n", _city_config.c_str());
    
    // Show loading state
    if (!_has_data) {
        lv_label_set_text(_temp_label, "--°C");
        lv_label_set_text(_main_label, "---");
    }
    
    // Publish event to request weather data (will be handled by Core 0)
    Event weatherEvent = Event::createCardEvent(EventType::WEATHER_REQUEST, "weather", _city_config);
    _event_queue.publishEvent(weatherEvent);
    
    _last_update = millis(); // Update timestamp when request is made
}

bool WeatherCard::isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}