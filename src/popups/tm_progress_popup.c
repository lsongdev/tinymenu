#include <stdio.h>

#include "popups/tm_progress_popup.h"
#include "core/tm_ui.h"

/* The value popup's look: same font, margins and bar. */
#define FONT DEFAULT_VAL_WIN_FONT
#define MARGIN DEFAULT_VAL_WIN_FONT_MARGIN
#define BAR_H DEFAULT_VAL_WIN_BAR_H
#define WIN_H (2 * MARGIN + 2 * GET_FNOT_H(FONT) + DEFAULT_VAL_TEXT_BAR_S)
/* The bar sits where the value popup's min/max row is. */
#define BAR_Y (MARGIN + GET_FNOT_H(FONT) + DEFAULT_VAL_TEXT_BAR_S + (GET_FNOT_H(FONT) - BAR_H) / 2)

static int16_t win_w(const tm_ui* ui) {
    return MIN(DEFAULT_VAL_WIN_W, ui->fb.w - 8);
}

/* Redraw now: the application usually changes the popup outside a callback. */
static void wake(tm_progress_popup* p) {
    p->page.ui->is_motionless = false;
}

static void draw(tm_progress_popup* p) {
    tm_ui* ui = p->page.ui;
    int16_t w = win_w(ui), x = (ui->fb.w - w) / 2, y = p->win_y.pos_cur;
    tm_fb_set_color(&ui->fb, TM_PEN_BLACK);
    tm_canvas_draw_rbox(&ui->w_all, x, y, w, WIN_H, tm_ui_radius(ui, DEFAULT_VAL_WIN_R));
    tm_fb_set_color(&ui->fb, TM_PEN_WHITE);
    tm_canvas_draw_rbox_empty(&ui->w_all, x, y, w, WIN_H, tm_ui_radius(ui, DEFAULT_VAL_WIN_R));
    tm_canvas c = {.fb = &ui->fb, .start_x = x + MARGIN, .start_y = y + MARGIN,
                   .w = w - 2 * MARGIN, .h = GET_FNOT_H(FONT)};
    if (!p->bar.busy) {
        char percent[5];
        snprintf(percent, sizeof(percent), "%u%%", (unsigned)tm_progress_percent(&p->bar));
        int16_t pw = (int16_t)tm_text_width(percent, FONT);
        tm_canvas_draw_str(&c, c.w - pw, 0, FONT, percent);
        c.w -= pw + DEFAULT_VAL_WIN_TXTVAL_S;
    }
    tm_canvas_draw_str(&c, 0, 0, FONT, p->text);
    tm_progress_draw(&p->bar, ui, x + MARGIN, y + BAR_Y, w - 2 * MARGIN, BAR_H);
}

static void entry(tm_page_addr addr) {
    tm_progress_popup* p = addr;
    tm_ui* ui = p->page.ui;
    p->result = TM_PROGRESS_RUNNING;
    p->close_requested = false;
    p->bar.fill.pos_cur = 0;
    p->win_y.pos_cur = -WIN_H;
    p->win_y.pos_tgt = (ui->fb.h - WIN_H) / 2;
    ui->win_bg_blur = ui->params.winbgblur_param[TM_POPUP_BLUR_PROGRESS];
}

static bool transition(tm_page_addr addr) {
    tm_progress_popup* p = addr;
    tm_ui* ui = p->page.ui;
    tm_animation(&p->win_y, tm_ui_anim_ms(ui, TM_ANI_POPUP), ui->time, &ui->anim_is_finish);
    draw(p);
    return p->win_y.pos_cur == p->win_y.pos_tgt;
}

static void show(tm_page_addr addr) {
    tm_progress_popup* p = addr;
    tm_ui* ui = p->page.ui;
    tm_page* bg = p->page.last_page;
    if (bg != &p->page)
        tm_page_show(bg);
    tm_fb_set_color(&ui->fb, TM_PEN_BLACK);
    tm_fb_blur(&ui->fb, ui->win_bg_blur);
    tm_fb_set_color(&ui->fb, TM_PEN_WHITE);
    draw(p);
}

static void leave(tm_progress_popup* p, tm_progress_result result) {
    p->result = result;
    p->close_requested = false;
    p->win_y.pos_tgt = -WIN_H;
    tm_page_return(p);
}

static bool react(tm_page_addr addr) {
    tm_progress_popup* p = addr;
    tm_msg msg = tm_msg_que_read(&p->page.ui->msg_queue);
    if (p->close_requested) {
        leave(p, TM_PROGRESS_DONE);
        return true;
    }
    bool done = false;
    if (p->page.auto_input) {
        if (msg == TM_MSG_RETURN && p->cancellable) {
            leave(p, TM_PROGRESS_CANCELLED);
            done = true;
        }
        if (msg != TM_MSG_NONE && p->page.cb)
            p->page.cb(&p->page, msg);
    } else if (msg != TM_MSG_NONE && p->page.cb) {
        done = p->page.cb(&p->page, msg);
        if (done && p->result == TM_PROGRESS_RUNNING)
            leave(p, TM_PROGRESS_CANCELLED);
    }
    return done;
}

static const tm_page_methods methods = {
    .in = transition, .in_para_init = entry, .show = show, .react = react,
    .indicator_ctrl = tm_func_do_nothing, .scrollbar_ctrl = tm_func_do_nothing,
};

bool tm_progress_popup_init(tm_progress_popup* p, tm_ui* ui, const char* text,
                            int32_t max, bool cancellable, tm_page_cb cb) {
    if (!p || !ui || ui->fb.h < WIN_H || ui->fb.w < 48)
        return false;
    memset(p, 0, sizeof(*p));
    p->page.page_type = TM_PAGE_PROGRESS_POPUP;
    tm_page_init(p, ui, cb);
    p->page.methods = &methods;
    p->text = text ? text : "";
    p->cancellable = cancellable;
    tm_progress_init(&p->bar, 0, max > 0 ? max : 0, 0);
    tm_progress_set_busy(&p->bar, max <= 0);
    return true;
}

void tm_progress_popup_set(tm_progress_popup* p, int32_t val) {
    tm_progress_set(&p->bar, val);
    wake(p);
}

void tm_progress_popup_set_max(tm_progress_popup* p, int32_t max) {
    p->bar.max = max > 0 ? max : 0;
    tm_progress_set(&p->bar, p->bar.val);
    wake(p);
}

void tm_progress_popup_set_busy(tm_progress_popup* p) {
    tm_progress_set_busy(&p->bar, true);
    wake(p);
}

void tm_progress_popup_set_text(tm_progress_popup* p, const char* text) {
    p->text = text ? text : "";
    wake(p);
}

void tm_progress_popup_close(tm_progress_popup* p) {
    p->close_requested = true;
    wake(p);
}
