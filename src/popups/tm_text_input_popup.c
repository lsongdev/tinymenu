#include <stdio.h>

#include "popups/tm_text_input_popup.h"
#include "core/tm_ui.h"

/* Three 8px rows with room for the border, including on 96x32 displays. */
#define TEXT_H 32
#define TEXT_CELL 6  /* text row: Font_6_8 cells, the cursor between them */
#define STRIP_CELL 8 /* strip: wider cells so the selection stands apart */
#define TEXT_Y 12
#define STRIP_Y 22
static const char DEFAULT_CHARSET[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -_.";

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

static int16_t popup_width(const tm_text_input_popup* p) {
    return MIN(160, p->page.ui->fb.w - 4);
}

/* Width inside the border and padding. */
static int16_t inner_width(const tm_text_input_popup* p) {
    return popup_width(p) - 6;
}

/* Characters the text row shows; the cursor may also sit just after the last. */
static size_t text_slots(const tm_text_input_popup* p) {
    return (size_t)(inner_width(p) - 1) / TEXT_CELL;
}

static size_t strip_items(const tm_text_input_popup* p) {
    return p->charset_len + TM_TEXT_INPUT_ACTIONS;
}

/* An odd number of strip cells, so the selection sits in the middle. */
static size_t strip_slots(const tm_text_input_popup* p) {
    size_t n = MIN((size_t)inner_width(p) / STRIP_CELL, strip_items(p));
    return n & 1 ? n : n - 1;
}

static int16_t strip_left(const tm_text_input_popup* p) {
    return (int16_t)((inner_width(p) - (int16_t)strip_slots(p) * STRIP_CELL) / 2);
}

/* Keeps the cursor in view without leaving blank cells after the text. */
static void clamp_scroll(tm_text_input_popup* p) {
    size_t slots = text_slots(p);
    if (p->cursor < p->first_visible)
        p->first_visible = p->cursor;
    if (p->cursor - p->first_visible > slots)
        p->first_visible = p->cursor - slots;
    if (p->length - p->first_visible < slots)
        p->first_visible = p->length > slots ? p->length - slots : 0;
}

/* Action icons and the space, drawn in a 5x7 cell. */
static void draw_symbol(tm_canvas* c, int16_t x, int16_t y, size_t item, size_t charset_len) {
    if (item >= charset_len) {
        switch ((tm_text_input_action)(item - charset_len)) {
        case TM_TEXT_INPUT_OK:
            tm_canvas_draw_line(c, x, y + 3, x + 2, y + 5);
            tm_canvas_draw_line(c, x + 2, y + 5, x + 4, y + 1);
            break;
        case TM_TEXT_INPUT_LEFT:
        case TM_TEXT_INPUT_RIGHT:
            for (int16_t i = 0; i < 4; i++) {
                int16_t col = item - charset_len == TM_TEXT_INPUT_LEFT ? x + 1 + i : x + 4 - i;
                tm_canvas_draw_line_v(c, col, y + 3 - i, y + 3 + i);
            }
            break;
        case TM_TEXT_INPUT_BACKSPACE:
            tm_canvas_draw_line_h(c, x, x + 4, y + 3);
            tm_canvas_draw_line(c, x, y + 3, x + 2, y + 1);
            tm_canvas_draw_line(c, x, y + 3, x + 2, y + 5);
            break;
        default: break;
        }
    } else {
        tm_canvas_draw_line_h(c, x, x + 4, y + 6);
        tm_canvas_draw_point(c, x, y + 5);
        tm_canvas_draw_point(c, x + 4, y + 5);
    }
}

static void draw(tm_text_input_popup* p) {
    tm_ui* ui = p->page.ui;
    int16_t w = popup_width(p), x = (ui->fb.w - w) / 2;
    int16_t y = p->win_y.pos_cur;
    tm_fb_set_color(&ui->fb, TM_PEN_BLACK);
    tm_canvas_draw_rbox(&ui->w_all, x, y, w, TEXT_H, tm_ui_radius(ui, 2));
    tm_fb_set_color(&ui->fb, TM_PEN_WHITE);
    tm_canvas_draw_rbox_empty(&ui->w_all, x, y, w, TEXT_H, tm_ui_radius(ui, 2));
    tm_canvas c = {.fb = &ui->fb, .start_x = x + 3, .start_y = y + 2,
                   .w = inner_width(p), .h = 8};

    /* Title and length, or the discard prompt. */
    if (p->discard_armed) {
        tm_canvas_draw_str(&c, 0, 0, Font_6_8, "Back = discard");
    } else {
        char count[12];
        snprintf(count, sizeof(count), "%u/%u", (unsigned)p->length, (unsigned)(p->capacity - 1));
        int16_t count_w = (int16_t)tm_text_width(count, Font_6_8);
        tm_canvas_draw_str(&c, c.w - count_w, 0, Font_6_8, count);
        tm_canvas title = c;
        title.w = c.w - count_w - TEXT_CELL;
        tm_canvas_draw_str(&title, 0, 0, Font_6_8, p->title);
    }

    /* Text with the cursor between characters; text starts 1px in for it. */
    c.start_y = y + TEXT_Y - 1;
    c.h = 10;
    size_t slots = text_slots(p);
    for (size_t i = 0; i < slots && p->first_visible + i < p->length; i++)
        tm_canvas_draw_char(&c, (int16_t)(1 + i * TEXT_CELL), 1, Font_6_8,
                            p->secret ? '*' : p->draft[p->first_visible + i]);
    tm_canvas_draw_line_v(&c, (int16_t)((p->cursor - p->first_visible) * TEXT_CELL), 0, 9);
    if (p->first_visible)
        tm_canvas_draw_point(&c, 0, 9);
    if (p->length - p->first_visible > slots)
        tm_canvas_draw_point(&c, c.w - 1, 9);

    /* The strip, centred on the selection. */
    c.start_y = y + STRIP_Y;
    c.h = 8;
    size_t n = strip_items(p), shown = strip_slots(p);
    int16_t left = strip_left(p);
    for (size_t i = 0; i < shown; i++) {
        size_t item = (p->selected + n - shown / 2 + i) % n;
        int16_t cx = left + (int16_t)(i * STRIP_CELL) + 1;
        if (item < p->charset_len && p->charset[item] != ' ')
            tm_canvas_draw_char(&c, cx, 0, Font_6_8, p->charset[item]);
        else
            draw_symbol(&c, cx, 0, item, p->charset_len);
    }
}

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
    p->length = p->cursor = length;
    p->first_visible = 0;
    p->selected = 0;
    p->discard_armed = false;
    p->result = TM_TEXT_INPUT_EDITING;
    clamp_scroll(p);
    p->win_y.pos_cur = -TEXT_H;
    p->win_y.pos_tgt = (p->page.ui->fb.h - TEXT_H) / 2;
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

void tm_text_input_popup_select(tm_text_input_popup* p, bool next) {
    size_t n = strip_items(p);
    p->selected = (p->selected + (next ? 1 : n - 1)) % n;
}

bool tm_text_input_popup_select_char(tm_text_input_popup* p, char character) {
    const char* at = character ? strchr(p->charset, character) : NULL;
    if (!at)
        return false;
    p->selected = (size_t)(at - p->charset);
    return true;
}

void tm_text_input_popup_select_action(tm_text_input_popup* p, tm_text_input_action action) {
    if (action < TM_TEXT_INPUT_ACTIONS)
        p->selected = p->charset_len + action;
}

bool tm_text_input_popup_move_cursor(tm_text_input_popup* p, bool right) {
    if (right ? p->cursor >= p->length : p->cursor == 0)
        return false;
    p->cursor += right ? 1 : (size_t)-1;
    clamp_scroll(p);
    return true;
}

bool tm_text_input_popup_insert(tm_text_input_popup* p, char character) {
    if (p->length >= p->capacity - 1 || !character || !strchr(p->charset, character))
        return false;
    memmove(p->draft + p->cursor + 1, p->draft + p->cursor, p->length - p->cursor + 1);
    p->draft[p->cursor++] = character;
    p->length++;
    clamp_scroll(p);
    return true;
}

bool tm_text_input_popup_backspace(tm_text_input_popup* p) {
    if (!p->cursor)
        return false;
    p->cursor--;
    memmove(p->draft + p->cursor, p->draft + p->cursor + 1, p->length - p->cursor);
    p->length--;
    clamp_scroll(p);
    return true;
}

void tm_text_input_popup_finish(tm_text_input_popup* p, bool save) {
    if (save)
        memcpy(p->buffer, p->draft, p->length + 1);
    p->result = save ? TM_TEXT_INPUT_SAVED : TM_TEXT_INPUT_CANCELLED;
    p->discard_armed = false;
    p->win_y.pos_tgt = -TEXT_H;
    tm_page_return(p);
}

bool tm_text_input_popup_activate(tm_text_input_popup* p) {
    if (p->selected < p->charset_len) {
        tm_text_input_popup_insert(p, p->charset[p->selected]);
        return false;
    }
    switch ((tm_text_input_action)(p->selected - p->charset_len)) {
    case TM_TEXT_INPUT_OK: tm_text_input_popup_finish(p, true); return true;
    case TM_TEXT_INPUT_LEFT: tm_text_input_popup_move_cursor(p, false); break;
    case TM_TEXT_INPUT_RIGHT: tm_text_input_popup_move_cursor(p, true); break;
    case TM_TEXT_INPUT_BACKSPACE: tm_text_input_popup_backspace(p); break;
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
        if (msg == TM_MSG_PREV || msg == TM_MSG_NEXT) {
            tm_text_input_popup_select(p, msg == TM_MSG_NEXT);
        } else if (msg == TM_MSG_UP || msg == TM_MSG_DOWN) { // like a wheel: up is the next
            tm_text_input_popup_select(p, msg == TM_MSG_UP);
        } else if (msg == TM_MSG_LEFT || msg == TM_MSG_RIGHT) {
            tm_text_input_popup_move_cursor(p, msg == TM_MSG_RIGHT);
        } else if (msg == TM_MSG_CLICK) {
            done = tm_text_input_popup_activate(p);
        } else if (msg == TM_MSG_RETURN) {
            if (armed || !changed(p)) {
                tm_text_input_popup_finish(p, false);
                done = true;
            } else {
                p->discard_armed = true;
            }
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
    int16_t x = (ui->fb.w - popup_width(p)) / 2 + 3;
    int16_t y = (ui->fb.h - TEXT_H) / 2;
    ui->indicator.x.pos_tgt = x + strip_left(p) + (int16_t)(strip_slots(p) / 2 * STRIP_CELL);
    ui->indicator.y.pos_tgt = y + STRIP_Y - 1;
    ui->indicator.w.pos_tgt = STRIP_CELL - 1;
    ui->indicator.h.pos_tgt = 9;
    tm_ui_draw_highlight(ui, 1);
}

static const tm_page_methods methods = {
    .in = transition, .in_para_init = entry, .show = show, .react = react,
    .indicator_ctrl = indicator, .scrollbar_ctrl = tm_func_do_nothing,
};

bool tm_text_input_popup_init(tm_text_input_popup* p, tm_ui* ui,
                              const char* title, char* buffer, char* draft,
                              size_t capacity, const char* charset, tm_page_cb cb) {
    size_t length, charset_len;
    if (!p || !ui || !buffer || !draft || !capacity || ui->fb.w < 96 || ui->fb.h < TEXT_H)
        return false;
    uintptr_t a = (uintptr_t)buffer, b = (uintptr_t)draft;
    if ((a <= b ? b - a : a - b) < capacity || !printable(buffer, capacity, &length))
        return false;
    if (!charset)
        charset = DEFAULT_CHARSET;
    if (!printable(charset, strlen(charset) + 1, &charset_len) || !charset_len)
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
    p->charset_len = charset_len;
    p->result = TM_TEXT_INPUT_EDITING;
    /* Make editing helpers usable immediately, as well as after entry. */
    memcpy(draft, buffer, length + 1);
    p->length = p->cursor = length;
    clamp_scroll(p);
    return true;
}
