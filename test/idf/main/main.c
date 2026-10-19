#include "tinymenu.h"

static tm_ui ui;
static tm_list_page page;
static uint8_t framebuffer[128 * 64 / 8];
static tm_option options[] = {
    {.text = "Hello"},
    {.text = "Switch", .kind = TM_OPTION_SWITCH},
};

static void flush(const tm_fb *fb, int16_t first_page, int16_t end_page, void *user)
{
    (void)fb, (void)first_page, (void)end_page, (void)user;
}

void app_main(void)
{
    tm_ui_init(&ui, framebuffer, 128, 64, flush, NULL);
    tm_list_page_init(&page, &ui, 2, options, TM_LIST_NORMAL, NULL);
    tm_ui_proc(&ui, 10);
}
