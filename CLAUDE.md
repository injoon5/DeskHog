# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and Development Commands

This is a PlatformIO ESP32-S3 project. Use these commands:

- **Build**: `pio run` or use PlatformIO extension in VS Code
- **Upload**: `pio run --target upload`
- **Monitor**: `pio device monitor`
- **Clean**: `pio run --target clean`
- **Unit tests**: Currently disabled (see platformio.ini for native env)

If flashing doesn't work, use **Erase Flash and Upload** from PlatformIO Project Tasks menu.

## Architecture Overview

DeskHog is an ESP32-S3 embedded device with a 240x135 TFT display that shows PostHog insights and interactive cards.

### Critical Core/Task Architecture

**IMPORTANT**: This is a dual-core FreeRTOS system with strict task isolation:

**Core 0 (Protocol CPU):**
- WiFi management
- Web portal server  
- PostHog API calls and insight parsing
- NeoPixel LED control

**Core 1 (Application CPU):**
- LVGL graphics library tick
- UI rendering and input handling

**Cross-core communication MUST use EventQueue** - never call UI functions from Core 0 or network functions from Core 1, as this will crash the device.

### Key Components

- **EventQueue**: Thread-safe messaging between cores - use this for all cross-core communication
- **CardNavigationStack**: Manages UI card transitions and animations
- **CardController**: Manages card lifecycle and updates from web UI
- **ConfigManager**: Persistent storage for WiFi credentials and card configurations
- **CaptivePortal**: Web server for device configuration
- **PostHogClient**: API client for fetching insights

### Card System

The UI is a stack of cards users navigate with physical buttons. Cards are dynamically configured via web UI with persistent storage.

**Built-in Cards:**
- **ProvisioningCard**: Shows QR code for initial setup (always at top, cannot be removed)
- **InsightCard**: Displays PostHog data visualizations (numeric, funnel, line graph)
- **FriendCard**: Shows Max the hedgehog animations

**Card Configuration Architecture:**
- `CardConfig`: Represents a configured card instance (stored in preferences)
- `CardDefinition`: Represents an available card type that users can add
- `CardController`: Manages card registration, lifecycle, and responds to config changes
- Cards stored as JSON array in `ConfigManager._cardPrefs`

**Adding New Cards:**
1. Add to `CardType` enum in `CardController.h`
2. Create card class implementing LVGL UI
3. Register `CardDefinition` in `CardController::initializeCardTypes()` with:
   - `factory`: Lambda function to create card instances
   - `allowMultiple`: Whether users can add multiple instances
   - `needsConfigInput`: Whether card requires configuration input
   - `configInputLabel`: Label for config input field
4. For interactive/game cards, inherit from `InputHandler` for `update()` callbacks

**Web UI Card Management:**
- `/api/cards/definitions`: GET available card types
- `/api/cards/configured`: GET/POST current card configuration
- Supports drag-and-drop reordering, adding/removing cards

### File Structure

- `src/`: C++ source code
  - `ui/`: Card classes and UI components
  - `posthog/`: API client and insight parsing
  - `hardware/`: Display, WiFi, NeoPixel interfaces
  - `config/`: Configuration management
- `include/`: Headers, fonts, sprites
- `html/`: Web portal UI (gets inlined by build script)
- `raw-png/`: PNG sprites (converted to C arrays by build script)

## Development Guidelines

- Use `EventQueue` for all cross-core communication
- Only update UI from Core 1 (UI task)
- Follow existing snakeCase variable naming
- Maintain separation of concerns (no network code in UI components)
- Use existing pin definitions from board variant file, don't redefine pins
- Test thoroughly - embedded systems crash easily with threading violations

## Hardware Details

- **Board**: Adafruit ESP32-S3 Reverse TFT Feather
- **Display**: 240x135 TFT via ST7789 driver
- **Buttons**: Up/Down navigation (▲/▼), Center select (●)
- **LED**: NeoPixel for status indication
- **Battery**: Optional 350mAh LiPoly for ~10 hour runtime
- **Reset sequence**: Hold ▼ (Down/D0) + Reset to enter bootloader mode
- **Pin definitions**: Located in `~/.platformio/packages/framework-arduinoespressif32/variants/adafruit_feather_esp32s3_reversetft/pins_arduino.h`
- **Power off**: Hold ● + ▼ for 2 seconds to enter deep sleep
- **Wake up**: Press reset tab on left side

## Libraries Used

- **LVGL 9.2.2**: Graphics library for UI rendering and animations
- **ArduinoJson**: JSON parsing for API responses and config storage
- **AsyncTCP/ESPAsyncWebServer**: Async web server for captive portal
- **FastLED**: NeoPixel LED control
- **Adafruit ST7789**: Display driver for TFT screen
- **Bounce2**: Button debouncing library

## Build System Features

- **Font conversion**: `ttf2c.py` converts TTF fonts to LVGL C arrays
- **Sprite system**: `png2c.py` converts PNG images to LVGL-compatible C arrays
  - Place PNGs in `raw-png/subdirectory/` structure
  - Generates `subdirectory_sprites[]` arrays with `_count` variables
  - Supports sprite animations for cards/games
- **HTML inlining**: `htmlconvert.py` inlines web portal assets (100KB budget)
- **Partitioned storage**: Uses custom partitions.csv for OTA updates

## Web Portal

- **Budget**: 100KB total for all portal assets
- **Access**: Via QR code on first launch, then by IP shown in status
- **Preview**: Open `html/portal.html` in browser to preview changes
- **Constraints**: All assets must be local (works without WiFi)