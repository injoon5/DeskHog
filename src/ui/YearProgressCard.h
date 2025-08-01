#pragma once

#include <lvgl.h>
#include "InputHandler.h"
#include "../config/CardConfig.h"

class YearProgressCard : public InputHandler {
public:
    YearProgressCard(lv_obj_t* parent, const String& timezone_config = "Seoul");
    ~YearProgressCard();
    
    lv_obj_t* getCard() const { return _card; }
    
    bool handleButtonPress(uint8_t button_index) override;
    bool update() override;
    void prepareForRemoval() override { _card = nullptr; }

private:
    void updateYearProgress();
    float calculateYearProgress();
    void initializeNTP();
    bool isWiFiConnected();
    
    lv_obj_t* _card;
    lv_obj_t* _year_label;
    lv_obj_t* _progress_bar;
    lv_obj_t* _top_rectangles[12];
    lv_obj_t* _bottom_rectangles[12];
    
    uint32_t _last_update;
    uint32_t _last_ntp_sync;
    int _current_year;
    TimezoneConfig _timezone_config;
    bool _ntp_initialized;
};