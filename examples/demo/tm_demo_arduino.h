// Input and debugging helpers shared by the Arduino examples. Header-only, so the
// host test (plain C) never compiles it.
#pragma once

#include <Arduino.h>
#include <esp_system.h>
#include <tinybutton.h>

#include "tinymenu.h"

static const char* const TM_DEMO_MSG_NAMES[] = {"up", "down", "left", "right", "click", "return",
                                                 "home", "prev", "next"};

// The UI the helpers below drive; set by tm_demo_begin().
inline tm_ui*& tm_demo_ui() {
    static tm_ui* ui = nullptr;
    return ui;
}

// Sends a message to the UI and logs it with its source, so a device report can be
// matched against what the UI actually received.
inline void tm_demo_send(tm_msg msg, const char* source) {
    Serial.printf("[%lu] %s -> %s\n", (unsigned long)millis(), source, TM_DEMO_MSG_NAMES[msg]);
    tm_ui_send(tm_demo_ui(), msg);
}

// A debounced, active-low push button.
//
// Without a long-press message the button fires on press and, if `repeat` is set,
// keeps firing while held (for scrolling). With one, a short press fires on release
// and a long press fires once while still held.
struct tm_demo_button {
    // `external_pullup`: GPIO34-39 on the ESP32 have no internal pull-up.
    tm_demo_button(uint8_t pin, const char* name, tm_msg short_msg, tm_msg long_msg = TM_MSG_NONE,
                   bool repeat = false, bool external_pullup = false)
        : pin(pin), name(name), short_msg(short_msg), long_msg(long_msg), repeat(repeat),
          external_pullup(external_pullup) {}

    uint8_t pin;
    const char* name;
    tm_msg short_msg;
    tm_msg long_msg;
    bool repeat;
    bool external_pullup;

    tb_button button = {};

    void begin() {
        pinMode(pin, external_pullup ? INPUT : INPUT_PULLUP);
        tb_config config = tb_default_config();
        config.debounce_ms = 15;
        config.click_ms = 0;
        config.long_press_ms = long_msg != TM_MSG_NONE ? 450 : (repeat ? 400 : 0);
        config.repeat_ms = repeat && long_msg == TM_MSG_NONE ? 110 : 0;
        tb_init(&button, &config);
        tb_set_callback(&button, on_event, this);
        tb_update(&button, digitalRead(pin) == LOW, millis());
    }

    void poll(uint32_t now) {
        tb_update(&button, digitalRead(pin) == LOW, now);
    }

  private:
    static void on_event(tb_button*, tb_event event, uint8_t, void* user) {
        tm_demo_button* self = static_cast<tm_demo_button*>(user);
        if (self->long_msg != TM_MSG_NONE) {
            if (event == TB_EVENT_CLICK) tm_demo_send(self->short_msg, self->name);
            if (event == TB_EVENT_LONG_PRESS) tm_demo_send(self->long_msg, self->name);
        } else if (event == TB_EVENT_PRESS ||
                   (self->repeat && (event == TB_EVENT_LONG_PRESS ||
                                     event == TB_EVENT_LONG_REPEAT))) {
            tm_demo_send(self->short_msg, self->name);
        }
    }
};

// Frame timing since the last report: how long tm_proc (drawing + flush) takes.
struct tm_demo_frame_stats {
    uint32_t frames = 0, total_us = 0, max_us = 0, since_ms = 0;
    uint32_t flushes = 0, flush_total_us = 0, flush_max_us = 0;
};
inline tm_demo_frame_stats& tm_demo_stats() {
    static tm_demo_frame_stats stats;
    return stats;
}

// Runs the UI at most every `frame_ms` and feeds the demo data. Animations advance by
// the real time since the previous frame, so a slow flush lowers the frame rate but
// not the speed of the motion.
inline void tm_demo_tick(uint32_t now, uint16_t frame_ms = 10) {
    static uint32_t last_frame = 0, last_wave = 0;
    if (now - last_wave >= 50) {
        last_wave = now;
        tm_demo_feed(now);
    }
    if (now - last_frame >= frame_ms) {
        const uint32_t elapsed = last_frame ? now - last_frame : frame_ms;
        last_frame = now;
        const uint32_t start = micros();
        tm_ui_proc(tm_demo_ui(), (uint16_t)(elapsed < 1000 ? elapsed : 1000));
        const uint32_t took = micros() - start;
        tm_demo_frame_stats& st = tm_demo_stats();
        st.frames++;
        st.total_us += took;
        if (took > st.max_us) st.max_us = took;
    }
}

// Examples call this from their flush callback so the report can split drawing from transfer.
inline void tm_demo_note_flush(uint32_t took_us) {
    tm_demo_frame_stats& st = tm_demo_stats();
    st.flushes++;
    st.flush_total_us += took_us;
    if (took_us > st.flush_max_us) st.flush_max_us = took_us;
}

// Prints frame timing since the previous report and starts a new window.
inline void tm_demo_print_stats() {
    tm_demo_frame_stats& st = tm_demo_stats();
    const uint32_t now = millis(), span = now - st.since_ms;
    Serial.printf("frames: %lu in %lu ms (%.1f fps), proc avg %.1f ms, max %.1f ms\n",
                  (unsigned long)st.frames, (unsigned long)span, span ? st.frames * 1000.0f / span : 0.0f,
                  st.frames ? st.total_us / 1000.0f / st.frames : 0.0f, st.max_us / 1000.0f);
    if (st.flushes)
        Serial.printf("flushes: %lu, avg %.1f ms, max %.1f ms\n", (unsigned long)st.flushes,
                      st.flush_total_us / 1000.0f / st.flushes, st.flush_max_us / 1000.0f);
    st = tm_demo_frame_stats();
    st.since_ms = now;
}

// Serial control: u d l r are the d-pad, . and , next and previous, e click, b back,
// p prints frame timing,
// s dumps the framebuffer as hex
// (page-major: bit y%8 of byte (y/8)*width + x), between FRAME_BEGIN <w> <h> and FRAME_END.
inline void tm_demo_serial_poll() {
    if (!Serial.available()) return;
    switch (Serial.read()) {
    case 'u': tm_demo_send(TM_MSG_UP, "serial"); break;
    case 'd': tm_demo_send(TM_MSG_DOWN, "serial"); break;
    case 'l': tm_demo_send(TM_MSG_LEFT, "serial"); break;
    case 'r': tm_demo_send(TM_MSG_RIGHT, "serial"); break;
    case '.': tm_demo_send(TM_MSG_NEXT, "serial"); break;
    case ',': tm_demo_send(TM_MSG_PREV, "serial"); break;
    case 'e': tm_demo_send(TM_MSG_CLICK, "serial"); break;
    case 'b': tm_demo_send(TM_MSG_RETURN, "serial"); break;
    case 'p': tm_demo_print_stats(); break;
    case 's': {
        const tm_fb& fb = tm_demo_ui()->fb;
        Serial.printf("FRAME_BEGIN %d %d\n", fb.w, fb.h);
        for (size_t i = 0; i < tm_fb_size(fb.w, fb.h); ++i) Serial.printf("%02x", fb.buf[i]);
        Serial.println("\nFRAME_END");
        break;
    }
    default: break;
    }
}

// Remembers the UI the helpers drive and prints a banner with the board and screen size.
inline void tm_demo_begin(tm_ui& ui, const char* board) {
    tm_demo_ui() = &ui;
    Serial.printf("tinymenu demo on %s, %dx%d (reset reason %d)\n", board, ui.fb.w, ui.fb.h,
                  (int)esp_reset_reason());
}
