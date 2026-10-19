#pragma once
// XiaoMiao handheld (ESP32), pins as in xiaomiao-shell's BoardConfig.h.

// ST7735 black-tab display on SPI2; the SD slot shares the bus and stays deselected.
#define TFT_CS 5
#define TFT_DC 4
#define TFT_RESET 19
#define SPI_CLOCK 18
#define SPI_MOSI 23
#define SD_CS 22
#define LCD_WIDTH 160
#define LCD_HEIGHT 128

// Buttons are active low. GPIO34/35 are input-only and use external pull-ups.
#define BUTTON_UP_PIN 2
#define BUTTON_DOWN_PIN 13
#define BUTTON_LEFT_PIN 27
#define BUTTON_RIGHT_PIN 35
#define BUTTON_A_PIN 34
#define BUTTON_B_PIN 12

// Analog sensors on ADC1: light on GPIO36, an NTC thermistor (10k, beta 3950, to ground
// under a 10k series resistor) on GPIO39.
#define LIGHT_PIN 36
#define THERMISTOR_PIN 39
#define ADC_REFERENCE 3.3f
#define THERMISTOR_SERIES 10000.0f
#define THERMISTOR_R25 10000.0f
#define THERMISTOR_BETA 3950.0f
