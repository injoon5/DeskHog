#pragma once

#include <Arduino.h>
#include <functional>
#include <vector>
#include <string>
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "posthog/parsers/InsightParser.h"

/**
 * @brief Event types in the system
 */
enum class EventType {
    INSIGHT_DATA_RECEIVED,
    INSIGHT_FORCE_REFRESH,
    WIFI_CREDENTIALS_FOUND,
    NEED_WIFI_CREDENTIALS,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_CONNECTION_FAILED,
    WIFI_AP_STARTED,
    OTA_PROCESS_START,
    OTA_PROCESS_END,
    CARD_CONFIG_CHANGED,
    CARD_TITLE_UPDATED,
    TIME_SYNC_REQUEST,
    TIME_SYNC_COMPLETE,
    WEATHER_REQUEST,
    WEATHER_DATA_RECEIVED,
    NOW_PLAYING_REQUEST,
    NOW_PLAYING_DATA_RECEIVED
};

/**
 * @brief Represents an event in the system
 */
struct Event {
    EventType type;                         // Type of event
    String insightId;                       // ID of the insight related to the event
    std::shared_ptr<InsightParser> parser;  // Optional parsed insight data
    String jsonData;                        // Raw JSON data for insights
    String title;                           // Title/name for card title updates
    String cardId;                          // Generic card identifier
    String data;                            // Generic data payload
    bool success;                           // Success/failure flag
    
    Event() : success(false) {}
    
    Event(EventType t, const String& id) : type(t), insightId(id), parser(nullptr), success(false) {}
    
    Event(EventType t, const String& id, std::shared_ptr<InsightParser> p)
        : type(t), insightId(id), parser(p), success(false) {}
        
    Event(EventType t, const String& id, const String& json)
        : type(t), insightId(id), parser(nullptr), jsonData(json), success(false) {}
        
    // Constructor for title update events
    static Event createTitleUpdateEvent(const String& id, const String& title_text) {
        Event e;
        e.type = EventType::CARD_TITLE_UPDATED;
        e.insightId = id;
        e.title = title_text;
        return e;
    }
    
    // Constructor for generic card events
    static Event createCardEvent(EventType eventType, const String& card_id, const String& payload = "", bool is_success = false) {
        Event e;
        e.type = eventType;
        e.cardId = card_id;
        e.data = payload;
        e.success = is_success;
        return e;
    }
};

/**
 * @brief Callback function type for event handlers
 */
using EventCallback = std::function<void(const Event&)>;

/**
 * @brief Thread-safe event queue for handling system events
 */
class EventQueue {
private:
    QueueHandle_t eventQueue;
    SemaphoreHandle_t callbackMutex;
    std::vector<EventCallback> eventCallbacks;
    
    static void eventProcessingTask(void* parameter);
    TaskHandle_t taskHandle;
    bool isRunning;
    
public:
    EventQueue(size_t queueSize = 10);
    ~EventQueue();
    
    /**
     * @brief Publish an event to the queue
     * 
     * @param eventType Type of the event
     * @param insightId ID of the insight related to the event
     * @return true if the event was successfully queued
     * @return false if the queue is full
     */
    bool publishEvent(EventType eventType, const String& insightId);
    
    /**
     * @brief Publish an event with parsed insight data
     * 
     * @param eventType Type of the event
     * @param insightId ID of the insight related to the event
     * @param parser Shared pointer to parsed insight data
     * @return true if the event was successfully queued
     * @return false if the queue is full
     */
    bool publishEvent(EventType eventType, const String& insightId, std::shared_ptr<InsightParser> parser);
    
    /**
     * @brief Publish an event with raw JSON data
     * 
     * @param eventType Type of the event
     * @param insightId ID of the insight related to the event
     * @param jsonData Raw JSON data string
     * @return true if the event was successfully queued
     * @return false if the queue is full
     */
    bool publishEvent(EventType eventType, const String& insightId, const String& jsonData);
    
    /**
     * @brief Alternative method to publish a pre-constructed Event
     * 
     * @param event The event to publish
     * @return true if the event was successfully queued
     * @return false if the queue is full
     */
    bool publishEvent(const Event& event);
    
    /**
     * @brief Publish method alias for publishing events
     * 
     * @param event The event to publish
     * @return true if the event was successfully queued
     */
    bool publish(const Event& event) { return publishEvent(event); }
    
    /**
     * @brief Subscribe to events
     * 
     * @param callback Function to call when an event is processed
     */
    void subscribe(EventCallback callback);
    
    /**
     * @brief Start the event processing task
     */
    void begin();
    
    /**
     * @brief Stop the event processing task
     */
    void end();
}; 