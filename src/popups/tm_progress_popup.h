#ifndef TM_POPUPS_TM_PROGRESS_POPUP_H_
#define TM_POPUPS_TM_PROGRESS_POPUP_H_

#include "core/tm_page.h"
#include "core/tm_progress.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TM_PROGRESS_RUNNING,
    TM_PROGRESS_DONE,      /* closed by tm_progress_popup_close() */
    TM_PROGRESS_CANCELLED, /* closed by return, when cancellable */
} tm_progress_result;

typedef struct {
    tm_page page;
    const char* text;
    tm_progress bar;
    tm_progress_result result;
    bool cancellable;
    bool close_requested;
    tm_anim win_y;
} tm_progress_popup;

/* Shows a loading or saving bar that the application drives, usually from
 * its own loop rather than from a callback:
 *
 *   tm_progress_popup_set_busy(&p);         // length unknown yet
 *   tm_jump_to_page(ui.current_page, &p);
 *   ...  tm_progress_popup_set(&p, done);   // 0..max as work proceeds
 *   tm_progress_popup_close(&p);
 *
 * max <= 0 starts in busy mode. Input does nothing except that return cancels
 * when cancellable; poll result to stop the work. Callbacks see every message.
 * Returns false for invalid arguments. text must outlive the popup.
 */
bool tm_progress_popup_init(tm_progress_popup* popup, tm_ui* ui, const char* text,
                            int32_t max, bool cancellable, tm_page_cb cb);
void tm_progress_popup_set(tm_progress_popup* popup, int32_t val);
void tm_progress_popup_set_max(tm_progress_popup* popup, int32_t max);
void tm_progress_popup_set_busy(tm_progress_popup* popup);
void tm_progress_popup_set_text(tm_progress_popup* popup, const char* text);
/* Closes on the next frame, with result TM_PROGRESS_DONE. */
void tm_progress_popup_close(tm_progress_popup* popup);

#ifdef __cplusplus
}
#endif
#endif
