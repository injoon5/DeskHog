#include "EventQueue.h"

EventQueue::EventQueue(size_t queueSize) : isRunning(false), taskHandle(nullptr) {
    // Create the event queue
    eventQueue = xQueueCreate(queueSize, sizeof(Event));
    
    // Create mutex for callback access
    callbackMutex = xSemaphoreCreateMutex();
}

EventQueue::~EventQueue() {
    // Stop the task if it's running
    end();
    
    // Clean up resources
    if (eventQueue) {
        vQueueDelete(eventQueue);
        eventQueue = nullptr;
    }
    
    if (callbackMutex) {
        vSemaphoreDelete(callbackMutex);
        callbackMutex = nullptr;
    }
}

bool EventQueue::publishEvent(EventType eventType, const String& insightId) {
    Event event(eventType, insightId);
    return publishEvent(event);
}

bool EventQueue::publishEvent(EventType eventType, const String& insightId, std::shared_ptr<InsightParser> parser) {
    Event event(eventType, insightId, parser);
    return publishEvent(event);
}

bool EventQueue::publishEvent(EventType eventType, const String& insightId, const String& jsonData) {
    // For large JSON data, we need to handle it carefully
    if (jsonData.length() > 8192) { // 8KB threshold
        Serial.printf("Large JSON detected (%u bytes), handling via event\n", jsonData.length());
    }
    
    Event event(eventType, insightId, jsonData);
    return publishEvent(event);
}

bool EventQueue::publishEvent(const Event& event) {
    // Add the event to the queue
    if (xQueueSend(eventQueue, &event, 0) == pdPASS) {
        return true;
    }
    return false;
}

void EventQueue::subscribe(EventCallback callback) {
    // Protect access to the callbacks vector
    if (xSemaphoreTake(callbackMutex, portMAX_DELAY) == pdTRUE) {
        eventCallbacks.push_back(callback);
        xSemaphoreGive(callbackMutex);
    }
}

void EventQueue::begin() {
    if (!isRunning) {
        isRunning = true;
        // Create a task to process events
        xTaskCreate(
            eventProcessingTask,
            "EventQueueTask",
            8192,           // Stack size increased for network operations
            this,           // Task parameter
            tskIDLE_PRIORITY + 1,  // Priority (adjust as needed)
            &taskHandle     // Task handle
        );
    }
}

void EventQueue::end() {
    if (isRunning && taskHandle != nullptr) {
        isRunning = false;
        
        // Wait a bit for any pending tasks
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // Delete the task
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
}

void EventQueue::eventProcessingTask(void* parameter) {
    EventQueue* self = static_cast<EventQueue*>(parameter);
    Event event;
    
    // Process events in a loop
    while (self->isRunning) {
        // Wait for an event (block until an event arrives)
        if (xQueueReceive(self->eventQueue, &event, pdMS_TO_TICKS(100)) == pdPASS) {
            // Process the event by calling all registered callbacks
            if (xSemaphoreTake(self->callbackMutex, portMAX_DELAY) == pdTRUE) {
                for (const auto& callback : self->eventCallbacks) {
                    callback(event);
                }
                xSemaphoreGive(self->callbackMutex);
            }
        }
        // Small delay to prevent CPU hogging
        vTaskDelay(1);
    }
    
    // Task cleanup
    vTaskDelete(NULL);
} 