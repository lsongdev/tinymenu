// tinymenu demo on the FoloToy AI Passport (ESP32-C3).
// UP/DOWN: previous/next, hold to repeat. OK: click, long OK: back.
// Panel initialization and key thresholds follow FoloToy/ai-passport-arduino
// (copyright 2026 FoloToy, MIT; see ../LICENSES/FoloToy-MIT.txt).

#include <LovyanGFX.hpp>
#include <driver/gpio.h>

#include "board.h"
#include "tm_demo.h"
#include "tm_demo_arduino.h"

// This ST7789P3 panel needs its board-specific porch, power and gamma settings.
class PassportPanel : public lgfx::Panel_ST7789 {
  protected:
    const uint8_t* getInitCommands(uint8_t list) const override {
        static const uint8_t commands[] = {
            0x11, CMD_INIT_DELAY, 120, // sleep out
            0xB0, 2, 0x00, 0xC0,     // RGB565 byte order expected by LovyanGFX
            0xB2, 5, 0x05, 0x05, 0x00, 0x33, 0x33,
            0xB7, 1, 0x35,
            0xBB, 1, 0x21,
            0xC0, 1, 0x2C,
            0xC2, 1, 0x01,
            0xC3, 1, 0x0B,
            0xC4, 1, 0x20,
            0xC6, 1, 0x0F,
            0xD0, 2, 0xA7, 0xA1,
            0xD0, 2, 0xA4, 0xA1,
            0xD6, 1, 0xA1,
            0xE0, 14, 0xD0, 0x04, 0x08, 0x0A, 0x09, 0x05, 0x2D,
                      0x43, 0x49, 0x09, 0x16, 0x15, 0x26, 0x2B,
            0xE1, 14 + CMD_INIT_DELAY, 0xD0, 0x03, 0x09, 0x0A, 0x0A, 0x06, 0x2E,
                                      0x44, 0x40, 0x3A, 0x15, 0x15, 0x26, 0x2A, 10,
            0x38, 0, // idle mode off
            0x29, 0, // display on
            0xFF, 0xFF,
        };
        return list == 0 ? commands : nullptr;
    }
};

class PassportLcd : public lgfx::LGFX_Device {
    PassportPanel panel;
    lgfx::Bus_SPI bus;

  public:
    PassportLcd() {
        auto b = bus.config();
        b.spi_host = SPI2_HOST;
        b.spi_mode = 0;
        b.freq_write = 40000000;
        b.pin_sclk = LCD_SCLK;
        b.pin_mosi = LCD_MOSI;
        b.pin_miso = -1;
        b.pin_dc = LCD_DC;
        bus.config(b);
        panel.setBus(&bus);

        auto p = panel.config();
        p.pin_cs = LCD_CS;
        p.pin_rst = -1; // panel reset is software only
        p.memory_width = p.panel_width = LCD_WIDTH;
        p.memory_height = p.panel_height = LCD_HEIGHT;
        p.rgb_order = true;
        p.invert = true;
        p.readable = false;
        p.bus_shared = false;
        panel.config(p);
        setPanel(&panel);
    }
};

static PassportLcd lcd;
static tm_ui ui;
static uint8_t framebuffer[UI_WIDTH * UI_HEIGHT / 8];
static const lgfx::swap565_t COLOR_ON(230, 240, 255), COLOR_OFF(16, 20, 28);
static constexpr int STRIP_ROWS = 8;
static uint16_t strip[LCD_WIDTH * STRIP_ROWS * UI_SCALE];

// Expand changed framebuffer pages to RGB565 in one small reusable strip.
static void flush(const tm_fb* fb, int16_t first_page, int16_t end_page, void*) {
    const uint32_t start = micros();
    lcd.startWrite();
    const int y_end = min(end_page * 8, (int)fb->h);
    for (int y = first_page * 8; y < y_end; y += STRIP_ROWS) {
        const int rows = min(STRIP_ROWS, y_end - y);
        tm_fb_expand16(fb, y, rows, UI_SCALE, COLOR_ON.raw, COLOR_OFF.raw, strip);
        lcd.pushImage(0, VIEWPORT_Y + y * UI_SCALE, LCD_WIDTH, rows * UI_SCALE,
                      (const lgfx::swap565_t*)strip);
    }
    lcd.endWrite();
    tm_demo_note_flush(micros() - start);
}

struct DemoButton {
    const char* name;
    tm_msg msg;
    tb_button button;
};
static DemoButton buttons[] = {{"up", TM_MSG_PREV, {}}, {"down", TM_MSG_NEXT, {}},
                                {"OK", TM_MSG_CLICK, {}}};

static void on_button(tb_button*, tb_event event, uint8_t, void* user) {
    const DemoButton* key = static_cast<DemoButton*>(user);
    if (key->msg == TM_MSG_CLICK) {
        if (event == TB_EVENT_CLICK) tm_demo_send(TM_MSG_CLICK, key->name);
        if (event == TB_EVENT_LONG_PRESS) tm_demo_send(TM_MSG_RETURN, key->name);
    } else if (event == TB_EVENT_PRESS || event == TB_EVENT_LONG_PRESS ||
               event == TB_EVENT_LONG_REPEAT) {
        tm_demo_send(key->msg, key->name);
    }
}

static void poll_buttons(uint32_t now) {
    static uint32_t last_poll;
    if (now - last_poll < 5) return;
    last_poll = now;
    // All three keys share a resistor ladder; released voltage is above 1900 mV.
    const uint32_t mv = analogReadMilliVolts(BUTTON_ADC);
    const int pressed = mv < 150 ? 0 : mv < 447 ? 1 : mv < 1900 ? 2 : -1;
    for (int i = 0; i < 3; i++) tb_update(&buttons[i].button, i == pressed, now);
}

void setup() {
    Serial.begin(115200);
    // Release display GPIO holds left by the board's previous deep-sleep firmware.
    gpio_deep_sleep_hold_dis();
    const int pins[] = {LCD_CS, LCD_SCLK, LCD_MOSI, LCD_DC, LCD_BACKLIGHT};
    for (int pin : pins) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, pin == LCD_CS ? HIGH : LOW);
        gpio_hold_dis((gpio_num_t)pin);
    }
    lcd.init();
    lcd.setRotation(0); // native portrait, RGB order, no mirroring
    lcd.fillScreen(0);
    digitalWrite(LCD_BACKLIGHT, HIGH);

    pinMode(BUTTON_ADC, INPUT); // external resistor ladder, no internal pull-up
    analogReadResolution(12);
    analogSetPinAttenuation(BUTTON_ADC, ADC_11db);
    for (DemoButton& key : buttons) {
        tb_config config = tb_default_config();
        config.debounce_ms = 25;
        config.click_ms = 0;
        config.long_press_ms = key.msg == TM_MSG_CLICK ? 450 : 400;
        config.repeat_ms = key.msg == TM_MSG_CLICK ? 0 : 110;
        tb_init(&key.button, &config);
        tb_set_callback(&key.button, on_button, &key);
    }

    tm_ui_init(&ui, framebuffer, UI_WIDTH, UI_HEIGHT, flush, nullptr);
    tm_demo_init(&ui);
    tm_demo_begin(ui, "FoloToy AI Passport");
}

void loop() {
    const uint32_t now = millis();
    poll_buttons(now);
    tm_demo_serial_poll();
    tm_demo_tick(now);
}
