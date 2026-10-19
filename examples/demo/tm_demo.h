#ifndef TM_DEMO_H_
#define TM_DEMO_H_

#include "tinymenu.h"

#ifdef __cplusplus
extern "C" {
#endif

// Builds the demo menu tree on `ui` and makes the tile page its home page.
void tm_demo_init(tm_ui* ui);

// Generates all demo data from time: feeds the four Sensor waves, animates the Stats
// spectrum and advances About -> update progress. Call every 50 ms or so; no sensors needed.
void tm_demo_feed(uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif
