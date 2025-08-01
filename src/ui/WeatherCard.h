#pragma once

#include <lvgl.h>
#include "InputHandler.h"
#include "../weather/WeatherClient.h"

class WeatherCard : public InputHandler {
public:
    WeatherCard(lv_obj_t* parent, const String& city_config = "Seoul");
    ~WeatherCard();
    
    lv_obj_t* getCard() const { return _card; }
    
    bool handleButtonPress(uint8_t button_index) override;
    bool update() override;
    void prepareForRemoval() override { _card = nullptr; }
    
    void setWeatherClient(WeatherClient* client) { _weatherClient = client; }

private:
    void updateWeatherDisplay(const WeatherData& weatherData);
    void showError(const String& message);
    void hideError();
    void requestWeatherUpdate();
    
    lv_obj_t* _card;
    lv_obj_t* _temp_label;
    lv_obj_t* _main_label;
    lv_obj_t* _feels_like_label;
    lv_obj_t* _humidity_label;
    lv_obj_t* _left_container;
    lv_obj_t* _right_container;
    lv_obj_t* _error_label;
    
    String _city_config;
    WeatherClient* _weatherClient;
    
    uint32_t _last_update;
    bool _error_shown;
    bool _has_data;
};