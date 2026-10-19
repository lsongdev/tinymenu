// tinymenu demo on the XiaoMiao handheld.
//
//   up / down / left / right   move (hold to repeat)
//   A                          click
//   B                          back
//
// Serial: u d l r e b inject input, p frame timing, s dumps the frame (see tm_demo_arduino.h).

#include <LovyanGFX.hpp>

#include "board.h"
#include "tinymenu.h"
#include "tm_demo.h"
#include "tm_demo_arduino.h"

// ST7735 black tab, 128x160 panel used in landscape. Same setup as xiaomiao-shell's
// driver (RGB order, no inversion, no offsets), but SPI2 at 40 MHz instead of 20.
class XiaoMiaoLcd : public lgfx::LGFX_Device {
    lgfx::Panel_ST7735S panel;
    lgfx::Bus_SPI bus;

  public:
    XiaoMiaoLcd() {
        auto b = bus.config();
        b.spi_host = SPI2_HOST;
        b.spi_mode = 0;
        b.freq_write = 40000000; // a full frame takes 8.7 ms (17 ms at 20 MHz)
        b.pin_sclk = SPI_CLOCK;
        b.pin_mosi = SPI_MOSI;
        b.pin_miso = -1;
        b.pin_dc = TFT_DC;
        b.dma_channel = SPI_DMA_CH_AUTO;
        bus.config(b);
        panel.setBus(&bus);

        auto p = panel.config();
        p.pin_cs = TFT_CS;
        p.pin_rst = TFT_RESET;
        p.memory_width = 128;
        p.memory_height = 160;
        p.panel_width = 128;
        p.panel_height = 160;
        p.offset_x = 0;
        p.offset_y = 0;
        p.rgb_order = false;
        p.invert = false;
        p.readable = false;
        p.bus_shared = false;
        panel.config(p);
        setPanel(&panel);
    }
};

static XiaoMiaoLcd lcd;
static tm_ui ui;
static uint8_t framebuffer[LCD_WIDTH * LCD_HEIGHT / 8];

// LCD-like palette in byte-swapped RGB565, the order the panel takes on the wire.
static const lgfx::swap565_t COLOR_ON(230, 240, 255);
static const lgfx::swap565_t COLOR_OFF(16, 20, 28);

// Two strips: one is sent by DMA while the next is converted.
static const int STRIP_ROWS = 16;
static uint16_t strips[2][LCD_WIDTH * STRIP_ROWS];

// Sends the rows of the changed pages, expanded to RGB565.
static void flush(const tm_fb* fb, int16_t first_page, int16_t end_page, void*) {
    const uint32_t start = micros();
    const int y_end = min(end_page * 8, (int)fb->h);
    lcd.startWrite();
    for (int y0 = first_page * 8, n = 0; y0 < y_end; y0 += STRIP_ROWS, n ^= 1) {
        const int rows = min(STRIP_ROWS, y_end - y0);
        tm_fb_expand16(fb, y0, rows, 1, COLOR_ON.raw, COLOR_OFF.raw, strips[n]);
        lcd.waitDMA(); // the other strip, sent last time, must be done before it is reused
        lcd.pushImageDMA(0, y0, fb->w, rows, (const lgfx::swap565_t*)strips[n]);
    }
    lcd.waitDMA();
    lcd.endWrite();
    tm_demo_note_flush(micros() - start);
}

static tm_demo_button buttons[] = {
    {BUTTON_UP_PIN, "up", TM_MSG_UP, TM_MSG_NONE, true},
    {BUTTON_DOWN_PIN, "down", TM_MSG_DOWN, TM_MSG_NONE, true},
    {BUTTON_LEFT_PIN, "left", TM_MSG_LEFT, TM_MSG_NONE, true},
    {BUTTON_RIGHT_PIN, "right", TM_MSG_RIGHT, TM_MSG_NONE, true, true},
    {BUTTON_A_PIN, "A", TM_MSG_CLICK, TM_MSG_NONE, false, true},
    {BUTTON_B_PIN, "B", TM_MSG_RETURN},
};

void setup() {
    Serial.begin(115200);
    pinMode(SD_CS, OUTPUT); // the SD card shares the SPI bus: keep it deselected
    digitalWrite(SD_CS, HIGH);
    lcd.init();
    lcd.setRotation(1); // landscape, MADCTL MX|MV as in xiaomiao-shell
    for (tm_demo_button& b : buttons) b.begin();

    tm_ui_init(&ui, framebuffer, LCD_WIDTH, LCD_HEIGHT, flush, nullptr);
    tm_demo_init(&ui);
    tm_demo_begin(ui, "XiaoMiao");
}

void loop() {
    const uint32_t now = millis();
    for (tm_demo_button& b : buttons) b.poll(now);
    tm_demo_serial_poll();
    tm_demo_tick(now);
}
