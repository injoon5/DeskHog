#pragma once

#include <lvgl.h>
#include "InputHandler.h"

class TimerCard : public InputHandler {
public:
    TimerCard(lv_obj_t* parent);
    ~TimerCard();
    
    lv_obj_t* getCard() const { return _card; }
    
    bool handleButtonPress(uint8_t button_index) override;
    bool update() override;
    void prepareForRemoval() override { _card = nullptr; }

private:
    void updateDisplay();
    void formatTime();
    void decrementTimer();
    
    lv_obj_t* _card;
    lv_obj_t* _left_container;
    lv_obj_t* _right_container;
    lv_obj_t* _time_label;
    lv_obj_t* _left_text_label;
    lv_obj_t* _add_1m_label;
    lv_obj_t* _add_10m_label;
    lv_obj_t* _clear_label;
    
    char _time_str[8];  // mm:ss format
    
    uint32_t _total_seconds;
    uint32_t _last_update;
    uint32_t _last_button_press;
    uint8_t _button_press_count;
    
    static const uint32_t BUTTON_DOUBLE_PRESS_TIMEOUT = 500; // 500ms for double press
};