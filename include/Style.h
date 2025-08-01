#pragma once

#include "lvgl.h"
#include "fonts/fonts.h"
class Style {
private:
    static const lv_font_t* _label_font;
    static const lv_font_t* _value_font;
    static const lv_font_t* _medium_value_font;
    static const lv_font_t* _large_value_font;
    static const lv_font_t* _largest_value_font;
    static const lv_font_t* _loud_noises_font;
    static bool _fonts_initialized;

    // Initialize fonts - implemented in Style.cpp
    static void initFonts();

public:
    // Initialize all style resources
    static void init() {
        initFonts();
    }

    // Font getters
    static const lv_font_t* labelFont() {
        initFonts();
        return _label_font;
    }

    static const lv_font_t* valueFont() {
        initFonts();
        return _value_font;
    }

    static const lv_font_t* mediumValueFont() {
        initFonts();
        return _medium_value_font;
    }

    static const lv_font_t* largeValueFont() {
        initFonts();
        return _large_value_font;
    }

    static const lv_font_t* largestValueFont() {
        initFonts();
        return _largest_value_font;
    }
    
    static const lv_font_t* loudNoisesFont() {
        initFonts();
        return _loud_noises_font;
    }

    // Text colors
    static lv_color_t labelColor() {
        return lv_color_hex(0xAAAAAA);  // Dark gray for labels
    }

    static lv_color_t valueColor() {
        return lv_color_hex(0xFFFFFF);
    }

    // Accent color for highlights and indicators
    static lv_color_t accentColor() {
        return lv_color_hex(0x2980b9);  // Blue accent color
    }

    // Background colors
    static lv_color_t backgroundColor() {
        return lv_color_hex(0x000000);
    }

private:
    // Private constructor to prevent instantiation
    Style() {}
};

// Static member declarations are in the header
// Definitions will be in Style.cpp 