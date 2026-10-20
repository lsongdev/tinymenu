/*
 * Host renderer: runs the demo UI with a scripted input session and writes
 * the screens as PBM images.
 *
 *   render <w>x<h> <outdir>
 *
 * The flush callback copies only the pages it is given into a simulated
 * display, and every snapshot checks that the display matches the
 * framebuffer, so a page that changed but was not flushed fails the run.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tinymenu.h"
#include "tm_demo.h"

static tm_ui ui;
static uint8_t *framebuffer, *display;
static const char *outdir;
static unsigned ticks;

static void flush(const tm_fb *fb, int16_t first_page, int16_t end_page, void *user)
{
    (void)user;
    memcpy(display + first_page * fb->w, fb->buf + first_page * fb->w, (size_t)(end_page - first_page) * fb->w);
}

/* Runs the UI for `frames` ticks of 10 ms, feeding the demo waves as it goes. */
static void run(int frames)
{
    for(int i = 0; i < frames; i++, ticks++)
    {
        if(ticks % 5 == 0)
            tm_demo_feed(ticks * 10);
        tm_ui_proc(&ui, 10);
    }
}

static void press(tm_msg msg)
{
    tm_ui_send(&ui, msg);
    run(150); // long enough for a full screen of rows to cascade in
}

/* Presses keys in turn: u d l r are the d-pad, n p next and previous (. and , on the
 * boards' serial), e clicks, b goes back. */
static void keys(const char *k)
{
    for(; *k; k++)
        press(*k == 'u' ? TM_MSG_UP : *k == 'd' ? TM_MSG_DOWN : *k == 'l' ? TM_MSG_LEFT
              : *k == 'r' ? TM_MSG_RIGHT : *k == 'n' ? TM_MSG_NEXT : *k == 'p' ? TM_MSG_PREV
              : *k == 'e' ? TM_MSG_CLICK : TM_MSG_RETURN);
}

/* Home items can be reordered without changing which demo the script opens. */
static void select_home(const char *name)
{
    if(ui.current_page != ui.home_page)
    {
        fprintf(stderr, "expected home before selecting %s\n", name);
        exit(1);
    }
    tm_menu_page *home = (tm_menu_page *)ui.home_page;
    for(uint8_t i = 0; i < home->menu.item_num; i++)
        if(strcmp(home->menu.option_array[i].text, name) == 0)
        {
            tm_menu_select(home, i);
            run(150);
            return;
        }
    fprintf(stderr, "home item not found: %s\n", name);
    exit(1);
}

/* Writes the last frame as a binary PBM (P4): rows of packed bits, MSB first. */
static void snapshot(const char *name)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.pbm", outdir, name);
    FILE *f = fopen(path, "wb");
    if(!f)
    {
        perror(path);
        exit(1);
    }
    const int w = ui.fb.w, h = ui.fb.h;
    if(memcmp(display, framebuffer, tm_fb_size(ui.fb.w, ui.fb.h)))
    {
        fprintf(stderr, "%s: the display does not match the framebuffer (a changed page was not flushed)\n", name);
        exit(1);
    }
    fprintf(f, "P4\n%d %d\n", w, h);
    for(int y = 0; y < h; y++)
    {
        for(int x0 = 0; x0 < w; x0 += 8)
        {
            unsigned char byte = 0;
            for(int x = x0; x < x0 + 8 && x < w; x++)
                if(display[(y / 8) * w + x] & (1 << (y & 7)))
                    byte |= (unsigned char)(0x80 >> (x - x0));
            fputc(byte, f);
        }
    }
    fclose(f);
}

int main(int argc, char **argv)
{
    int w, h;
    if(argc != 3 || sscanf(argv[1], "%dx%d", &w, &h) != 2)
    {
        fprintf(stderr, "usage: %s <w>x<h> <outdir>\n", argv[0]);
        return 2;
    }
    outdir = argv[2];
    framebuffer = malloc(tm_fb_size(w, h));
    display = calloc(1, tm_fb_size(w, h));
    tm_ui_init(&ui, framebuffer, w, h, flush, NULL);
    tm_demo_init(&ui);

    run(100);
    snapshot("01_home");
    // Display: a value popup, then a choice list that opens on the current value.
    keys("e");
    snapshot("02_display");
    keys("ne");
    snapshot("03_value_popup");
    keys("nne");
    snapshot("04_display_brightness");
    keys("ne");
    snapshot("05_list_popup");
    keys("ne");
    snapshot("06_display_sleep");
    // Sound: above 80 the volume popup asks first, in a popup on top of it.
    keys("bne");
    snapshot("07_sound");
    keys("ne");
    keys("nnnnn");
    snapshot("08_volume_confirm");
    keys("e");
    snapshot("09_volume_kept");
    keys("e");
    // A radio list, and the row that shows its choice.
    keys("nne");
    snapshot("10_ringtone");
    keys("nne");
    snapshot("11_ringtone_bell");
    keys("b");
    snapshot("12_sound_after");
    // A check box set through a confirm popup.
    keys("ne");
    snapshot("13_confirm");
    keys("le");
    // Network: a long value scrolls while selected; fixed point in a digit popup.
    keys("bne");
    snapshot("14_network");
    keys("nn");
    run(300);
    snapshot("15_network_ssid_scroll");
    keys("ne");
    snapshot("16_digit");
    keys("bne");
    snapshot("17_forget_confirm");
    keys("le");
    snapshot("18_network_forgotten");
    // Sensor: live waves.
    keys("bne");
    run(300);
    snapshot("19_sensor");
    // Settings: change the look of the whole UI.
    keys("b");
    select_home("Settings");
    keys("e");
    snapshot("20_settings");
    keys("nnnnnnnne");
    snapshot("21_dim_popup");
    keys("b");
    keys("nne");
    snapshot("22_classic");
    keys("pppppp"); // up to Highlight: Fill -> Frame
    keys("ene");
    snapshot("23_outline");
    keys("b");
    select_home("Display");
    keys("e");
    snapshot("24_display_outline");
    keys("pe");
    snapshot("25_value_classic");
    // About: facts, and a message popup that scrolls.
    keys("bb");
    select_home("About");
    keys("e");
    snapshot("26_about");
    keys("nnnne");
    snapshot("27_msg_popup");
    // Nested text input: the keyboard, by next/prev/click/back and by the d-pad.
    char name[48] = "tinymenu-lab-5G-upstairs", scratch[sizeof(name)];
    tm_text_input_popup text;
    if (!tm_text_input_popup_init(&text, &ui, "Device name", name, scratch,
                                  sizeof(name), NULL, NULL))
        return 1;
    ui.params = tm_default_params;
    tm_jump_to_page(ui.current_page, &text);
    run(100);
    snapshot("28_text_input");
    keys("nnnnn"); // next along the keys to 'f'
    snapshot("29_text_keys");
    keys("e");     // append 'f'
    keys("u");     // up from the first row wraps to the action under it: Space
    snapshot("30_text_actions");
    keys("re");    // Del removes the 'f'
    keys("lle");   // #+ turns to the symbols
    keys("de");    // down to the '#' over it, append it
    keys("b");     // unsaved changes: back asks again
    snapshot("31_text_discard");
    keys("ppp");   // disarm, then back past the first key to OK
    ui.params.style.animate = false;
    ui.params.style.highlight = TM_HIGHLIGHT_FRAME;
    tm_ui_invalidate(&ui); // a still UI redraws only when told
    run(100);
    snapshot("32_text_save");
    keys("e");
    if (text.result != TM_TEXT_INPUT_SAVED || strcmp(name, "tinymenu-lab-5G-upstairs#"))
        return 1;
    snapshot("33_text_closed");
    // About -> Check for update: a busy bar, then a download the demo drives from its loop.
    keys("bnne"); // close Credits, down past Keys to the update row
    snapshot("34_progress_busy");
    run(150);
    snapshot("35_progress");
    run(400);
    snapshot("36_progress_done");
    keys("eb"); // again, and cancel with back
    snapshot("37_progress_cancel");
    // A d-pad goes by the screen: left/right along the tiles, up raises a value, and
    // the digit popup picks digits with left/right and changes them with up/down.
    ui.params = tm_default_params;
    keys("b");
    select_home("Display");
    snapshot("38_tile_dpad");
    keys("u"); // up on a row of tiles goes right, as up raises a value
    snapshot("38_tile_up");
    keys("d");
    keys("ee");    // Display, which kept Brightness selected
    keys("lldu");  // left, left and down lower it, up raises it: 100 -> 80
    snapshot("39_value_dpad");
    keys("e");
    keys("brreue");   // Network, up from Forget network to Radio MHz
    keys("ulu");      // hundredths +1, then tenths +1
    snapshot("40_digit_dpad");
    keys("b");
    // Slide transition: back pushes the menu out to the right and home in from the
    // left; going deeper comes in from the right. Caught mid-way, then settled.
    ui.params.style.transition = TM_TRANSITION_SLIDE;
    tm_ui_send(&ui, TM_MSG_RETURN);
    run(6);
    snapshot("41_slide_back");
    run(80);
    tm_ui_send(&ui, TM_MSG_CLICK);
    run(6);
    snapshot("42_slide_in");
    run(80);
    snapshot("43_slide_done");
    // Cascade: the old page fades out, then the rows slide in one after another.
    ui.params.style.transition = TM_TRANSITION_CASCADE;
    keys("b");
    tm_ui_send(&ui, TM_MSG_CLICK);
    run(24);
    snapshot("44_cascade");
    run(80);
    // Home as a grid: the same home page laid out again (Settings -> Home as); down a row
    // scrolls, a click opens the item.
    ui.params = tm_default_params;
    tm_menu_page* home = (tm_menu_page*)ui.home_page;
    keys("b"); // home, on Network
    tm_menu_page_set_layout(home, TM_MENU_GRID);
    run(150);
    snapshot("45_grid");
    keys("d"); // a row down: Battery
    snapshot("46_grid_down");
    keys("e");
    snapshot("47_grid_open");
    keys("b");
    snapshot("48_grid_back");
    tm_ui_send(&ui, TM_MSG_DOWN); // Battery -> Mail: the grid scrolls a row
    run(8);
    snapshot("48_grid_scrolling");
    run(150);
    keys("d"); // the last row: About
    snapshot("48_grid_bottom");
    // ... and as an icon list: icons where the marks were, taller rows.
    tm_menu_page_set_layout(home, TM_MENU_ICON_LIST);
    run(150);
    snapshot("49_icon_list");
    keys("pp"); // up two: Lock
    snapshot("50_icon_list_up");
    // One highlight for every menu: corners on list rows too.
    ui.params.style.highlight = TM_HIGHLIGHT_CORNERS;
    keys("p");
    snapshot("51_corners");
    tm_menu_page_set_layout(home, TM_MENU_TILE);
    run(150);
    // Switch rows always use switches; check rows use checkboxes.
    static tm_option toggles[] = {
        {.text = "Toggles"},
        {.text = "Wi-Fi", .kind = TM_OPTION_SWITCH, .val = 1},
        {.text = "Bluetooth", .kind = TM_OPTION_SWITCH},
        {.text = "Checked", .kind = TM_OPTION_CHECK, .val = 1},
    };
    static tm_list_page toggle_list;
    tm_list_page_init(&toggle_list, &ui, 4, toggles, TM_LIST_NORMAL, NULL);
    ui.params = tm_default_params;
    tm_jump_to_page(ui.current_page, &toggle_list);
    run(150);
    snapshot("52_switches");
    keys("nne"); // Bluetooth on
    snapshot("53_switch_on");
    toggles[1].kind = toggles[2].kind = TM_OPTION_CHECK;
    keys("n");
    snapshot("54_checkboxes");
    // Stats: the week as bars (a day edited in the value popup), rows and a line; a spectrum.
    ui.params = tm_default_params;
    for (uint8_t i = 0; i < home->menu.item_num; i++)
        if (strcmp(home->menu.option_array[i].text, "Stats") == 0)
            home->menu.select_item = i;
    tm_menu_page_set_layout(home, TM_MENU_TILE); // lays home out again on Stats
    keys("be");
    keys("ne");
    snapshot("55_chart_bars");
    keys("reuue"); // Tue: 45 -> 47
    snapshot("56_chart_edited");
    keys("bne");
    snapshot("57_chart_rows");
    keys("bne");
    snapshot("58_chart_line");
    keys("bne");
    snapshot("59_spectrum");
    // Sensor: the light level on a fixed axis, filled; a digital input in steps.
    keys("bb");
    select_home("Sensor");
    keys("er");
    run(400);
    snapshot("60_wave_light");
    keys("rr");
    run(400);
    snapshot("61_wave_gpio");
    free(framebuffer);
    free(display);
    return 0;
}
