#pragma once

#include <lvgl.h>
#include "InputHandler.h"
#include "../nowplaying/NowPlayingClient.h"
#include "EventQueue.h"

class NowPlayingCard : public InputHandler {
public:
    NowPlayingCard(lv_obj_t* parent, EventQueue& eventQueue, const String& username_config);
    ~NowPlayingCard();
    
    lv_obj_t* getCard() const { return _card; }
    
    bool handleButtonPress(uint8_t button_index) override;
    bool update() override;
    void prepareForRemoval() override { _card = nullptr; }
    
    void setNowPlayingClient(NowPlayingClient* client) { _nowPlayingClient = client; }

private:
    void onEvent(const Event& event);
    void updateNowPlayingDisplay(const NowPlayingData& data);
    void showError(const String& message);
    void hideError();
    void requestNowPlayingUpdate();
    bool isWiFiConnected();
    
    EventQueue& _event_queue;
    
    lv_obj_t* _card;
    lv_obj_t* _title_container;
    lv_obj_t* _album_container;
    lv_obj_t* _artist_container;
    lv_obj_t* _status_container;
    lv_obj_t* _title_label;
    lv_obj_t* _album_label;
    lv_obj_t* _artist_label;
    lv_obj_t* _status_label;
    lv_obj_t* _error_label;
    
    String _username_config;
    NowPlayingClient* _nowPlayingClient;
    
    uint32_t _last_update;
    bool _error_shown;
    bool _has_data;
    uint8_t _retry_count;
    
    static const uint32_t UPDATE_INTERVAL = 30000; // 30 sec in ms
    static const uint8_t MAX_RETRIES = 5;
};