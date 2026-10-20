#ifndef TM_POPUPS_TM_TEXT_INPUT_POPUP_H_
#define TM_POPUPS_TM_TEXT_INPUT_POPUP_H_

#include "core/tm_page.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The keys of the bottom row, under the character grid. */
typedef enum {
    TM_TEXT_INPUT_CASE,      /* "Aa": lower <-> upper case letters (default charset only) */
    TM_TEXT_INPUT_SYMBOLS,   /* "#+": symbols <-> letters (default charset only) */
    TM_TEXT_INPUT_SPACE,     /* append a space (when the charset has one) */
    TM_TEXT_INPUT_BACKSPACE, /* delete the last character */
    TM_TEXT_INPUT_OK,        /* save and close */
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
    const char* charset; /* NULL: the built-in pages */
    size_t length;
    uint8_t keys;    /* page of keys shown: 0 lower, 1 upper, 2 symbols */
    uint8_t row;     /* selected row; the grid's rows, then the action row */
    uint8_t col;     /* selected column, or action in the action row */
    uint8_t top_row; /* first row on screen when they do not all fit */
    uint8_t action_n;
    uint8_t actions[TM_TEXT_INPUT_ACTIONS]; /* the action row, left to right */
    tm_text_input_result result;
    bool secret; /* Mask draft characters; false by default. */
    bool discard_armed; /* return pressed once with unsaved changes */
    tm_anim win_y;
} tm_text_input_popup;

/* A full-screen keyboard: the text, edited at its end, over a grid of keys
 * and a row of actions. No allocation. buffer and draft must be distinct,
 * non-overlapping arrays of capacity bytes, kept alive with title and charset
 * for the popup's lifetime. buffer must contain a NUL-terminated printable
 * ASCII string. charset is a nonempty printable ASCII string whose characters
 * (a space becomes the Space key) are the only keys; NULL gives three pages:
 * lower case letters with digits, upper case, and symbols.
 * Returns false for invalid arguments; popup is left untouched.
 * Each entry copies buffer into draft. Only OK writes back to buffer.
 *
 * Up/down/left/right move over the keys (wrapping), next/prev go through them
 * all in order, so two buttons with long presses are enough. Click types the
 * selected key. Return cancels; with unsaved changes it asks for a second
 * return first. Callbacks see result after processing.
 */
bool tm_text_input_popup_init(tm_text_input_popup* popup, tm_ui* ui,
                              const char* title, char* buffer, char* draft,
                              size_t capacity, const char* charset, tm_page_cb cb);

/* Helpers for custom input handlers (auto_input = false). */
/* Moves the selection by rows and columns, wrapping. */
void tm_text_input_popup_move(tm_text_input_popup* popup, int8_t rows, int8_t cols);
/* The next or previous key, through the grid and then the actions. */
void tm_text_input_popup_step(tm_text_input_popup* popup, bool next);
/* Selects a character's key, turning to its page; false if there is none. */
bool tm_text_input_popup_select_char(tm_text_input_popup* popup, char character);
/* Selects an action; false if the action row does not have it. */
bool tm_text_input_popup_select_action(tm_text_input_popup* popup, tm_text_input_action action);
bool tm_text_input_popup_append(tm_text_input_popup* popup, char character);
bool tm_text_input_popup_backspace(tm_text_input_popup* popup);
/* Click on the selected key. Returns true if that closed the popup (OK). */
bool tm_text_input_popup_activate(tm_text_input_popup* popup);
/* Call from react/callback; custom callbacks must return true after finishing. */
void tm_text_input_popup_finish(tm_text_input_popup* popup, bool save);

#ifdef __cplusplus
}
#endif
#endif
