#ifndef TM_POPUPS_TM_TEXT_INPUT_POPUP_H_
#define TM_POPUPS_TM_TEXT_INPUT_POPUP_H_

#include "core/tm_page.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The strip is a ring: the charset's characters, then these actions, which
 * sit between the last character and the first. */
typedef enum {
    TM_TEXT_INPUT_OK,        /* save and close */
    TM_TEXT_INPUT_LEFT,      /* move the cursor left */
    TM_TEXT_INPUT_RIGHT,     /* move the cursor right */
    TM_TEXT_INPUT_BACKSPACE, /* delete the character before the cursor */
    TM_TEXT_INPUT_ACTIONS,
} tm_text_input_action;

typedef enum {
    TM_TEXT_INPUT_EDITING,
    TM_TEXT_INPUT_SAVED,
    TM_TEXT_INPUT_CANCELLED,
} tm_text_input_result;

typedef struct {
    tm_page page;
    const char* title;
    char* buffer;
    char* draft;
    size_t capacity;
    const char* charset;
    size_t charset_len;
    size_t length;
    size_t cursor;   /* insertion point, 0..length */
    size_t selected; /* strip item: < charset_len is a character, else an action */
    size_t first_visible;
    tm_text_input_result result;
    bool secret; /* Mask draft characters; false by default. */
    bool discard_armed; /* return pressed once with unsaved changes */
    tm_anim win_y;
} tm_text_input_popup;

/* No allocation. buffer and draft must be distinct, non-overlapping arrays of
 * capacity bytes, kept alive with title and charset for the popup's lifetime.
 * buffer must contain a NUL-terminated printable ASCII string. charset is a
 * nonempty printable ASCII string; NULL selects letters, digits, space, -_.
 * Returns false for invalid arguments; popup is left untouched.
 * Each entry copies buffer into draft and puts the cursor at the end. Only OK
 * writes back to buffer.
 *
 * Works with next/prev/click/return alone (two buttons with long presses):
 * next/prev move along the strip, click inserts the selected character at the
 * cursor or runs the selected action. On a d-pad up/down turn the strip (up is
 * the next character) and left/right move the cursor directly. Return
 * cancels; with unsaved changes it asks for a second return first. Callbacks
 * see result after processing.
 */
bool tm_text_input_popup_init(tm_text_input_popup* popup, tm_ui* ui,
                              const char* title, char* buffer, char* draft,
                              size_t capacity, const char* charset, tm_page_cb cb);

/* Helpers for custom input handlers (auto_input = false). */
void tm_text_input_popup_select(tm_text_input_popup* popup, bool next);
/* Selects a character of the charset or an action; false if there is none. */
bool tm_text_input_popup_select_char(tm_text_input_popup* popup, char character);
void tm_text_input_popup_select_action(tm_text_input_popup* popup, tm_text_input_action action);
bool tm_text_input_popup_move_cursor(tm_text_input_popup* popup, bool right);
bool tm_text_input_popup_insert(tm_text_input_popup* popup, char character);
bool tm_text_input_popup_backspace(tm_text_input_popup* popup);
/* Click on the selected item. Returns true if that closed the popup (OK). */
bool tm_text_input_popup_activate(tm_text_input_popup* popup);
/* Call from react/callback; custom callbacks must return true after finishing. */
void tm_text_input_popup_finish(tm_text_input_popup* popup, bool save);

#ifdef __cplusplus
}
#endif
#endif
