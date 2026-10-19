#include "core/tm_anim.h"

// Longest step considered at once; keeps |distance| * elapsed well inside int32.
#define TM_ANIM_MAX_STEP_MS 1000

void tm_animation(tm_anim* anim, uint16_t duration_ms, uint16_t elapsed_ms, uint8_t* done) {
    if (anim->pos_cur == anim->pos_tgt) {
        anim->acc = 0;
        return; // a finished animation leaves *done as it is
    }
    *done = false;
    if (elapsed_ms >= duration_ms || elapsed_ms > TM_ANIM_MAX_STEP_MS) {
        anim->pos_cur = anim->pos_tgt; // the whole animation fits in this step
        anim->acc = 0;
        return;
    }
    // Move by distance * elapsed / duration and carry the remainder, so the motion
    // takes the same wall time at any frame rate and never stalls short of the target.
    anim->acc += (int32_t)(anim->pos_tgt - anim->pos_cur) * elapsed_ms;
    const int32_t step = anim->acc / duration_ms;
    anim->pos_cur += (int16_t)step;
    anim->acc -= step * duration_ms;
}
