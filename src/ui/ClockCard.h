#pragma once

#include <lvgl.h>
#include "InputHandler.h"

class ClockCard : public InputHandler {
public:
    ClockCard(lv_obj_t* parent);
    ~ClockCard();
    
    lv_obj_t* getCard() const { return _card; }
    
    bool handleButtonPress(uint8_t button_index) override;
    bool update() override;
    void prepareForRemoval() override { _card = nullptr; }

private:
    void updateTime();
    void formatTime();
    void formatDate();
    
    lv_obj_t* _card;
    lv_obj_t* _time_label;
    lv_obj_t* _date_label;
    lv_obj_t* _ampm_label;
    lv_obj_t* _left_container;
    
    char _time_str[16];
    char _date_str[8];
    char _ampm_str[4];
    
    uint32_t _last_update;
};