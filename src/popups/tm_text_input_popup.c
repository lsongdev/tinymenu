#include <stdio.h>

#include "popups/tm_text_input_popup.h"
#include "core/tm_ui.h"

#define FONT Font_6_8
#define MARGIN 2              /* between the window and the screen's edge */
#define INSET (MARGIN + 2)    /* the window's frame and padding, above and below */
#define INSET_X (MARGIN + 3)  /* and at the sides */
#define CHAR_W 6    /* a Font_6_8 cell; the glyph is 5 px wide */
#define HEAD_H 10   /* title and count, when the screen is tall enough for them */
#define BOX_H 12    /* the text, underlined */
#define KEY_MIN_H 10
#define KEY_MIN_W 10
#define MAX_COLS 10

/* The built-in pages: the letters in either case, with the digits after them,
 * and the symbols. */
enum { PAGE_LOWER, PAGE_UPPER, PAGE_SYMBOLS, PAGE_COUNT };
static const char* const PAGES[PAGE_COUNT] = {
    "abcdefghijklmnopqrstuvwxyz0123456789",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
    "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~",
};
/* The action labels, shorter and shorter for narrower screens. */
static const char* const LABELS[][TM_TEXT_INPUT_ACTIONS] = {
    {"Aa", "#+", "Space", "Del", "OK"},
    {"Aa", "#+", "Spc", "Del", "OK"},
    {"Aa", "#", "_", "<", "OK"},
};
#define LABEL_SETS (sizeof(LABELS) / sizeof(LABELS[0]))

static bool printable(const char* text, size_t capacity, size_t* length) {
    for (size_t i = 0; i < capacity; i++) {
        unsigned char c = (unsigned char)text[i];
        if (!c) {
            *length = i;
            return true;
        }
        if (c < 32 || c > 126)
            return false;
    }
    return false;
}

//------- The keys: the page's characters (a charset's spaces are the Space key)
static const char* page_chars(const tm_text_input_popup* p) {
    return p->charset ? p->charset : PAGES[p->keys];
}
static size_t key_count(const tm_text_input_popup* p) {
    size_t n = 0;
    for (const char* s = page_chars(p); *s; s++)
        n += *s != ' ';
    return n;
}
static char key_char(const tm_text_input_popup* p, size_t index) {
    for (const char* s = page_chars(p); *s; s++)
        if (*s != ' ' && index-- == 0)
            return *s;
    return 0;
}
static bool allowed(const tm_text_input_popup* p, char c) {
    if (p->charset)
        return c && strchr(p->charset, c);
    if (c == ' ')
        return true;
    for (size_t i = 0; i < sizeof(PAGES) / sizeof(PAGES[0]); i++)
        if (c && strchr(PAGES[i], c))
            return true;
    return false;
}

//------- Layout, inside the window, from the screen size
static int16_t screen_h(const tm_text_input_popup* p) { return p->page.ui->fb.h; }
static int16_t inner_w(const tm_text_input_popup* p) { return p->page.ui->fb.w - 2 * INSET_X; }
static int16_t inner_h(const tm_text_input_popup* p) { return screen_h(p) - 2 * INSET; }
static uint8_t cols(const tm_text_input_popup* p) {
    return (uint8_t)MAX(MIN(MAX_COLS, inner_w(p) / KEY_MIN_W), 1);
}
static uint8_t grid_rows(const tm_text_input_popup* p) {
    return (uint8_t)((key_count(p) + cols(p) - 1) / cols(p));
}
static uint8_t rows(const tm_text_input_popup* p) { return grid_rows(p) + 1; } /* + actions */
/* Rows of the tallest page, so the keys keep their size from page to page. */
static uint8_t layout_rows(const tm_text_input_popup* p) {
    if (p->charset)
        return rows(p);
    size_t most = 0;
    for (uint8_t i = 0; i < PAGE_COUNT; i++)
        most = MAX(most, strlen(PAGES[i]));
    return (uint8_t)((most + cols(p) - 1) / cols(p) + 1);
}
static int16_t head_h(const tm_text_input_popup* p) { return inner_h(p) >= 40 ? HEAD_H : 0; }
static int16_t keys_top(const tm_text_input_popup* p) { return head_h(p) + BOX_H + 1; }
static int16_t keys_space(const tm_text_input_popup* p) { return inner_h(p) - keys_top(p); }
/* Rows share the room under the text, up to keys as tall as they are wide (a tall
 * screen leaves the rest); when that makes them too short, fewer show. */
static int16_t key_h(const tm_text_input_popup* p) {
    const int16_t square = MAX(inner_w(p) / cols(p), KEY_MIN_H);
    return MIN(MAX(keys_space(p) / layout_rows(p), KEY_MIN_H), square);
}
/* Height the keys take; the window shrinks to it and is centred. */
static int16_t keys_room(const tm_text_input_popup* p) {
    return MIN(keys_space(p), (int16_t)(layout_rows(p) * key_h(p)));
}
/* Space left above (and below) the window on a tall screen. */
static int16_t win_top(const tm_text_input_popup* p) { return (keys_space(p) - keys_room(p)) / 2; }
/* The action row stays at the bottom and only the grid scrolls, when there is room
 * for a row of each; on shorter screens the action row scrolls with the grid. */
static bool actions_fixed(const tm_text_input_popup* p) { return keys_room(p) >= 2 * key_h(p); }
/* Rows that scroll: the grid's, or all of them. */
static uint8_t scroll_rows(const tm_text_input_popup* p) {
    return actions_fixed(p) ? grid_rows(p) : rows(p);
}
static uint8_t visible_rows(const tm_text_input_popup* p) {
    const int16_t room = keys_room(p) - (actions_fixed(p) ? key_h(p) : 0);
    return (uint8_t)MAX(MIN(room / key_h(p), scroll_rows(p)), 1);
}
/* Top of a row, below the text (rows scrolled away are not drawn). */
static int16_t row_y(const tm_text_input_popup* p, uint8_t row) {
    if (actions_fixed(p) && row == grid_rows(p))
        return keys_room(p) - key_h(p);
    return (int16_t)((row - p->top_row) * key_h(p));
}
/* Characters in a grid row (the last may be short). */
static uint8_t row_len(const tm_text_input_popup* p, uint8_t row) {
    if (row >= grid_rows(p))
        return p->action_n;
    return (uint8_t)MIN((size_t)cols(p), key_count(p) - (size_t)row * cols(p));
}

/* An action's label: the longest set that leaves 4 px between them. */
static const char* label(const tm_text_input_popup* p, uint8_t i) {
    size_t set = 0;
    for (; set + 1 < LABEL_SETS; set++) {
        int16_t w = 0;
        for (uint8_t k = 0; k < p->action_n; k++)
            w += (int16_t)tm_text_width(LABELS[set][p->actions[k]], FONT) + 4;
        if (w <= inner_w(p))
            break;
    }
    return LABELS[set][p->actions[i]];
}

/* Left edge (inside the window) and width of a key; actions get their label's width
 * and a share of the rest. */
static void key_x(const tm_text_input_popup* p, uint8_t row, uint8_t col, int16_t* x, int16_t* w) {
    const int16_t sw = inner_w(p);
    if (row < grid_rows(p)) {
        *x = (int16_t)(col * sw / cols(p));
        *w = (int16_t)((col + 1) * sw / cols(p)) - *x;
        return;
    }
    int16_t labels = 0;
    for (uint8_t i = 0; i < p->action_n; i++)
        labels += (int16_t)tm_text_width(label(p, i), FONT);
    const int16_t extra = (sw - labels) / p->action_n;
    *x = 0;
    for (uint8_t i = 0;; i++) {
        *w = i == p->action_n - 1 ? sw - *x
                                  : (int16_t)tm_text_width(label(p, i), FONT) + extra;
        if (i == col)
            return;
        *x += *w;
    }
}

/* Keeps the selected row on screen (the fixed action row needs no scrolling) and
 * remembers the last key, for coming back from the actions. */
static void follow(tm_text_input_popup* p) {
    if (p->row < grid_rows(p))
        p->key_row = p->row, p->key_col = p->col;
    if (p->row >= scroll_rows(p))
        return;
    const uint8_t shown = visible_rows(p);
    if (p->row < p->top_row)
        p->top_row = p->row;
    if (p->row >= p->top_row + shown)
        p->top_row = p->row - shown + 1;
    if (p->top_row + shown > scroll_rows(p))
        p->top_row = scroll_rows(p) - shown;
}

static void set_actions(tm_text_input_popup* p) {
    p->action_n = 0;
    if (!p->charset) {
        p->actions[p->action_n++] = TM_TEXT_INPUT_CASE;
        p->actions[p->action_n++] = TM_TEXT_INPUT_SYMBOLS;
    }
    if (!p->charset || strchr(p->charset, ' '))
        p->actions[p->action_n++] = TM_TEXT_INPUT_SPACE;
    p->actions[p->action_n++] = TM_TEXT_INPUT_BACKSPACE;
    p->actions[p->action_n++] = TM_TEXT_INPUT_OK;
}

//------- Drawing
static void draw(tm_text_input_popup* p) {
    tm_ui* ui = p->page.ui;
    const int16_t iw = inner_w(p), ox = INSET_X, oy = INSET + win_top(p) + p->win_y.pos_cur;
    const int16_t win_w = ui->fb.w - 2 * MARGIN, win_h = screen_h(p) - 2 * MARGIN - 2 * win_top(p);
    tm_fb_set_color(&ui->fb, TM_PEN_BLACK);
    tm_canvas_draw_rbox(&ui->w_all, MARGIN, oy - INSET + MARGIN, win_w, win_h, tm_ui_radius(ui, 3));
    tm_fb_set_color(&ui->fb, TM_PEN_WHITE);
    tm_canvas_draw_rbox_empty(&ui->w_all, MARGIN, oy - INSET + MARGIN, win_w, win_h,
                              tm_ui_radius(ui, 3));
    tm_canvas c = {.fb = &ui->fb, .start_x = ox, .start_y = oy + 1, .w = iw, .h = 8};

    /* Title and length, or the discard prompt (in the text box on short screens). */
    char count[12];
    snprintf(count, sizeof(count), "%u/%u", (unsigned)p->length, (unsigned)(p->capacity - 1));
    if (head_h(p)) {
        if (p->discard_armed) {
            tm_canvas_draw_str(&c, 0, 0, FONT, "Back = discard");
        } else {
            const int16_t count_w = (int16_t)tm_text_width(count, FONT);
            tm_canvas_draw_str(&c, c.w - count_w, 0, FONT, count);
            tm_canvas title = c;
            title.w = c.w - count_w - CHAR_W;
            tm_canvas_draw_str(&title, 0, 0, FONT, p->title);
        }
    }

    /* The text's end with the cursor after it, over a line. */
    const int16_t box_y = oy + head_h(p);
    tm_canvas_draw_line_h(&ui->w_all, ox, ox + iw - 1, box_y + BOX_H - 2);
    c = (tm_canvas){.fb = &ui->fb, .start_x = ox + 2, .start_y = box_y + 1, .w = iw - 4, .h = 8};
    if (p->discard_armed && !head_h(p)) {
        tm_canvas_draw_str(&c, 0, 0, FONT, "Back = discard");
    } else {
        const size_t slots = (size_t)(c.w - 2) / CHAR_W; /* and the cursor after them */
        const size_t first = p->length > slots ? p->length - slots : 0;
        for (size_t i = first; i < p->length; i++)
            tm_canvas_draw_char(&c, (int16_t)((i - first) * CHAR_W), 0, FONT,
                                p->secret ? '*' : p->draft[i]);
        tm_canvas_draw_line_v(&c, (int16_t)((p->length - first) * CHAR_W), 0, 7);
        if (first)
            tm_canvas_draw_point(&ui->w_all, ox, box_y + 4);
    }

    /* The keys on screen: the scrolled rows, then the fixed action row. */
    const int16_t kh = key_h(p), top = oy + keys_top(p);
    for (uint8_t r = p->top_row; r <= rows(p) - 1; r++) {
        if (r >= p->top_row + visible_rows(p) && !(actions_fixed(p) && r == grid_rows(p)))
            continue;
        const int16_t ky = top + row_y(p, r);
        for (uint8_t k = 0; k < row_len(p, r); k++) {
            int16_t kx, kw;
            key_x(p, r, k, &kx, &kw);
            kx += ox;
            if (r < grid_rows(p)) {
                tm_canvas_draw_char(&ui->w_all, kx + (kw - (CHAR_W - 1)) / 2, ky + (kh - 8) / 2, FONT,
                                    key_char(p, (size_t)r * cols(p) + k));
            } else {
                const char* name = label(p, k);
                tm_canvas_draw_str(&ui->w_all, kx + (kw - (int16_t)tm_text_width(name, FONT)) / 2,
                                   ky + (kh - 8) / 2, FONT, name);
            }
        }
    }
    if (p->top_row) // more rows above / below: a dot in the padding
        tm_canvas_draw_point(&ui->w_all, ox + iw, top);
    if (p->top_row + visible_rows(p) < scroll_rows(p))
        tm_canvas_draw_point(&ui->w_all, ox + iw, top + visible_rows(p) * kh - 1);
}

//------- Page methods
static void entry(tm_page_addr addr) {
    tm_text_input_popup* p = addr;
    size_t length;
    /* Revalidate because the application may change buffer between entries. */
    if (!printable(p->buffer, p->capacity, &length)) {
        p->draft[0] = '\0';
        length = 0;
    } else {
        memcpy(p->draft, p->buffer, length + 1);
    }
    p->length = length;
    p->keys = PAGE_LOWER;
    p->row = p->col = p->top_row = p->key_row = p->key_col = 0;
    p->discard_armed = false;
    p->result = TM_TEXT_INPUT_EDITING;
    p->win_y.pos_cur = screen_h(p);
    p->win_y.pos_tgt = 0;
    p->page.ui->win_bg_blur = p->page.ui->params.winbgblur_param[TM_POPUP_BLUR_TEXT_INPUT];
}

static bool transition(tm_page_addr addr) {
    tm_text_input_popup* p = addr;
    tm_ui* ui = p->page.ui;
    tm_animation(&p->win_y, tm_ui_anim_ms(ui, TM_ANI_POPUP), ui->time, &ui->anim_is_finish);
    draw(p);
    return p->win_y.pos_cur == p->win_y.pos_tgt;
}

static void show(tm_page_addr addr) {
    tm_text_input_popup* p = addr;
    tm_ui* ui = p->page.ui;
    tm_page* bg = p->page.last_page;
    if (bg != &p->page)
        tm_page_show(bg);
    tm_fb_set_color(&ui->fb, TM_PEN_BLACK);
    tm_fb_blur(&ui->fb, ui->win_bg_blur);
    tm_fb_set_color(&ui->fb, TM_PEN_WHITE);
    draw(p);
}

//------- Selection
/* On the keys or on the actions. */
static bool on_actions(const tm_text_input_popup* p) { return p->row >= grid_rows(p); }

/* From the actions back to the keys, as they were left. */
static void to_keys(tm_text_input_popup* p) {
    if (!grid_rows(p))
        return;
    p->row = MIN(p->key_row, grid_rows(p) - 1);
    p->col = MIN(p->key_col, row_len(p, p->row) - 1);
    follow(p);
}

/* The d-pad: one key at a time, by rows and columns. */
void tm_text_input_popup_move(tm_text_input_popup* p, int8_t dr, int8_t dc) {
    if (dr) {
        int16_t x, w;
        key_x(p, p->row, p->col, &x, &w);
        const int16_t centre = x + w / 2; // the key under (or over) this one
        p->row = (uint8_t)((p->row + rows(p) + dr % rows(p)) % rows(p));
        p->col = 0;
        for (uint8_t k = 0; k < row_len(p, p->row); k++) {
            key_x(p, p->row, k, &x, &w);
            if (centre >= x)
                p->col = k;
        }
    }
    if (dc) {
        const uint8_t n = row_len(p, p->row);
        p->col = (uint8_t)((p->col + n + dc % n) % n);
    }
    follow(p);
}

void tm_text_input_popup_step(tm_text_input_popup* p, bool next) {
    if (on_actions(p)) { // along the actions; past either end, back to the keys
        if (next ? p->col + 1 >= p->action_n : p->col == 0)
            to_keys(p);
        else
            p->col += next ? 1 : -1;
        return;
    }
    const size_t n = key_count(p); // along this page's keys only
    const size_t at = ((size_t)p->row * cols(p) + p->col + (next ? 1 : n - 1)) % n;
    p->row = (uint8_t)(at / cols(p));
    p->col = (uint8_t)(at % cols(p));
    follow(p);
}

bool tm_text_input_popup_select_action(tm_text_input_popup* p, tm_text_input_action action) {
    for (uint8_t i = 0; i < p->action_n; i++)
        if (p->actions[i] == action) {
            p->row = grid_rows(p);
            p->col = i;
            follow(p);
            return true;
        }
    return false;
}

bool tm_text_input_popup_back(tm_text_input_popup* p) {
    if (on_actions(p))
        return false;
    return tm_text_input_popup_select_action(p, TM_TEXT_INPUT_OK); // done typing
}

bool tm_text_input_popup_select_char(tm_text_input_popup* p, char character) {
    if (character == ' ')
        return tm_text_input_popup_select_action(p, TM_TEXT_INPUT_SPACE);
    const uint8_t pages = p->charset ? 1 : PAGE_COUNT;
    const uint8_t from = p->keys;
    for (uint8_t i = 0; i < pages; i++) {
        p->keys = (uint8_t)((from + i) % pages); // the page shown first, if it has it
        for (size_t k = 0; k < key_count(p); k++)
            if (key_char(p, k) == character) {
                p->row = (uint8_t)(k / cols(p));
                p->col = (uint8_t)(k % cols(p));
                p->top_row = (uint8_t)MIN(p->top_row, MAX(scroll_rows(p) - visible_rows(p), 0));
                follow(p);
                return true;
            }
    }
    p->keys = from;
    return false;
}

/* Turns to another page and goes on typing there, from its first key. */
static void turn(tm_text_input_popup* p, uint8_t keys) {
    p->keys = keys;
    p->row = p->col = p->top_row = 0;
    follow(p);
}

//------- Editing
bool tm_text_input_popup_append(tm_text_input_popup* p, char character) {
    if (p->length >= p->capacity - 1 || !allowed(p, character))
        return false;
    p->draft[p->length++] = character;
    p->draft[p->length] = '\0';
    return true;
}

bool tm_text_input_popup_backspace(tm_text_input_popup* p) {
    if (!p->length)
        return false;
    p->draft[--p->length] = '\0';
    return true;
}

void tm_text_input_popup_finish(tm_text_input_popup* p, bool save) {
    if (save)
        memcpy(p->buffer, p->draft, p->length + 1);
    p->result = save ? TM_TEXT_INPUT_SAVED : TM_TEXT_INPUT_CANCELLED;
    p->discard_armed = false;
    p->win_y.pos_tgt = screen_h(p);
    tm_page_return(p);
}

bool tm_text_input_popup_activate(tm_text_input_popup* p) {
    if (!on_actions(p)) {
        tm_text_input_popup_append(p, key_char(p, (size_t)p->row * cols(p) + p->col));
        return false;
    }
    switch ((tm_text_input_action)p->actions[p->col]) {
    case TM_TEXT_INPUT_CASE: turn(p, p->keys == PAGE_LOWER ? PAGE_UPPER : PAGE_LOWER); break;
    case TM_TEXT_INPUT_SYMBOLS: turn(p, PAGE_SYMBOLS); break;
    case TM_TEXT_INPUT_SPACE: // and back to typing
        tm_text_input_popup_append(p, ' ');
        to_keys(p);
        break;
    case TM_TEXT_INPUT_BACKSPACE: tm_text_input_popup_backspace(p); break;
    case TM_TEXT_INPUT_OK: tm_text_input_popup_finish(p, true); return true;
    default: break;
    }
    return false;
}

/* Unsaved changes: the draft differs from what entry loaded. */
static bool changed(const tm_text_input_popup* p) {
    return strncmp(p->draft, p->buffer, p->capacity) != 0;
}

static bool react(tm_page_addr addr) {
    tm_text_input_popup* p = addr;
    tm_msg msg = tm_msg_que_read(&p->page.ui->msg_queue);
    bool done = false;
    if (p->page.auto_input) {
        bool armed = p->discard_armed;
        if (msg != TM_MSG_NONE)
            p->discard_armed = false;
        switch (msg) {
        case TM_MSG_PREV:
        case TM_MSG_NEXT: tm_text_input_popup_step(p, msg == TM_MSG_NEXT); break;
        case TM_MSG_UP:
        case TM_MSG_DOWN: tm_text_input_popup_move(p, msg == TM_MSG_DOWN ? 1 : -1, 0); break;
        case TM_MSG_LEFT:
        case TM_MSG_RIGHT: tm_text_input_popup_move(p, 0, msg == TM_MSG_RIGHT ? 1 : -1); break;
        case TM_MSG_CLICK: done = tm_text_input_popup_activate(p); break;
        case TM_MSG_RETURN:
            if (tm_text_input_popup_back(p)) { // done typing: to the actions, at OK
            } else if (armed || !changed(p)) {
                tm_text_input_popup_finish(p, false);
                done = true;
            } else {
                p->discard_armed = true;
            }
            break;
        default: break;
        }
        if (msg != TM_MSG_NONE && p->page.cb)
            p->page.cb(&p->page, msg);
    } else if (msg != TM_MSG_NONE && p->page.cb) {
        done = p->page.cb(&p->page, msg);
        if (done && p->result == TM_TEXT_INPUT_EDITING)
            tm_text_input_popup_finish(p, false);
    }
    return done;
}

static void indicator(tm_page_addr addr) {
    tm_text_input_popup* p = addr;
    tm_ui* ui = p->page.ui;
    int16_t x, w;
    key_x(p, p->row, p->col, &x, &w);
    ui->indicator.x.pos_tgt = INSET_X + x + 1;
    ui->indicator.y.pos_tgt = p->win_y.pos_tgt + INSET + win_top(p) + keys_top(p) + row_y(p, p->row);
    ui->indicator.w.pos_tgt = w - 2;
    ui->indicator.h.pos_tgt = key_h(p) - 1;
    tm_ui_draw_highlight(ui, 2);
}

static const tm_page_methods methods = {
    .in = transition, .in_para_init = entry, .show = show, .react = react,
    .indicator_ctrl = indicator, .scrollbar_ctrl = tm_func_do_nothing,
};

bool tm_text_input_popup_init(tm_text_input_popup* p, tm_ui* ui,
                              const char* title, char* buffer, char* draft,
                              size_t capacity, const char* charset, tm_page_cb cb) {
    size_t length, charset_len;
    if (!p || !ui || !buffer || !draft || !capacity || ui->fb.w < 64 ||
        ui->fb.h < 2 * INSET + BOX_H + 1 + KEY_MIN_H)
        return false;
    uintptr_t a = (uintptr_t)buffer, b = (uintptr_t)draft;
    if ((a <= b ? b - a : a - b) < capacity || !printable(buffer, capacity, &length))
        return false;
    if (charset && (!printable(charset, strlen(charset) + 1, &charset_len) || !charset_len))
        return false;
    memset(p, 0, sizeof(*p));
    p->page.page_type = TM_PAGE_TEXT_INPUT_POPUP;
    tm_page_init(p, ui, cb);
    p->page.methods = &methods;
    p->title = title ? title : "Text";
    p->buffer = buffer;
    p->draft = draft;
    p->capacity = capacity;
    p->charset = charset;
    p->result = TM_TEXT_INPUT_EDITING;
    set_actions(p);
    /* Make editing helpers usable immediately, as well as after entry. */
    memcpy(draft, buffer, length + 1);
    p->length = length;
    return true;
}
