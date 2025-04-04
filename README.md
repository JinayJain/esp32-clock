# CYD Clock

A multifunctional clock and information display for ESP32-based Cheap Yellow Display (CYD) hardware.

## Features

- Digital clock display
- Google Calendar integration for showing upcoming events
- Spotify integration for displaying current playback
- Weather forecast display
- LVGL-based user interface

## Hardware Requirements

- ESP32 development board
- Compatible display (ST7789 or ILI9341)
- XPT2046 Touchscreen

## Software Dependencies

- PlatformIO for development
- Arduino framework
- Libraries:
  - LVGL (v9.2.2)
  - TFT_eSPI (v2.5.34)
  - XPT2046_Touchscreen
  - ArduinoJson (v6.21.3)

## Setup

1. Clone the repository
2. Configure your WiFi credentials in `src/secrets.h`
3. Set up Google Calendar authentication in the `google-calendar-auth` directory
4. Build and upload with PlatformIO

## Development

This project uses FreeRTOS tasks to manage the GUI and application logic separately:

- GUI task runs on Core 0
- Application task runs on Core 1
