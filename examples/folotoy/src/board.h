#pragma once

// FoloToy AI Passport, ESP32-C3 variant. Native USB uses GPIO18/19.
static constexpr int LCD_MOSI = 9;
static constexpr int LCD_SCLK = 8;
static constexpr int LCD_CS = 1;
static constexpr int LCD_DC = 20;
static constexpr int LCD_BACKLIGHT = 21;
static constexpr int LCD_WIDTH = 240;
static constexpr int LCD_HEIGHT = 320;
static constexpr int BUTTON_ADC = 0;

// A 240x256 viewport leaves the panel's rounded top/bottom corners clear.
static constexpr int UI_SCALE = 2;
static constexpr int UI_WIDTH = 120;
static constexpr int UI_HEIGHT = 128;
static constexpr int VIEWPORT_Y = (LCD_HEIGHT - UI_HEIGHT * UI_SCALE) / 2;
