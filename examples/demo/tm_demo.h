#ifndef TM_DEMO_H_
#define TM_DEMO_H_

#include "tinymenu.h"

#ifdef __cplusplus
extern "C" {
#endif

// Builds the demo menu tree on `ui` and makes the tile page its home page.
void tm_demo_init(tm_ui* ui);

// Where the Sensor page's readings come from. A board fills in the ones it can read; the
// rest (and any reading that returns false) are simulated. Call before or after tm_demo_init.
typedef struct tm_demo_sensors {
    bool (*temperature)(int16_t* tenths_c); // degrees C in tenths
    bool (*light)(int16_t* percent);        // 0..100
    bool (*gpio)(int16_t* level);           // 0 or 1
    const char* gpio_name;                  // title of the GPIO wave, e.g. "GPIO34 (A)"
} tm_demo_sensors;
void tm_demo_set_sensors(const tm_demo_sensors* sensors);

// Advances the demo's simulated data: pushes one sample into each of the Sensor page's
// two waves and moves the About -> update progress along. Call it every 50 ms or so.
void tm_demo_feed(uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif
