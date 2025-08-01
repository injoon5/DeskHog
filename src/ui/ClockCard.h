#pragma once

#include <lvgl.h>
#include "InputHandler.h"
#include "../config/CardConfig.h"
#include "EventQueue.h"

class ClockCard : public InputHandler {
public:
    ClockCard(lv_obj_t* parent, EventQueue& eventQueue, const String& timezone_config = "Seoul");
    ~ClockCard();
    
    lv_obj_t* getCard() const { return _card; }
    
    bool handleButtonPress(uint8_t button_index) override;
    bool update() override;
    void prepareForRemoval() override { _card = nullptr; }

private:
    void onEvent(const Event& event);
    void requestTimeSync();
    void updateTime();
    void formatTime();
    void formatDate();
    void formatWeekday();
    void showError(const String& message);
    void hideError();
    bool isWiFiConnected();
    void updateProgressBar();
    
    EventQueue& _event_queue;
    
    lv_obj_t* _card;
    lv_obj_t* _time_label;
    lv_obj_t* _date_label;
    lv_obj_t* _ampm_label;
    lv_obj_t* _weekday_label;
    lv_obj_t* _left_container;
    lv_obj_t* _error_label;
    lv_obj_t* _progress_bar;
    
    char _time_str[16];
    char _date_str[8];
    char _ampm_str[4];
    char _weekday_str[8];
    
    uint32_t _last_update;
    uint32_t _last_ntp_sync;
    TimezoneConfig _timezone_config;
    bool _ntp_initialized;
    bool _time_synced;
    bool _error_shown;
};