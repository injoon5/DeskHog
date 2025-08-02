#pragma once

#include <Arduino.h>
#include <functional>
#include <lvgl.h>

/**
 * @brief Enum to uniquely identify each type of card available in the system
 */
enum class CardType {
    INSIGHT,      ///< PostHog insight visualization card
    FRIEND,       ///< Walking animation/encouragement card
    HELLO_WORLD,  ///< Simple hello world card
    FLAPPY_HOG,   ///< Flappy Hog game card
    QUESTION,     ///< Question trivia card
    PADDLE,       ///< Paddle game card
    CLOCK,        ///< Digital clock card
    WEATHER,      ///< Weather information card
    NOW_PLAYING,  ///< Now playing music card
    YEAR_PROGRESS, ///< Year progress visualization card
    TIMER,        ///< Timer countdown card
    STOPWATCH     ///< Stopwatch counting up card
    // New card types can be added here
};

/**
 * @brief Represents an instance of a configured card
 * 
 * This struct represents a card that has been added by the user and configured.
 * A list of these will be stored in persistent memory.
 */
struct CardConfig {
    CardType type;      ///< The type of card (enum value)
    String config;      ///< Configuration string (e.g., insight ID, animation speed)
    int order;          ///< Display order in the card stack
    String name;        ///< Human-readable name (e.g., "PostHog Insight", "Walking Animation")

    /**
     * @brief Default constructor
     */
    CardConfig() : type(CardType::INSIGHT), config(""), order(0), name("") {}
    
    /**
     * @brief Constructor with parameters
     */
    CardConfig(CardType t, const String& c, int o, const String& n) : type(t), config(c), order(o), name(n) {}
};

/**
 * @brief Represents an available type of card that a user can choose to add
 * 
 * These will be defined in CardController and represent the "menu" of 
 * card types that users can select from in the web UI.
 */
struct CardDefinition {
    CardType type;                  ///< The type of card this definition describes
    String name;                    ///< Human-readable name (e.g., "PostHog Insight", "Walking Animation")
    bool allowMultiple;             ///< Can the user add more than one of this card type?
    bool needsConfigInput;          ///< Does this card require a config value from the user?
    String configInputLabel;        ///< Label for config input field (e.g., "Insight ID", "Animation Speed")
    String uiDescription;           ///< Description shown to user in web UI
    
    // Factory function to create an instance of the card's UI
    std::function<lv_obj_t*(const String& configValue)> factory;
    
    /**
     * @brief Default constructor
     */
    CardDefinition() : type(CardType::INSIGHT), name(""), allowMultiple(false), 
                      needsConfigInput(false), configInputLabel(""), uiDescription("") {}
    
    /**
     * @brief Constructor with parameters (without factory function)
     */
    CardDefinition(CardType t, const String& n, bool multiple, bool needsConfig, 
                  const String& inputLabel, const String& description)
        : type(t), name(n), allowMultiple(multiple), needsConfigInput(needsConfig),
          configInputLabel(inputLabel), uiDescription(description) {}
};

/**
 * @brief Helper function to convert CardType enum to string
 * @param type The CardType to convert
 * @return String representation of the card type
 */
inline String cardTypeToString(CardType type) {
    switch (type) {
        case CardType::INSIGHT: return "INSIGHT";
        case CardType::FRIEND: return "FRIEND";
        case CardType::HELLO_WORLD: return "HELLO_WORLD";
        case CardType::FLAPPY_HOG: return "FLAPPY_HOG";
        case CardType::QUESTION: return "QUESTION";
        case CardType::PADDLE: return "PADDLE";
        case CardType::CLOCK: return "CLOCK";
        case CardType::WEATHER: return "WEATHER";
        case CardType::NOW_PLAYING: return "NOW_PLAYING";
        case CardType::YEAR_PROGRESS: return "YEAR_PROGRESS";
        case CardType::TIMER: return "TIMER";
        case CardType::STOPWATCH: return "STOPWATCH";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Helper function to convert string to CardType enum
 * @param str The string to convert
 * @return CardType enum value, defaults to INSIGHT if string not recognized
 */
inline CardType stringToCardType(const String& str) {
    if (str == "INSIGHT") return CardType::INSIGHT;
    if (str == "FRIEND") return CardType::FRIEND;
    if (str == "HELLO_WORLD") return CardType::HELLO_WORLD;
    if (str == "FLAPPY_HOG") return CardType::FLAPPY_HOG;
    if (str == "QUESTION") return CardType::QUESTION;
    if (str == "PADDLE") return CardType::PADDLE;
    if (str == "CLOCK") return CardType::CLOCK;
    if (str == "WEATHER") return CardType::WEATHER;
    if (str == "NOW_PLAYING") return CardType::NOW_PLAYING;
    if (str == "YEAR_PROGRESS") return CardType::YEAR_PROGRESS;
    if (str == "TIMER") return CardType::TIMER;
    if (str == "STOPWATCH") return CardType::STOPWATCH;
    return CardType::INSIGHT; // Default fallback
}

/**
 * @brief Timezone configuration helper for clock cards
 */
struct TimezoneConfig {
    String name;         ///< Display name (e.g., "Seoul", "New York")
    String timezone;     ///< POSIX timezone string (e.g., "KST-9", "EST5EDT,M3.2.0,M11.1.0")
    int utc_offset_hours; ///< UTC offset in hours for display purposes
    
    TimezoneConfig() : name("Seoul"), timezone("KST-9"), utc_offset_hours(9) {}
    TimezoneConfig(const String& n, const String& tz, int offset) 
        : name(n), timezone(tz), utc_offset_hours(offset) {}
};

/**
 * @brief Helper function to get timezone config from string
 * @param config_str Configuration string (timezone name)
 * @return TimezoneConfig struct with timezone details
 */
inline TimezoneConfig getTimezoneConfig(const String& config_str) {
    // Use simpler timezone offsets in seconds for ESP32 configTime
    if (config_str == "Seoul" || config_str == "") return TimezoneConfig("Seoul", "KST-9", 9);
    if (config_str == "New York") return TimezoneConfig("New York", "EST5EDT,M3.2.0,M11.1.0", -5);
    if (config_str == "London") return TimezoneConfig("London", "GMT0BST,M3.5.0,M10.5.0", 0);
    if (config_str == "Tokyo") return TimezoneConfig("Tokyo", "JST-9", 9);
    if (config_str == "Los Angeles") return TimezoneConfig("Los Angeles", "PST8PDT,M3.2.0,M11.1.0", -8);
    if (config_str == "Sydney") return TimezoneConfig("Sydney", "AEST-10AEDT,M10.1.0,M4.1.0", 10);
    
    // Default to Seoul
    return TimezoneConfig("Seoul", "KST-9", 9);
}