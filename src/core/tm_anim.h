#ifndef TM_CORE_TM_ANIM_H_
#define TM_CORE_TM_ANIM_H_

#include "tm_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Eased motion of one coordinate toward a target: every step covers
 * (remaining distance) * elapsed / duration, an exponential approach whose
 * speed is set in milliseconds and does not depend on the frame rate.
 * Retargeting mid-way continues smoothly from the current position.
 */
typedef struct tm_anim {
    int16_t pos_cur;
    int16_t pos_tgt;
    int32_t acc; // remainder carried between steps, in distance * ms
} tm_anim;

// Advances by `elapsed_ms`; clears *done while the coordinate is still moving.
void tm_animation(tm_anim* anim, uint16_t duration_ms, uint16_t elapsed_ms, uint8_t* done);

#ifdef __cplusplus
}
#endif

#endif
