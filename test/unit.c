/*
 * Unit checks for helpers the snapshot test does not reach.
 *
 *   unit
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tinymenu.h"

static int failures;
#define CHECK(cond)                                                            \
    do {                                                                       \
        if(!(cond))                                                            \
        {                                                                      \
            fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #cond); \
            failures++;                                                        \
        }                                                                      \
    } while(0)

static void test_expand16(void)
{
    enum { W = 5, H = 10, ON = 0xF00D, OFF = 0x0001 };
    uint8_t buf[W * 2] = {0};
    tm_fb fb;
    tm_fb_init(&fb, buf, W, H);
    buf[2] = 1u << 3;      /* (2, 3) */
    buf[W + 4] = 1u << 1;  /* (4, 9) */

    uint16_t out[W * H];
    tm_fb_expand16(&fb, 0, H, 1, ON, OFF, out);
    for(int y = 0; y < H; y++)
        for(int x = 0; x < W; x++)
            CHECK(out[y * W + x] == (((x == 2 && y == 3) || (x == 4 && y == 9)) ? ON : OFF));

    /* rows 3..4 at scale 3: a 15 x 6 block */
    uint16_t big[W * 3 * 2 * 3];
    tm_fb_expand16(&fb, 3, 2, 3, ON, OFF, big);
    for(int y = 0; y < 6; y++)
        for(int x = 0; x < W * 3; x++)
            CHECK(big[y * W * 3 + x] == ((y / 3 == 0 && x / 3 == 2) ? ON : OFF));
}

/* Pixels of a canvas drawn with the foreground pen. */
static int lit(const tm_fb *fb, int x, int y) { return (fb->buf[(y / 8) * fb->w + x] >> (y & 7)) & 1; }

static void test_draw_line(void)
{
    enum { W = 16, H = 16 };
    uint8_t buf[W * 2];
    tm_fb fb;
    tm_fb_init(&fb, buf, W, H);
    tm_fb_set_color(&fb, 1);
    tm_canvas c = {&fb, 0, 0, W, H};
    const struct { int x1, y1, x2, y2; } lines[] = {
        {3, 2, 3, 12},  /* vertical, used to be skipped */
        {12, 9, 12, 1}, /* vertical, upwards */
        {1, 1, 14, 5},  /* shallow */
        {2, 14, 6, 2},  /* steep, |slope| = 3 */
        {0, 0, 9, 9},   /* diagonal */
    };
    for(size_t i = 0; i < sizeof(lines) / sizeof(lines[0]); i++)
    {
        memset(buf, 0, sizeof(buf));
        tm_canvas_draw_line(&c, lines[i].x1, lines[i].y1, lines[i].x2, lines[i].y2);
        CHECK(lit(&fb, lines[i].x1, lines[i].y1));
        CHECK(lit(&fb, lines[i].x2, lines[i].y2));
        /* one pixel per step along the longer axis, and no gaps */
        int dx = abs(lines[i].x2 - lines[i].x1), dy = abs(lines[i].y2 - lines[i].y1), n = 0;
        for(int y = 0; y < H; y++)
            for(int x = 0; x < W; x++)
                n += lit(&fb, x, y);
        CHECK(n == (dx > dy ? dx : dy) + 1);
    }
}

/* Wrapped text breaks between words; only a word wider than the canvas is split. */
static void test_wrap(void)
{
    const int line = Font_6_8.Height + TM_LINE_SPACING;
#define LINES(str) ((tm_text_height_wrapped(60, (str), Font_6_8) + TM_LINE_SPACING) / line) /* 10 columns */
    CHECK(LINES("") == 1);
    CHECK(LINES("hello") == 1);
    CHECK(LINES("hello world again") == 3);
    CHECK(LINES("abcdefghijklmno") == 2);
    CHECK(LINES("one two\nthree") == 2);
    CHECK(LINES("exactly10c next") == 2);
    CHECK(LINES("a b c d e f") == 2);
#undef LINES
}

/* Text editing is transactional and bounded, including capacity == 1, and
 * works from next/prev/click/return alone. */
static tm_text_input_result callback_result;
static bool text_cb(const tm_page* page, tm_msg msg) {
    callback_result = ((const tm_text_input_popup*)page)->result;
    return false;
}
static bool text_key(tm_text_input_popup* p, tm_msg msg) {
    tm_ui_send(p->page.ui, msg);
    return p->page.methods->react(p);
}
static void test_text_input(void) {
    uint8_t fb[128 * 64 / 8];
    tm_ui ui;
    tm_list_page home;
    tm_option options[] = {{.text = "Home"}};
    tm_ui_init(&ui, fb, 128, 64, NULL, NULL);
    tm_list_page_init(&home, &ui, 1, options, TM_LIST_NORMAL, NULL);
    char value[5] = "ab", draft[5];
    tm_text_input_popup p;
    CHECK(!tm_text_input_popup_init(&p, &ui, NULL, value, value, 5, NULL, NULL));
    CHECK(!tm_text_input_popup_init(&p, &ui, NULL, value, draft, 5, "", NULL));
    CHECK(!tm_text_input_popup_init(&p, &ui, NULL, value, draft, 5, "a\n", NULL));
    char unterminated[] = {'a', 'b'};
    CHECK(!tm_text_input_popup_init(&p, &ui, NULL, unterminated, draft, 2, NULL, NULL));
    CHECK(tm_text_input_popup_init(&p, &ui, "Name", value, draft, 5, "abc ", text_cb));
    p.page.methods->in_para_init(&p);
    CHECK(p.cursor == 2 && p.selected == 0); // cursor at the end, strip on 'a'

    // Two buttons: next/prev walk the ring, click acts, return leaves.
    text_key(&p, TM_MSG_NEXT); // 'b'
    CHECK(!text_key(&p, TM_MSG_CLICK));
    CHECK(strcmp(draft, "abb") == 0 && strcmp(value, "ab") == 0 && p.cursor == 3);
    text_key(&p, TM_MSG_PREV);
    text_key(&p, TM_MSG_PREV); // 'a' -> backspace, the ring wraps through the actions
    CHECK(p.selected == 4 + TM_TEXT_INPUT_BACKSPACE);
    text_key(&p, TM_MSG_CLICK);
    CHECK(strcmp(draft, "ab") == 0);
    // Return with unsaved changes asks again; anything else disarms it.
    text_key(&p, TM_MSG_CLICK); // "a"
    CHECK(!text_key(&p, TM_MSG_RETURN) && p.discard_armed);
    text_key(&p, TM_MSG_UP); // a d-pad turns the strip like a wheel: up is the next
    CHECK(p.selected == 0);
    CHECK(!p.discard_armed && !text_key(&p, TM_MSG_RETURN));
    CHECK(text_key(&p, TM_MSG_RETURN));
    CHECK(strcmp(value, "ab") == 0 && callback_result == TM_TEXT_INPUT_CANCELLED);
    // Unchanged: one return is enough.
    p.page.methods->in_para_init(&p);
    CHECK(strcmp(draft, "ab") == 0 && !p.discard_armed);
    CHECK(text_key(&p, TM_MSG_RETURN));

    // Edit the middle with the cursor actions, then save.
    p.page.methods->in_para_init(&p);
    tm_text_input_popup_select_action(&p, TM_TEXT_INPUT_LEFT);
    text_key(&p, TM_MSG_CLICK);
    CHECK(p.cursor == 1);
    tm_text_input_popup_select_char(&p, 'c');
    text_key(&p, TM_MSG_CLICK);
    CHECK(strcmp(draft, "acb") == 0 && p.cursor == 2);
    text_key(&p, TM_MSG_LEFT); // a d-pad moves the cursor directly
    text_key(&p, TM_MSG_LEFT);
    CHECK(p.cursor == 0 && !tm_text_input_popup_backspace(&p));
    text_key(&p, TM_MSG_LEFT);
    CHECK(p.cursor == 0);
    CHECK(tm_text_input_popup_insert(&p, ' '));
    CHECK(!tm_text_input_popup_insert(&p, 'a')); // full, including NUL
    CHECK(!tm_text_input_popup_insert(&p, 'x')); // outside charset
    CHECK(!tm_text_input_popup_select_char(&p, 'x'));
    CHECK(strcmp(draft, " acb") == 0);
    text_key(&p, TM_MSG_RIGHT);
    text_key(&p, TM_MSG_RIGHT);
    text_key(&p, TM_MSG_RIGHT);
    text_key(&p, TM_MSG_RIGHT);
    CHECK(p.cursor == 4);
    tm_text_input_popup_select_action(&p, TM_TEXT_INPUT_OK);
    CHECK(text_key(&p, TM_MSG_CLICK));
    CHECK(strcmp(value, " acb") == 0 && callback_result == TM_TEXT_INPUT_SAVED);

    // The next entry takes fresh application data.
    strcpy(value, "a");
    p.page.methods->in_para_init(&p);
    CHECK(strcmp(draft, "a") == 0 && p.cursor == 1);
    CHECK(tm_text_input_popup_backspace(&p) && strcmp(draft, "") == 0 && p.cursor == 0);

    // Long text scrolls with the cursor and never leaves blank cells behind.
    char name[48] = "tinymenu-lab-5G-upstairs", scratch[sizeof(name)];
    CHECK(tm_text_input_popup_init(&p, &ui, NULL, name, scratch, sizeof(name), NULL, NULL));
    p.page.methods->in_para_init(&p);
    CHECK(p.first_visible > 0 && p.cursor - p.first_visible <= 20);
    while (tm_text_input_popup_move_cursor(&p, false)) {}
    CHECK(p.first_visible == 0);
    while (tm_text_input_popup_move_cursor(&p, true)) {}
    size_t first = p.first_visible;
    while (tm_text_input_popup_backspace(&p))
        CHECK(p.first_visible <= first && p.length - p.first_visible <= 20);
    CHECK(p.first_visible == 0);

    char empty[1] = "", none[1];
    CHECK(tm_text_input_popup_init(&p, &ui, NULL, empty, none, 1, NULL, NULL));
    CHECK(!tm_text_input_popup_insert(&p, 'a'));
    CHECK(!tm_text_input_popup_backspace(&p));
    CHECK(none[0] == 0 && empty[0] == 0);
}

static void no_flush(const tm_fb* fb, int16_t first_page, int16_t end_page, void* user) {}

/* Progress: clamped values, busy mode, and a popup the application closes
 * from outside any callback, or the user cancels. */
static void test_progress(void) {
    tm_progress bar;
    tm_progress_init(&bar, 10, 0, 5); // swapped bounds
    CHECK(bar.min == 0 && bar.max == 10 && tm_progress_percent(&bar) == 50);
    tm_progress_set(&bar, 99);
    CHECK(bar.val == 10 && tm_progress_percent(&bar) == 100);
    tm_progress_set_busy(&bar, true);
    CHECK(bar.busy);
    tm_progress_set(&bar, -3);
    CHECK(!bar.busy && bar.val == 0);
    CHECK(tm_progress_fill_width(0, 100, 50, 104) == 50);
    CHECK(tm_progress_fill_width(0, 0, 0, 104) == 0); // no division by zero
    CHECK(tm_progress_fill_width(0, 100, 500, 104) == 100);

    uint8_t fb[128 * 64 / 8];
    tm_ui ui;
    tm_list_page home;
    tm_option options[] = {{.text = "Home"}};
    tm_ui_init(&ui, fb, 128, 64, no_flush, NULL);
    ui.params.style.animate = false;
    tm_list_page_init(&home, &ui, 1, options, TM_LIST_NORMAL, NULL);
    tm_progress_popup p;
    CHECK(tm_progress_popup_init(&p, &ui, "Saving", 0, false, NULL));
    CHECK(p.bar.busy);
    tm_ui_proc(&ui, 10);
    for (int i = 0; i < 500 && !ui.is_motionless; i++) tm_ui_proc(&ui, 10);
    CHECK(ui.is_motionless); // home is idle
    tm_jump_to_page(&home, &p); // from the application, not a callback
    for (int i = 0; i < 100; i++) tm_ui_proc(&ui, 10);
    CHECK(ui.current_page == (tm_page*)&p);
    CHECK(!ui.is_motionless); // busy keeps redrawing
    tm_ui_send(&ui, TM_MSG_RETURN); // not cancellable
    for (int i = 0; i < 10; i++) tm_ui_proc(&ui, 10);
    CHECK(ui.current_page == (tm_page*)&p && p.result == TM_PROGRESS_RUNNING);
    tm_progress_popup_set_max(&p, 4);
    tm_progress_popup_set(&p, 4);
    CHECK(!p.bar.busy && tm_progress_percent(&p.bar) == 100);
    tm_progress_popup_close(&p);
    for (int i = 0; i < 100; i++) tm_ui_proc(&ui, 10);
    CHECK(ui.current_page == (tm_page*)&home && p.result == TM_PROGRESS_DONE);

    CHECK(tm_progress_popup_init(&p, &ui, "Loading", 100, true, NULL));
    tm_jump_to_page(&home, &p);
    for (int i = 0; i < 100; i++) tm_ui_proc(&ui, 10);
    tm_ui_send(&ui, TM_MSG_RETURN);
    for (int i = 0; i < 100; i++) tm_ui_proc(&ui, 10);
    CHECK(ui.current_page == (tm_page*)&home && p.result == TM_PROGRESS_CANCELLED);
}

/* One axis reads in sequence; a d-pad goes by the screen. */
static void test_msg_axes(void) {
    CHECK(tm_msg_step(TM_MSG_NEXT) == 1 && tm_msg_step(TM_MSG_DOWN) == 1 && tm_msg_step(TM_MSG_RIGHT) == 1);
    CHECK(tm_msg_step(TM_MSG_PREV) == -1 && tm_msg_step(TM_MSG_UP) == -1 && tm_msg_step(TM_MSG_LEFT) == -1);
    CHECK(tm_msg_amount(TM_MSG_NEXT) == 1 && tm_msg_amount(TM_MSG_UP) == 1 && tm_msg_amount(TM_MSG_RIGHT) == 1);
    CHECK(tm_msg_amount(TM_MSG_PREV) == -1 && tm_msg_amount(TM_MSG_DOWN) == -1 && tm_msg_amount(TM_MSG_LEFT) == -1);
    CHECK(tm_msg_step(TM_MSG_CLICK) == 0 && tm_msg_amount(TM_MSG_RETURN) == 0);
    CHECK(tm_msg_is_direct_msg(TM_MSG_NEXT) && !tm_msg_is_direct_msg(TM_MSG_CLICK));

    // A confirm popup is fully usable with next alone: with nothing to scroll, it
    // switches the button.
    uint8_t fb[128 * 64 / 8];
    tm_ui ui;
    tm_list_page home;
    tm_option options[] = {{.text = "Home"}};
    tm_ui_init(&ui, fb, 128, 64, no_flush, NULL);
    tm_list_page_init(&home, &ui, 1, options, TM_LIST_NORMAL, NULL);
    tm_confirm_popup c;
    tm_confirm_popup_init(&c, &ui, "Sure?", "Yes", "No", false, false, false, 2, NULL);
    tm_jump_to_page(&home, &c);
    tm_ui_send(&ui, TM_MSG_NEXT);
    CHECK(c.page.methods->react(&c) == false && c.conf_ret);
    tm_ui_send(&ui, TM_MSG_PREV);
    c.page.methods->react(&c);
    CHECK(!c.conf_ret);

    // A value goes up with next, up and right.
    tm_value_popup v;
    tm_value_popup_init(&v, &ui, "V", 5, 0, 10, 1, false, false, NULL);
    const tm_msg ups[] = {TM_MSG_NEXT, TM_MSG_UP, TM_MSG_RIGHT, TM_MSG_PREV, TM_MSG_DOWN, TM_MSG_LEFT};
    const int32_t after[] = {6, 7, 8, 7, 6, 5};
    for (int i = 0; i < 6; i++) {
        tm_ui_send(&ui, ups[i]);
        v.page.methods->react(&v);
        CHECK(v.val == after[i]);
    }

    // Digits: next/prev keep the select-then-edit flow; a d-pad edits directly.
    tm_digit_popup d;
    tm_digit_popup_init(&d, &ui, "D", 0, TM_DECIMALS_0, 0, 999, false, false, NULL);
    tm_ui_send(&ui, TM_MSG_UP); // edits the ones digit straight away
    d.page.methods->react(&d);
    CHECK(d.val == 1);
    tm_ui_send(&ui, TM_MSG_LEFT); // to the tens
    d.page.methods->react(&d);
    tm_ui_send(&ui, TM_MSG_UP);
    d.page.methods->react(&d);
    CHECK(d.val == 11);
    tm_ui_send(&ui, TM_MSG_DOWN);
    d.page.methods->react(&d);
    CHECK(d.val == 1);
}

/* A slide draws both pages for a few frames, then lands on the new one; going
 * back and jumping home slide the other way. The default fade is untouched. */
static void test_slide(void) {
    uint8_t fb[128 * 64 / 8];
    tm_ui ui;
    tm_list_page home, sub;
    tm_option home_opts[] = {{.text = "Home"}, {.text = "Open"}};
    tm_option sub_opts[] = {{.text = "Sub"}, {.text = "Row"}};
    tm_ui_init(&ui, fb, 128, 64, no_flush, NULL);
    tm_list_page_init(&home, &ui, 2, home_opts, TM_LIST_NORMAL, NULL);
    tm_list_page_init(&sub, &ui, 2, sub_opts, TM_LIST_NORMAL, NULL);
    for (int i = 0; i < 200 && !ui.is_motionless; i++) tm_ui_proc(&ui, 10);
    ui.params.style.transition = TM_TRANSITION_SLIDE;

    tm_jump_to_page(&home, &sub);
    CHECK(ui.state == TM_STATE_SLIDE && ui.slide_dir == 1);
    tm_ui_proc(&ui, 10);
    tm_ui_proc(&ui, 10);
    CHECK(ui.state == TM_STATE_SLIDE && ui.slide.pos_cur > 0 && ui.slide.pos_cur < 128);
    CHECK(ui.fb.ox == 0); // the origin is reset after every frame
    for (int i = 0; i < 200 && ui.state != TM_STATE_PROC; i++) tm_ui_proc(&ui, 10);
    CHECK(ui.current_page == (tm_page*)&sub);

    tm_ui_send(&ui, TM_MSG_RETURN); // back slides the other way
    tm_ui_proc(&ui, 10);
    CHECK(ui.state == TM_STATE_SLIDE && ui.slide_dir == -1);
    for (int i = 0; i < 200 && ui.state != TM_STATE_PROC; i++) tm_ui_proc(&ui, 10);
    CHECK(ui.current_page == (tm_page*)&home);

    tm_jump_to_page(&home, &sub);
    for (int i = 0; i < 200 && ui.state != TM_STATE_PROC; i++) tm_ui_proc(&ui, 10);
    tm_jump_to_page(&sub, &home); // a jump to home counts as going back
    CHECK(ui.state == TM_STATE_SLIDE && ui.slide_dir == -1);
    for (int i = 0; i < 200 && ui.state != TM_STATE_PROC; i++) tm_ui_proc(&ui, 10);

    ui.params.style.animate = false; // no animation: no slide either
    tm_jump_to_page(&home, &sub);
    CHECK(ui.state != TM_STATE_SLIDE);
}

/* Grid: rows of three, moves by item and by row, scrolls to the selection. */
static void test_grid(void) {
    uint8_t fb[128 * 64 / 8];
    tm_ui ui;
    tm_list_page home;
    tm_option home_opts[] = {{.text = "Home"}};
    tm_ui_init(&ui, fb, 128, 64, no_flush, NULL);
    tm_list_page_init(&home, &ui, 1, home_opts, TM_LIST_NORMAL, NULL);
    static tm_icon icons[7];
    tm_option opts[7] = {{.text = "a"}, {.text = "b"}, {.text = "c"}, {.text = "d"},
                         {.text = "e"}, {.text = "f"}, {.text = "g"}};
    for (int i = 0; i < 7; i++) opts[i].icon = icons[i];
    tm_grid_page g;
    tm_grid_page_init(&g, &ui, 7, opts, 0, NULL);
    CHECK(tm_grid_page_cols(&g) == 3 && g.scroll.pos_tgt == 0);
    CHECK(g.icon_w == 30 && g.icon_h == 30);
    CHECK(!tm_grid_page_move(&g, -3));            // nothing above the first row
    CHECK(tm_grid_page_move(&g, 3) && g.select_item == 3);
    CHECK(tm_grid_page_move(&g, 1) && g.select_item == 4);
    CHECK(tm_grid_page_move(&g, 3) && g.select_item == 6); // short last row: its last item
    CHECK(g.scroll.pos_tgt > 0);                  // scrolled to show the third row
    CHECK(!tm_grid_page_move(&g, 3));             // already on the last row
    CHECK(tm_grid_page_move(&g, 1) && g.select_item == 0); // wraps (loop on by default)
    CHECK(g.scroll.pos_tgt == 0);
    ui.params.loop_param[TM_LOOP_TILE] = false;
    CHECK(!tm_grid_page_move(&g, -1));
    CHECK(tm_menu_selected(&g.page) == &opts[0]);
    // Names: all, the selected one only, or none; the room under the icons stays.
    tm_icon_menu* menu = (tm_icon_menu*)&g;
    CHECK(g.name_mode == TM_ICON_NAME_ALL && tm_icon_menu_shows_name(menu, 1));
    g.name_mode = TM_ICON_NAME_SELECTED;
    CHECK(tm_icon_menu_shows_name(menu, 0) && !tm_icon_menu_shows_name(menu, 1));
    g.name_mode = TM_ICON_NAME_NONE;
    CHECK(!tm_icon_menu_shows_name(menu, 0));
    CHECK(tm_icon_item_h(30, true) > tm_icon_item_h(30, false));

    // The same options as an icon list: taller rows, the selection read the same way.
    tm_list_page icon_list;
    tm_list_page_init(&icon_list, &ui, 7, opts, TM_LIST_NORMAL, NULL);
    CHECK(icon_list.icon_w == 0 && icon_list.line_h == 13);
    tm_list_page_set_icon_size(&icon_list, 30, 30);
    CHECK(icon_list.line_h == 34 && icon_list.line_n == 1); // the row follows the icon
    tm_list_page_set_icon_size(&icon_list, 16, 16);
    CHECK(icon_list.line_h == 20 && icon_list.line_n == 3);
    tm_list_page_set_icon_size(&icon_list, 8, 8);
    CHECK(icon_list.line_h == 13); // never shorter than a line of text
    CHECK(tm_menu_move(&icon_list, 2));
    CHECK(tm_menu_selected(&icon_list.page) == &opts[2]);
    CHECK(tm_menu_selected(&icon_list) == &opts[2]);
    tm_grid_page_init(&g, &ui, 7, opts, 5, NULL);
    CHECK(tm_grid_page_cols(&g) == 3); // only three fit in 128 px
    g.icon_w = g.icon_h = 16;
    CHECK(tm_grid_page_cols(&g) == 5); // smaller icons: the five asked for fit

    // Cascade: a list entered this way ends exactly as after the unfold.
    tm_list_page sub;
    tm_option sub_opts[] = {{.text = "Sub"}, {.text = "One"}, {.text = "Two"}};
    tm_list_page_init(&sub, &ui, 3, sub_opts, TM_LIST_NORMAL, NULL);
    for (int i = 0; i < 200 && !ui.is_motionless; i++) tm_ui_proc(&ui, 10);
    ui.params.style.transition = TM_TRANSITION_CASCADE;
    tm_jump_to_page(&home, &sub);
    for (int i = 0; i < 300 && !(ui.state == TM_STATE_PROC && ui.current_page == (tm_page*)&sub); i++)
        tm_ui_proc(&ui, 10);
    CHECK(ui.current_page == (tm_page*)&sub && !sub.vars.cascading);
    CHECK(sub.vars.optInt.pos_cur == sub.ind_y_tgt - 0); // first row at the top
}

/* The two presets the API offers. */
static void test_presets(void) {
    uint8_t fb[128 * 64 / 8];
    tm_ui ui;
    tm_ui_init(&ui, fb, 128, 64, no_flush, NULL);
    CHECK(ui.params.style.transition == TM_TRANSITION_CASCADE);
    CHECK(ui.params.style.highlight_width == TM_HIGHLIGHT_FULL_ROW);
    CHECK(ui.params.style.highlight == TM_HIGHLIGHT_FILL && ui.params.style.animate);
    tm_ui_set_preset(&ui, TM_PRESET_CLASSIC);
    CHECK(!ui.params.style.animate && ui.params.style.radius == 0);
    CHECK(ui.params.style.highlight_width == TM_HIGHLIGHT_FULL_ROW);
    CHECK(ui.params.winbgblur_param[TM_POPUP_BLUR_VALUE] == TM_BLUR_0);
    ui.params.ani_param[TM_ANI_LIST] = 7;
    tm_ui_set_preset(&ui, TM_PRESET_DEFAULT); // a preset is the whole set of parameters
    CHECK(memcmp(&ui.params, &tm_default_params, sizeof(ui.params)) == 0);
}

/* One menu page, any layout: switching keeps the items, the selection and the way back. */
static void test_menu_page(void) {
    uint8_t fb[128 * 64 / 8];
    tm_ui ui;
    tm_ui_init(&ui, fb, 128, 64, no_flush, NULL);
    static tm_icon icons[5];
    tm_option opts[5] = {{.text = "a"}, {.text = "b"}, {.text = "c"}, {.text = "d"}, {.text = "e"}};
    for (int i = 0; i < 5; i++) opts[i].icon = icons[i];
    tm_menu_page home;
    tm_list_page sub;
    tm_option sub_opts[] = {{.text = "Sub"}};
    tm_menu_page_init(&home, &ui, 5, opts, TM_MENU_TILE, NULL);
    tm_list_page_init(&sub, &ui, 1, sub_opts, TM_LIST_NORMAL, NULL);
    CHECK(ui.home_page == (tm_page_addr)&home && tm_menu_page_layout(&home) == TM_MENU_TILE);
    tm_menu_select(&home, 3);
    CHECK(home.menu.select_item == 3);
    sub.page.last_page = &home;
    tm_menu_page_set_layout(&home, TM_MENU_GRID);
    CHECK(tm_menu_page_layout(&home) == TM_MENU_GRID && home.menu.select_item == 3);
    CHECK(tm_menu_selected(&home.menu) == &opts[3] && ui.home_page == (tm_page_addr)&home);
    tm_menu_page_set_layout(&home, TM_MENU_ICON_LIST);
    CHECK(tm_menu_page_layout(&home) == TM_MENU_ICON_LIST && home.menu.icon_w == 30);
    CHECK(home.list.select_item == 3 && home.list.line_h == 34);
    tm_menu_page_set_layout(&home, TM_MENU_LIST);
    CHECK(tm_menu_page_layout(&home) == TM_MENU_LIST && home.menu.icon_w == 0);
    CHECK(tm_menu_selected(&home.menu.page) == &opts[3]);
    for (int i = 0; i < 300; i++) tm_ui_proc(&ui, 10); // shows the new layout without trouble
    CHECK(ui.current_page == (tm_page_addr)&home && ui.state == TM_STATE_PROC);
}

/* A list popup is a menu like the others: options, the selection read the same way. */
static void test_list_popup(void) {
    uint8_t fb[128 * 64 / 8];
    tm_ui ui;
    tm_list_page home;
    tm_option home_opts[] = {{.text = "Home"}};
    tm_ui_init(&ui, fb, 128, 64, no_flush, NULL);
    tm_list_page_init(&home, &ui, 1, home_opts, TM_LIST_NORMAL, NULL);
    tm_option choices[] = {{.text = "One"}, {.text = "Two", .mark = '*'}, {.text = NULL}};
    tm_list_popup popup;
    tm_list_popup_init(&popup, &ui, 3, choices, true, NULL);
    CHECK(choices[2].text != NULL && choices[1].order == 1);
    CHECK(tm_menu_move(&popup, 1) && !tm_menu_move(&popup, 0));
    CHECK(tm_menu_selected(&popup.page) == &choices[1]);
    CHECK(tm_menu_selected(&popup) == &choices[1]);
    tm_jump_to_page(&home, &popup);
    for (int i = 0; i < 100; i++) tm_ui_proc(&ui, 10);
    tm_ui_send(&ui, TM_MSG_CLICK); // writes the choice back to the row it opened on
    for (int i = 0; i < 100; i++) tm_ui_proc(&ui, 10);
    CHECK(home_opts[0].content == choices[1].text);
}

/* A chart is a menu of values: moves, reads like the others, redraws on demand. */
static void test_chart(void) {
    uint8_t fb[128 * 64 / 8];
    tm_ui ui;
    tm_list_page home;
    tm_option home_opts[] = {{.text = "Home"}};
    tm_ui_init(&ui, fb, 128, 64, no_flush, NULL);
    tm_list_page_init(&home, &ui, 1, home_opts, TM_LIST_NORMAL, NULL);
    tm_option days[] = {{.text = "Mon", .val = 3}, {.text = "Tue", .val = -2}, {.text = "Wed", .val = 7}};
    tm_chart_page chart;
    tm_chart_page_init(&chart, &ui, 3, days, TM_CHART_BAR, NULL);
    CHECK(days[2].order == 2 && chart.min == 0 && chart.max == 0);
    tm_jump_to_page(&home, &chart);
    for (int i = 0; i < 300; i++) tm_ui_proc(&ui, 10); // draws negative values too
    CHECK(ui.current_page == (tm_page_addr)&chart);
    tm_ui_send(&ui, TM_MSG_UP); // columns run sideways: up goes forward
    tm_ui_proc(&ui, 10);
    CHECK(chart.select_item == 1 && tm_menu_selected(&chart.page) == &days[1]);
    CHECK(tm_chart_page_move(&chart, 1) && tm_chart_page_move(&chart, 1) && chart.select_item == 0); // wraps
    chart.type = TM_CHART_HBAR;
    tm_ui_send(&ui, TM_MSG_DOWN); // rows run down, as in a list
    tm_ui_proc(&ui, 10);
    CHECK(chart.select_item == 1);
    for (int i = 0; i < 300 && !ui.is_motionless; i++) tm_ui_proc(&ui, 10);
    days[0].val = 9;
    tm_chart_page_refresh(&chart); // a new value wakes the UI to draw it
    CHECK(!ui.is_motionless);
}

/* Waves: switch between them, stop one and scroll back through what it kept. */
static void test_wave(void) {
    uint8_t fb[128 * 64 / 8];
    tm_ui ui;
    tm_ui_init(&ui, fb, 128, 64, no_flush, NULL);
    static int16_t a[300], b[300];
    tm_wave_data waves[] = {{.text = "A", .data = a, .depth = 300},
                            {.text = "B", .data = b, .depth = 300, .fixed_min = 0, .fixed_max = 1}};
    tm_wave_page wp;
    tm_wave_page_init(&wp, &ui, 2, waves, NULL);
    CHECK(waves[1].range_min == 0 && waves[1].range_max == 1); // a fixed axis from the start
    for (int i = 0; i < 200; i++) tm_wave_page_push(&wp, 0, (int16_t)i);
    CHECK(tm_wave_page_move(&wp, 1) && wp.selected == 1);
    CHECK(tm_wave_page_move(&wp, 1) && wp.selected == 0); // wraps
    CHECK(!tm_wave_page_can_scroll(&wp, 0, -1));            // only a stopped wave scrolls
    tm_wave_page_stop(&wp, 0, true);
    CHECK(waves[0].stopped && !tm_wave_page_can_scroll(&wp, 0, 1)); // already at the newest
    CHECK(tm_wave_page_can_scroll(&wp, 0, -1) && tm_wave_page_scroll(&wp, 0, -1));
    CHECK(tm_wave_page_scroll(&wp, 0, 1) && !tm_wave_page_scroll(&wp, 0, 1));
    tm_wave_page_stop(&wp, 0, false);
    CHECK(!waves[0].stopped);
}

/* A page's draw callback runs after the page draws itself: shown, under a popup, while
 * sliding, and after a menu page changes layout. */
static int drawn;
static const tm_page* drawn_page;
static void count_draw(const tm_page* page, tm_canvas* canvas) {
    drawn++;
    drawn_page = page;
    tm_canvas_draw_point(canvas, 0, 0);
}
static void test_draw_hook(void) {
    uint8_t fb[128 * 64 / 8];
    tm_ui ui;
    tm_ui_init(&ui, fb, 128, 64, no_flush, NULL);
    tm_option items[] = {{.text = "One"}, {.text = "Two"}};
    tm_menu_page home;
    tm_menu_page_init(&home, &ui, 2, items, TM_MENU_LIST, NULL);
    tm_page_set_draw(&home, count_draw);
    for (int i = 0; i < 100; i++) tm_ui_proc(&ui, 10);
    CHECK(drawn > 0 && drawn_page == &home.menu.page);
    tm_msg_popup msg;
    tm_msg_popup_init(&msg, &ui, "Hi", false, 2, NULL);
    tm_jump_to_page(&home, &msg);
    for (int i = 0; i < 100; i++) tm_ui_proc(&ui, 10);
    drawn = 0;
    ui.is_motionless = false;
    tm_ui_proc(&ui, 10);
    CHECK(drawn == 1); // still drawn as the page under the popup
    tm_ui_send(&ui, TM_MSG_RETURN);
    for (int i = 0; i < 100; i++) tm_ui_proc(&ui, 10);
    tm_menu_page_set_layout(&home, TM_MENU_GRID);
    CHECK(home.menu.page.draw == count_draw); // kept across layouts
    tm_list_page sub;
    tm_option sub_items[] = {{.text = "Sub"}};
    tm_list_page_init(&sub, &ui, 1, sub_items, TM_LIST_NORMAL, NULL);
    ui.params.style.transition = TM_TRANSITION_SLIDE;
    tm_jump_to_page(&home, &sub);
    drawn = 0;
    tm_ui_proc(&ui, 10);
    tm_ui_proc(&ui, 10);
    CHECK(ui.state == TM_STATE_SLIDE && drawn > 0); // the page sliding out draws it too
    tm_page_set_draw(&home, NULL);
}

int main(void)
{
    test_expand16();
    test_draw_line();
    test_wrap();
    test_text_input();
    test_progress();
    test_msg_axes();
    test_slide();
    test_grid();
    test_presets();
    test_menu_page();
    test_list_popup();
    test_chart();
    test_wave();
    test_draw_hook();
    if(failures)
        return 1;
    puts("unit checks pass");
    return 0;
}
