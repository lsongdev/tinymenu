// tinymenu demo on the M5StickC.
//
//   B (side)            short: next      long: previous
//   A (front)           short: click     long: back
//   power key (short)   back
//
// Serial: u d l r e b inject input, p frame timing, s dumps the frame (see tm_demo_arduino.h).

#include <M5StickC.h>

#include "tinymenu.h"
#include "tm_demo.h"
#include "tm_demo_arduino.h"

static const uint8_t BUTTON_A = 37, BUTTON_B = 39; // input-only, external pull-ups

static const int16_t SCREEN_W = 160, SCREEN_H = 80; // landscape

static tm_ui ui;
static uint8_t framebuffer[SCREEN_W * SCREEN_H / 8];
static uint16_t pixels[SCREEN_W * SCREEN_H];

// Sends the rows of the changed pages, expanded to RGB565.
static void flush(const tm_fb* fb, int16_t first_page, int16_t end_page, void*) {
    const uint32_t start = micros();
    const int16_t y = first_page * 8, rows = min(end_page * 8, (int)fb->h) - y;
    tm_fb_expand16(fb, y, rows, 1, TFT_WHITE, TFT_BLACK, pixels);
    M5.Lcd.pushImage(0, y, fb->w, rows, pixels);
    tm_demo_note_flush(micros() - start);
}

static tm_demo_button button_a(BUTTON_A, "A", TM_MSG_CLICK, TM_MSG_RETURN, false, true);
static tm_demo_button button_b(BUTTON_B, "B", TM_MSG_NEXT, TM_MSG_PREV, false, true);

void setup() {
    M5.begin();
    M5.Lcd.setRotation(1);
    M5.Lcd.fillScreen(TFT_BLACK);
    button_a.begin();
    button_b.begin();

    tm_ui_init(&ui, framebuffer, SCREEN_W, SCREEN_H, flush, nullptr);
    tm_demo_init(&ui);
    tm_demo_begin(ui, "M5StickC");
}

void loop() {
    const uint32_t now = millis();
    button_a.poll(now);
    button_b.poll(now);
    if (M5.Axp.GetBtnPress() == 0x02) tm_demo_send(TM_MSG_RETURN, "power"); // short press
    tm_demo_serial_poll();
    tm_demo_tick(now);
}
