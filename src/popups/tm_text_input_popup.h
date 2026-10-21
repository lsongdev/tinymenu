#ifndef TM_POPUPS_TM_TEXT_INPUT_POPUP_H_
#define TM_POPUPS_TM_TEXT_INPUT_POPUP_H_

#include "core/tm_page.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The keys of the bottom row, under the character grid. */
typedef enum {
    TM_TEXT_INPUT_CASE,      /* "Aa": lower <-> upper case letters (default charset only) */
    TM_TEXT_INPUT_SYMBOLS,   /* "#+": the symbols (default charset only) */
    TM_TEXT_INPUT_SPACE,     /* append a space and go on typing (when the charset has one) */
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
    uint8_t keys;    /* page of keys shown: 0 lower, 1 upper (with digits), 2 symbols */
    uint8_t row;     /* selected row; the grid's rows, then the action row */
    uint8_t col;     /* selected column, or action in the action row */
    uint8_t top_row; /* first row of the grid on screen when they do not all fit */
    uint8_t key_row, key_col; /* the last key selected, to come back to from the actions */
    uint8_t action_n;
    uint8_t actions[TM_TEXT_INPUT_ACTIONS]; /* the action row, left to right */
    tm_text_input_result result;
    bool secret; /* Mask draft characters; false by default. */
    bool discard_armed; /* return pressed once with unsaved changes */
    tm_anim win_y;
} tm_text_input_popup;

/* A keyboard in a window: the title, the text, edited at its end, a grid of
 * keys (no taller than wide; on a tall screen the window shrinks to them, centred)
 * and a fixed row of actions. No allocation. buffer and draft must be distinct,
 * non-overlapping arrays of capacity bytes, kept alive with title and charset
 * for the popup's lifetime. buffer must contain a NUL-terminated printable
 * ASCII string. charset is a nonempty printable ASCII string whose characters
 * (a space becomes the Space key) are the only keys; NULL gives three pages:
 * lower case and upper case letters, each with the digits, and symbols.
 * Returns false for invalid arguments; popup is left untouched.
 * Each entry copies buffer into draft. Only OK writes back to buffer.
 *
 * Next/prev go along the page's keys (two buttons), up/down/left/right move
 * over them (a d-pad), both wrapping; click types the key. Return goes to the
 * actions, on OK: next/prev or left/right move along them (next/prev past
 * either end go back to the keys) and click acts: Aa and #+ turn to their
 * page and go on typing, Space types a space and goes back to the keys, Del
 * deletes the last character. Return there cancels; with unsaved changes it
 * asks for a second return first. Callbacks see result after processing.
 */
bool tm_text_input_popup_init(tm_text_input_popup* popup, tm_ui* ui,
                              const char* title, char* buffer, char* draft,
                              size_t capacity, const char* charset, tm_page_cb cb);

/* Helpers for custom input handlers (auto_input = false). */
/* The d-pad: moves one key by rows and columns, wrapping. */
void tm_text_input_popup_move(tm_text_input_popup* popup, int8_t rows, int8_t cols);
/* next/prev: the next or previous key of the page, or action (past either end
 * of the actions, back to the keys). */
void tm_text_input_popup_step(tm_text_input_popup* popup, bool next);
/* return: from the keys to the actions, at OK; false if already there (where
 * return cancels). */
bool tm_text_input_popup_back(tm_text_input_popup* popup);
/* Selects a character's key, turning to its page; false if there is none. */
bool tm_text_input_popup_select_char(tm_text_input_popup* popup, char character);
/* Selects an action; false if the action row does not have it. */
bool tm_text_input_popup_select_action(tm_text_input_popup* popup, tm_text_input_action action);
bool tm_text_input_popup_append(tm_text_input_popup* popup, char character);
bool tm_text_input_popup_backspace(tm_text_input_popup* popup);
/* Click on the selected key or action. Returns true if that closed the popup (OK). */
bool tm_text_input_popup_activate(tm_text_input_popup* popup);
/* Call from react/callback; custom callbacks must return true after finishing. */
void tm_text_input_popup_finish(tm_text_input_popup* popup, bool save);

#ifdef __cplusplus
}
#endif
#endif
