// The smallest useful tinymenu sketch: a settings list on a 128x64 SSD1306 over I2C,
// driven by three buttons wired to GND. Clicking "Brightness" opens a value popup.
//
// Needs the Adafruit SSD1306 library (it only talks to the panel here).

#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <tinymenu.h>

const uint8_t PIN_UP = 25, PIN_DOWN = 26, PIN_OK = 27; // long press OK: back
const int16_t SCREEN_W = 128, SCREEN_H = 64;

Adafruit_SSD1306 oled(SCREEN_W, SCREEN_H, &Wire, -1);
uint8_t framebuffer[SCREEN_W * SCREEN_H / 8];

tm_ui ui;
tm_list_page settings;
tm_value_popup brightness_popup;

tm_option options[] = {
    {.text = "Settings"},
    {.text = "Brightness", .kind = TM_OPTION_INT, .val = 80},
    {.text = "Sound", .kind = TM_OPTION_SWITCH, .val = 1},
    {.text = "Version", .kind = TM_OPTION_TEXT, .content = "0.1"},
};

// The SSD1306 buffer has the framebuffer's page-major layout: copy and send.
void flush(const tm_fb* fb, int16_t first_page, int16_t end_page, void*) {
    memcpy(oled.getBuffer() + first_page * fb->w, fb->buf + first_page * fb->w,
           (end_page - first_page) * fb->w);
    oled.display();
}

bool on_settings(const tm_page* page, tm_msg msg) {
    const tm_option* item = tm_menu_selected(page);
    if (msg == TM_MSG_CLICK && item == &options[1])
        tm_jump_to_page((tm_page_addr)page, &brightness_popup);
    // options[2].val follows the Sound switch: the list flips it on click.
    return false;
}

void setup() {
    Wire.begin();
    oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    pinMode(PIN_UP, INPUT_PULLUP);
    pinMode(PIN_DOWN, INPUT_PULLUP);
    pinMode(PIN_OK, INPUT_PULLUP);

    tm_ui_init(&ui, framebuffer, SCREEN_W, SCREEN_H, flush, nullptr);
    tm_list_page_init(&settings, &ui, sizeof(options) / sizeof(options[0]), options, TM_LIST_NORMAL,
                      on_settings);
    // Takes its text and value from the selected option and writes the value back.
    tm_value_popup_init(&brightness_popup, &ui, nullptr, 0, 0, 100, 5, true, true, nullptr);
}

// True once per press. Called every 10 ms, which is enough to ride out contact bounce.
bool pressed(uint8_t pin, bool& was_down) {
    const bool down = digitalRead(pin) == LOW;
    const bool edge = down && !was_down;
    was_down = down;
    return edge;
}

void loop() {
    static bool up_down, down_down, ok_down;
    static uint32_t ok_since, last_frame;
    const uint32_t now = millis();
    if (now - last_frame < 10)
        return;

    if (pressed(PIN_UP, up_down)) tm_ui_send(&ui, TM_MSG_PREV); // one axis: prev/next
    if (pressed(PIN_DOWN, down_down)) tm_ui_send(&ui, TM_MSG_NEXT);
    const bool ok_was_down = ok_down;
    if (pressed(PIN_OK, ok_down)) ok_since = now;
    if (ok_was_down && !ok_down) // released: short press clicks, long press goes back
        tm_ui_send(&ui, now - ok_since < 500 ? TM_MSG_CLICK : TM_MSG_RETURN);

    tm_ui_proc(&ui, now - last_frame); // animations follow the real elapsed time
    last_frame = now;
}
