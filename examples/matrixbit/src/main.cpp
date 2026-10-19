// tinymenu demo on the Matrix:bit.
//
//   A   short: next      long: previous
//   B   short: click     long: back
//
// Serial: u d l r e b inject input, s dumps the frame (see tm_demo_arduino.h).

#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "tinymenu.h"
#include "tm_demo.h"
#include "tm_demo_arduino.h"

static const uint8_t I2C_SDA = 23, I2C_SCL = 22, OLED_ADDRESS = 0x3c;
static const uint8_t BUTTON_A = 0, BUTTON_B = 2;
static const int16_t SCREEN_W = 128, SCREEN_H = 64;
// The SSD1306 is specified for 400 kHz; this board runs cleanly at 1 MHz (no NACKs under
// load), which cuts a full frame from 25 ms to 11 ms. Drop it back if the panel glitches.
static const uint32_t I2C_HZ = 1000000;

// Adafruit_SSD1306 only initialises the panel; frames go out through flush() below.
static Adafruit_SSD1306 oled(SCREEN_W, SCREEN_H, &Wire, -1, I2C_HZ, I2C_HZ);
static uint8_t framebuffer[SCREEN_W * SCREEN_H / 8];

static tm_demo_button button_a(BUTTON_A, "A", TM_MSG_NEXT, TM_MSG_PREV);
static tm_demo_button button_b(BUTTON_B, "B", TM_MSG_CLICK, TM_MSG_RETURN);

static tm_ui ui;

// The SSD1306 RAM is page-major like the framebuffer, so the changed pages go out as they
// are: set the address window, then stream the data in transactions as large as the
// Wire buffer allows.
static void flush(const tm_fb* fb, int16_t first_page, int16_t end_page, void*) {
    const uint32_t start = micros();
    static const uint8_t window[] = {SSD1306_COLUMNADDR, 0, SCREEN_W - 1, SSD1306_PAGEADDR};
    Wire.beginTransmission(OLED_ADDRESS);
    Wire.write((uint8_t)0x00); // command stream
    Wire.write(window, sizeof(window));
    Wire.write((uint8_t)first_page);
    Wire.write((uint8_t)(end_page - 1));
    uint8_t err = Wire.endTransmission();

    const uint8_t* data = fb->buf + first_page * fb->w;
    size_t left = (size_t)(end_page - first_page) * fb->w;
    while (left) {
        const size_t n = left < I2C_BUFFER_LENGTH - 1 ? left : I2C_BUFFER_LENGTH - 1;
        Wire.beginTransmission(OLED_ADDRESS);
        Wire.write((uint8_t)0x40); // data stream
        Wire.write(data, n);
        err |= Wire.endTransmission();
        data += n;
        left -= n;
    }
    if (err) Serial.printf("[%lu] I2C error %u while flushing\n", (unsigned long)millis(), err);
    tm_demo_note_flush(micros() - start);
}

void setup() {
    Serial.begin(115200);
    Wire.begin(I2C_SDA, I2C_SCL, I2C_HZ);
    if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS, false, false))
        Serial.println("SSD1306 not found");
    button_a.begin();
    button_b.begin();

    tm_ui_init(&ui, framebuffer, SCREEN_W, SCREEN_H, flush, nullptr);
    tm_demo_init(&ui);
    tm_demo_begin(ui, "Matrix:bit");
}

void loop() {
    const uint32_t now = millis();
    button_a.poll(now);
    button_b.poll(now);
    tm_demo_serial_poll();
    tm_demo_tick(now);
}
