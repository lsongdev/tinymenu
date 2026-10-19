// The tinymenu demo: a settings menu for an imaginary handheld that uses every
// page and popup once, so it shows what the library can do. The boards in
// examples/ and the host test (test/render.c) all run this same menu.
#include "tm_demo.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define COUNT(a) ((uint8_t)(sizeof(a) / sizeof((a)[0])))

static tm_ui* demo_ui;

// Pages
static tm_menu_page home_page; // tiles, a grid or an icon list (Settings -> Home as)
static tm_list_page display_page, sound_page, ringtone_page, network_page, settings_page, about_page;
static tm_wave_page sensor_page;
static tm_list_page stats_page;
static tm_chart_page week_chart, spectrum_chart;
// Popups. Value, digit, confirm and message popups are shared: each reads the
// row it was opened on and writes the result back to it.
static tm_value_popup value_popup, volume_popup;
static tm_digit_popup digit_popup;
static tm_text_input_popup text_input_popup;
static char ssid[33] = "tinymenu-lab-5G-upstairs", ssid_draft[sizeof(ssid)];
static tm_confirm_popup confirm_popup, loud_confirm, forget_confirm;
static tm_msg_popup msg_popup;
static tm_list_popup sleep_popup, dim_popup, transition_popup, highlight_popup,
    home_as_popup;

//-------- Home
enum { HOME_DISPLAY, HOME_SOUND, HOME_NETWORK, HOME_SENSOR,
       // Stats opens charts; the other middle entries give the grid rows to scroll through.
       HOME_CLOCK, HOME_BATTERY, HOME_STATS, HOME_MUSIC, HOME_MAIL, HOME_LOCK,
       HOME_SETTINGS, HOME_ABOUT };
static tm_icon home_icons[] = { // in HOME_* order
    {
        0xFC, 0xFE, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x83, 0x81, 0x01, 0x01, 0x01, 0x01, 0x01, 0xE1,
        0xE1, 0x01, 0x01, 0x01, 0x01, 0x01, 0x81, 0x83, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFE, 0xFC,
        0xFF, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xC0, 0xC1, 0x03, 0x03, 0x00, 0xE0, 0xF0, 0xF8, 0xF8,
        0xF8, 0xF8, 0xF0, 0xE0, 0x00, 0x03, 0x03, 0xC1, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0x00, 0xFF,
        0xFF, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x60, 0x70, 0x30, 0x00, 0x01, 0x03, 0x07, 0xC7,
        0xC7, 0x07, 0x03, 0x01, 0x00, 0x30, 0x70, 0x60, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xFF,
        0x0F, 0x1F, 0x3F, 0x3F, 0x3E, 0x3C, 0x38, 0x30, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x21,
        0x21, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x30, 0x38, 0x3C, 0x3E, 0x3F, 0x3F, 0x1F, 0x0F,
    }, // Display
    {
        0xFC, 0xFE, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x81, 0xC1, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFE, 0xFC,
        0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF,
        0xFF, 0xFF, 0x00, 0x00, 0x00, 0x0C, 0xF8, 0xE1, 0x03, 0x0E, 0xF8, 0x00, 0x00, 0x00, 0xFF,
        0xFF, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x07, 0x0F, 0x1F, 0x3F,
        0x7F, 0xFF, 0x00, 0x00, 0x00, 0x0C, 0x07, 0x21, 0x30, 0x1C, 0x07, 0x00, 0x80, 0xC0, 0xFF,
        0x0F, 0x1F, 0x3F, 0x3F, 0x3E, 0x3C, 0x38, 0x30, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x30, 0x38, 0x3C, 0x3E, 0x3F, 0x3F, 0x1F, 0x0F,
    }, // Sound
    {
        0xFC, 0xFE, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x81, 0x81,
        0x81, 0x81, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFE, 0xFC,
        0xFF, 0x00, 0x00, 0x00, 0x00, 0x08, 0x0C, 0x06, 0x46, 0x63, 0x33, 0x31, 0x11, 0x99, 0x99,
        0x99, 0x99, 0x11, 0x31, 0x33, 0x63, 0x46, 0x06, 0x0C, 0x08, 0x00, 0x00, 0x00, 0x00, 0xFF,
        0xFF, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x71, 0xF9,
        0xF9, 0x71, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xFF,
        0x0F, 0x1F, 0x3F, 0x3F, 0x3E, 0x3C, 0x38, 0x30, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x30, 0x38, 0x3C, 0x3E, 0x3F, 0x3F, 0x1F, 0x0F,
    }, // Network
    {
        0xFC, 0xFE, 0x7F, 0x3F, 0x1F, 0x0F, 0xC7, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x87, 0x8F, 0x1F, 0x3F, 0x7F, 0xFE, 0xFC,
        0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xC0, 0xE0, 0xE0, 0xC0, 0x80,
        0xE0, 0xFC, 0x3E, 0x0E, 0x1C, 0x38, 0x3C, 0x1F, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0xFF,
        0xFF, 0xC0, 0x80, 0x00, 0x00, 0x00, 0xFF, 0x80, 0x9C, 0x9F, 0x8F, 0x83, 0x80, 0x81, 0x83,
        0x83, 0x81, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xFF,
        0x0F, 0x1F, 0x3F, 0x3F, 0x3E, 0x3C, 0x38, 0x30, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x30, 0x38, 0x3C, 0x3E, 0x3F, 0x3F, 0x1F, 0x0F,
    }, // Sensor
    {
        0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF, 0xFF, 0xFF, 0x7F, 0x7F, 0x3F, 0x3F, 0x9F, 0x9F, 0x9F,
        0x9F, 0x9F, 0x9F, 0x3F, 0x3F, 0x7F, 0x7F, 0xFF, 0xFF, 0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x03, 0xF0, 0xFC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
        0x00, 0x7F, 0x7F, 0xFF, 0xFF, 0xFE, 0xFC, 0xF0, 0x03, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xF0, 0xC3, 0x8F, 0x9F, 0x3F, 0x3F, 0x7F, 0x7F, 0x7E,
        0x7E, 0x7E, 0x7C, 0x3C, 0x38, 0x99, 0x8F, 0xC3, 0xF0, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3E, 0x3E, 0x3E,
        0x3E, 0x3E, 0x3E, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01,
    }, // Clock
    {
        0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0x01, 0x01, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0,
        0xE0, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE0, 0xE0, 0xFC, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
        0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01,
    }, // Battery
    {
        0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x07, 0x07, 0x07,
        0xFF, 0x3F, 0x3F, 0x3F, 0x3F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0xFF, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E,
        0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x1E, 0x0F, 0x07, 0x03, 0x01,
    }, // Stats
    {
        0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x7F,
        0x7F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x1F, 0x1F, 0x1F, 0x1F, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
        0xF8, 0xF8, 0xFC, 0xFC, 0x7C, 0x7C, 0x7C, 0xFE, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC7, 0x83, 0x01, 0x01, 0x01, 0x83, 0xC0, 0xC0,
        0xFF, 0xFF, 0xF1, 0xE0, 0xC0, 0xC0, 0xC0, 0xE0, 0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
        0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01,
    }, // Music
    {
        0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xF8, 0xF0, 0xE4, 0xEC, 0xDC, 0x9C, 0x3C, 0x7C,
        0x7C, 0x3C, 0x9C, 0xDC, 0xEC, 0xE4, 0xF0, 0xF8, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0xC0, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCE,
        0xCE, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xC0, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
        0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01,
    }, // Mail
    {
        0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F, 0x1F, 0x03, 0x21, 0x39, 0x3C, 0x3C,
        0x3C, 0x3C, 0x39, 0x21, 0x03, 0x1F, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x1E,
        0x1E, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
        0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01,
    }, // Lock
    {
        0xFC, 0xFE, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x81, 0xC1, 0xC1, 0xC1, 0x81, 0x01, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFE, 0xFC,
        0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x41, 0xE1, 0xF1, 0xF1, 0xF1, 0xE1, 0x41, 0x41,
        0x43, 0x47, 0x47, 0x47, 0x43, 0x41, 0x41, 0x41, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
        0xFF, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x10, 0x10, 0x10,
        0x10, 0x10, 0x38, 0x7C, 0x7C, 0x7C, 0x38, 0x10, 0x10, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xFF,
        0x0F, 0x1F, 0x3F, 0x3F, 0x3E, 0x3C, 0x38, 0x30, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x30, 0x38, 0x3C, 0x3E, 0x3F, 0x3F, 0x1F, 0x0F,
    }, // Settings
    {
        0xFC, 0xFE, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x01, 0x01, 0x01, 0x81, 0x81, 0x81,
        0x81, 0x81, 0x81, 0x01, 0x01, 0x01, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFE, 0xFC,
        0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x0C, 0x06, 0x03, 0x01, 0x00, 0x00, 0xE6,
        0xE6, 0x00, 0x00, 0x01, 0x03, 0x06, 0x0C, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
        0xFF, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x03, 0x0C, 0x18, 0x30, 0x20, 0x40, 0x40, 0x4F,
        0x4F, 0x40, 0x40, 0x20, 0x30, 0x18, 0x0C, 0x03, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xFF,
        0x0F, 0x1F, 0x3F, 0x3F, 0x3E, 0x3C, 0x38, 0x30, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x30, 0x38, 0x3C, 0x3E, 0x3F, 0x3F, 0x1F, 0x0F,
    }, // About
};
#define EXAMPLE "An example item: there is nothing behind it."
static tm_option home_options[] = {
    [HOME_DISPLAY] = {.text = "Display", .icon = home_icons[HOME_DISPLAY]},
    [HOME_SOUND] = {.text = "Sound", .icon = home_icons[HOME_SOUND]},
    [HOME_NETWORK] = {.text = "Network", .icon = home_icons[HOME_NETWORK]},
    [HOME_SENSOR] = {.text = "Sensor", .icon = home_icons[HOME_SENSOR]},
    [HOME_CLOCK] = {.text = "Clock", .icon = home_icons[HOME_CLOCK], .content = EXAMPLE},
    [HOME_BATTERY] = {.text = "Battery", .icon = home_icons[HOME_BATTERY], .content = EXAMPLE},
    [HOME_STATS] = {.text = "Stats", .icon = home_icons[HOME_STATS]},
    [HOME_MUSIC] = {.text = "Music", .icon = home_icons[HOME_MUSIC], .content = EXAMPLE},
    [HOME_MAIL] = {.text = "Mail", .icon = home_icons[HOME_MAIL], .content = EXAMPLE},
    [HOME_LOCK] = {.text = "Lock", .icon = home_icons[HOME_LOCK], .content = EXAMPLE},
    [HOME_SETTINGS] = {.text = "Settings", .icon = home_icons[HOME_SETTINGS]},
    [HOME_ABOUT] = {.text = "About", .icon = home_icons[HOME_ABOUT]},
};



//-------- Display: a value popup, a choice list popup, a switch
enum { DISPLAY_TITLE, DISPLAY_BRIGHTNESS, DISPLAY_SLEEP, DISPLAY_ALWAYS_ON };
static tm_option display_options[] = {
    [DISPLAY_TITLE] = {.text = "[ Display ]"},
    [DISPLAY_BRIGHTNESS] = {.text = "Brightness", .kind = TM_OPTION_INT, .val = 80},
    [DISPLAY_SLEEP] = {.text = "Sleep after", .kind = TM_OPTION_TEXT, .content = "30 s"},
    [DISPLAY_ALWAYS_ON] = {.text = "Always on", .kind = TM_OPTION_SWITCH},
};
static tm_option SLEEP_CHOICES[] = {{.text = "Never"}, {.text = "15 s"}, {.text = "30 s"}, {.text = "1 min"}, {.text = "5 min"}};

//-------- Sound: a popup that opens another popup, a radio list, a confirmed check box
enum { SOUND_TITLE, SOUND_VOLUME, SOUND_KEY_CLICK, SOUND_RINGTONE, SOUND_DND };
static tm_option sound_options[] = {
    [SOUND_TITLE] = {.text = "[ Sound ]"},
    [SOUND_VOLUME] = {.text = "Volume", .kind = TM_OPTION_INT, .val = 60},
    [SOUND_KEY_CLICK] = {.text = "Key click", .kind = TM_OPTION_SWITCH, .val = 1},
    [SOUND_RINGTONE] = {.text = "Ringtone", .kind = TM_OPTION_TEXT, .content = "Chime"},
    [SOUND_DND] = {.text = "Do not disturb", .kind = TM_OPTION_CHECK,
                   .content = "Silence calls and alerts until you turn it off?"},
};
#define LOUD_VOLUME 80
static bool loud_ok; // the listener agreed to go above LOUD_VOLUME

static tm_option ringtone_options[] = {
    {.text = "[ Ringtone ]"},
    {.text = "Chime", .kind = TM_OPTION_CHECK, .val = 1},
    {.text = "Bell", .kind = TM_OPTION_CHECK},
    {.text = "Beep", .kind = TM_OPTION_CHECK},
    {.text = "Silent", .kind = TM_OPTION_CHECK},
};

//-------- Network: a long value that scrolls, a fixed-point digit popup, a confirm popup
enum { NET_TITLE, NET_WIFI, NET_SSID, NET_FREQUENCY, NET_FORGET };
static tm_option network_options[] = {
    [NET_TITLE] = {.text = "[ Network ]"},
    [NET_WIFI] = {.text = "Wi-Fi", .kind = TM_OPTION_SWITCH, .val = 1},
    [NET_SSID] = {.text = "SSID", .kind = TM_OPTION_TEXT, .content = ssid},
    [NET_FREQUENCY] = {.text = "Radio MHz", .kind = TM_OPTION_FIXED, .val = 43392,
                       .decimals = TM_DECIMALS_2},
    [NET_FORGET] = {.text = "Forget network"},
};
static char forget_question[64];

//-------- Sensor: four simulated waves
#define WAVE_DEPTH 320 // samples kept per wave; a stopped wave can scroll back through them
static int16_t temperature_samples[WAVE_DEPTH], light_samples[WAVE_DEPTH],
    vibration_samples[WAVE_DEPTH], gpio_samples[WAVE_DEPTH];
// One wave of each kind, with samples generated by tm_demo_feed.
enum { WAVE_TEMPERATURE, WAVE_LIGHT, WAVE_VIBRATION, WAVE_GPIO };
static tm_wave_data sensor_waves[] = {
    [WAVE_TEMPERATURE] = {.text = "Temperature", .type = TM_WAVE_SOLID,
                          .data = temperature_samples, .depth = WAVE_DEPTH, .decimals = TM_DECIMALS_1},
    // A percentage: filled, on a fixed 0..100 axis.
    [WAVE_LIGHT] = {.text = "Light %", .type = TM_WAVE_FILL, .data = light_samples,
                    .depth = WAVE_DEPTH, .fixed_min = 0, .fixed_max = 100},
    [WAVE_VIBRATION] = {.text = "Vibration", .type = TM_WAVE_DASH, .data = vibration_samples,
                        .depth = WAVE_DEPTH, .decimals = TM_DECIMALS_2},
    // A simulated digital input: steps, on a fixed 0..1 axis.
    [WAVE_GPIO] = {.text = "GPIO", .type = TM_WAVE_STEP, .data = gpio_samples,
                   .depth = WAVE_DEPTH, .fixed_min = 0, .fixed_max = 1},
};

//-------- Stats: a week of numbers as columns, rows or a line, and a live spectrum
enum { STATS_TITLE, STATS_BARS, STATS_ROWS, STATS_LINE, STATS_SPECTRUM };
static tm_option stats_options[] = {
    [STATS_TITLE] = {.text = "[ Stats ]"},
    [STATS_BARS] = {.text = "Week as bars"},
    [STATS_ROWS] = {.text = "Week as rows"},
    [STATS_LINE] = {.text = "Week as a line"},
    [STATS_SPECTRUM] = {.text = "Spectrum"},
};
// One chart, three ways; a click on a day edits it in the value popup.
static tm_option week[] = {
    {.text = "Mon", .val = 32}, {.text = "Tue", .val = 45}, {.text = "Wed", .val = 12},
    {.text = "Thu", .val = 58}, {.text = "Fri", .val = 71}, {.text = "Sat", .val = 40},
    {.text = "Sun", .val = 25},
};
// Bands from 50 Hz to 16 kHz, named by frequency; the levels move with tm_demo_feed.
#define SPECTRUM_BANDS 24
static tm_option spectrum[SPECTRUM_BANDS];
static char spectrum_names[SPECTRUM_BANDS][8];

//-------- Settings: the look of the whole UI, changed live
enum {
    SETTINGS_TITLE, SETTINGS_ANIMATE, SETTINGS_HOME_AS, SETTINGS_TRANSITION, SETTINGS_HIGHLIGHT,
    SETTINGS_ROUNDED,
    SETTINGS_FULL_ROW, SETTINGS_LOOP,
    SETTINGS_DIM, SETTINGS_MOTION, SETTINGS_CLASSIC, SETTINGS_DEFAULT,
};
static tm_option settings_options[] = {
    [SETTINGS_TITLE] = {.text = "[ Settings ]"},
    [SETTINGS_ANIMATE] = {.text = "Animate", .kind = TM_OPTION_SWITCH},
    [SETTINGS_HOME_AS] = {.text = "Home as", .kind = TM_OPTION_TEXT},
    [SETTINGS_TRANSITION] = {.text = "Pages", .kind = TM_OPTION_TEXT},
    [SETTINGS_HIGHLIGHT] = {.text = "Highlight", .kind = TM_OPTION_TEXT},
    [SETTINGS_ROUNDED] = {.text = "Rounded", .kind = TM_OPTION_SWITCH},
    [SETTINGS_FULL_ROW] = {.text = "Full row", .kind = TM_OPTION_SWITCH},
    [SETTINGS_LOOP] = {.text = "Wrap around", .kind = TM_OPTION_SWITCH},
    [SETTINGS_DIM] = {.text = "Popup dim", .kind = TM_OPTION_TEXT},
    [SETTINGS_MOTION] = {.text = "Motion ms", .kind = TM_OPTION_INT},
    [SETTINGS_CLASSIC] = {.text = "Classic look", .mark = '*'},
    [SETTINGS_DEFAULT] = {.text = "Default look", .mark = '*'},
};
static tm_option DIM_CHOICES[] = {{.text = "None"}, {.text = "Light"}, {.text = "Half"}, {.text = "Dark"}, {.text = "Black"}};
// In tm_transition order.
static tm_option TRANSITION_CHOICES[] = {{.text = "Fade"}, {.text = "Slide"}, {.text = "Cascade"}};
// In tm_highlight order.
static tm_option HIGHLIGHT_CHOICES[] = {{.text = "Fill"}, {.text = "Frame"}, {.text = "Corners"}};
static tm_option HOME_AS_CHOICES[] = {{.text = "Tile"}, {.text = "Grid"}, {.text = "List"}};
static const tm_menu_layout HOME_LAYOUTS[] = {TM_MENU_TILE, TM_MENU_GRID, TM_MENU_ICON_LIST};

//-------- About: plain facts, message popups and a simulated update with a progress popup
enum { ABOUT_TITLE, ABOUT_VERSION, ABOUT_SCREEN, ABOUT_LICENSE, ABOUT_CREDITS, ABOUT_KEYS,
       ABOUT_UPDATE };
static char screen_size[12];
static tm_option about_options[] = {
    [ABOUT_TITLE] = {.text = "[ About ]"},
    [ABOUT_VERSION] = {.text = "Version", .kind = TM_OPTION_TEXT, .content = "0.1.0"},
    [ABOUT_SCREEN] = {.text = "Screen", .kind = TM_OPTION_TEXT, .content = screen_size},
    [ABOUT_LICENSE] = {.text = "License", .kind = TM_OPTION_TEXT, .content = "AGPL-3.0-only"},
    [ABOUT_CREDITS] = {.text = "Credits", .content = "tinymenu is a refactor of WouoUI-PageVersion "
                                                     "by Sheep118, based on WouoUI by RQNG."},
    [ABOUT_KEYS] = {.text = "Keys", .content = "Up/down move, click opens or confirms, back "
                                               "returns. In a message, up/down scroll it."},
    [ABOUT_UPDATE] = {.text = "Check for update"},
};
// The update: "Checking" while busy, then a download with known length; back cancels.
#define UPDATE_CHECK_MS 1500
#define UPDATE_DOWNLOAD_MS 4000
static tm_progress_popup update_popup;
static bool update_starting, update_running;
static uint32_t update_start_ms;

//-------- Helpers
static tm_option* selected(const tm_page* page) {
    return tm_menu_selected(page);
}

static void return_home(const tm_page* page) {
    tm_jump_to_page((tm_page_addr)page, &home_page);
    // The home page is the root; returning from it should not reopen this menu.
    home_page.menu.page.last_page = NULL;
}

// Opens a choice list with the row's current value selected.
static void open_choices(const tm_page* from, tm_list_popup* popup) {
    const char* current = selected(from)->content;
    for (uint8_t i = 0; i < popup->item_num; i++)
        if (current && strcmp(popup->option_array[i].text, current) == 0)
            tm_list_popup_select(popup, i);
    tm_jump_to_page((tm_page_addr)from, popup);
}

static void open_value(const tm_page* from, int32_t min, int32_t step, int32_t max) {
    tm_value_popup_set_min_step_max(&value_popup, min, step, max);
    tm_jump_to_page((tm_page_addr)from, &value_popup);
}

//-------- Callbacks
static bool home_cb(const tm_page* page, tm_msg msg) {
    static tm_page_addr const TARGETS[] = {
        [HOME_DISPLAY] = &display_page, [HOME_SOUND] = &sound_page, [HOME_NETWORK] = &network_page,
        [HOME_SENSOR] = &sensor_page,   [HOME_SETTINGS] = &settings_page, [HOME_ABOUT] = &about_page,
        [HOME_STATS] = &stats_page,
    };
    if (msg != TM_MSG_CLICK)
        return false;
    const uint8_t to = selected(page)->order;
    // The examples open the message popup, which shows their content.
    tm_jump_to_page((tm_page_addr)page,
                    to < COUNT(TARGETS) && TARGETS[to] ? TARGETS[to] : (tm_page_addr)&msg_popup);
    return false;
}

static bool display_cb(const tm_page* page, tm_msg msg) {
    if (msg != TM_MSG_CLICK)
        return false;
    switch (selected(page)->order) {
    case DISPLAY_TITLE: return_home(page); break;
    case DISPLAY_BRIGHTNESS: open_value(page, 0, 5, 100); break;
    case DISPLAY_SLEEP: open_choices(page, &sleep_popup); break;
    default: break;
    }
    return false;
}

static bool sound_cb(const tm_page* page, tm_msg msg) {
    if (msg != TM_MSG_CLICK)
        return false;
    switch (selected(page)->order) {
    case SOUND_TITLE: return_home(page); break;
    case SOUND_VOLUME: tm_jump_to_page((tm_page_addr)page, &volume_popup); break;
    case SOUND_RINGTONE: tm_jump_to_page((tm_page_addr)page, &ringtone_page); break;
    case SOUND_DND: tm_jump_to_page((tm_page_addr)page, &confirm_popup); break;
    default: break;
    }
    return false;
}

// Going above LOUD_VOLUME asks first, in a popup on top of the volume popup.
static bool volume_cb(const tm_page* page, tm_msg msg) {
    if (tm_msg_amount(msg) > 0 && volume_popup.val > LOUD_VOLUME && !loud_ok)
        tm_jump_to_page((tm_page_addr)page, &loud_confirm);
    return false;
}

static bool loud_confirm_cb(const tm_page* page, tm_msg msg) {
    if (msg == TM_MSG_CLICK && loud_confirm.conf_ret)
        loud_ok = true;
    else if (msg == TM_MSG_CLICK || msg == TM_MSG_RETURN)
        volume_popup.val = LOUD_VOLUME;
    return false;
}

static bool ringtone_cb(const tm_page* page, tm_msg msg) {
    if (msg == TM_MSG_CLICK && selected(page)->order == 0) {
        return_home(page);
        return false;
    }
    if (msg == TM_MSG_CLICK && selected(page)->kind == TM_OPTION_CHECK)
        sound_options[SOUND_RINGTONE].content = selected(page)->text;
    return false;
}

static bool network_cb(const tm_page* page, tm_msg msg) {
    if (msg != TM_MSG_CLICK)
        return false;
    switch (selected(page)->order) {
    case NET_TITLE: return_home(page); break;
    case NET_SSID:
        tm_jump_to_page((tm_page_addr)page, &text_input_popup);
        break;
    case NET_FREQUENCY:
        tm_digit_popup_set_min_max_decimals(&digit_popup, 40000, 47000, TM_DECIMALS_2);
        tm_jump_to_page((tm_page_addr)page, &digit_popup);
        break;
    case NET_FORGET:
        snprintf(forget_question, sizeof(forget_question), "Forget %s?",
                 network_options[NET_SSID].content);
        forget_confirm.content = forget_question;
        forget_confirm.conf_ret = false; // default to the safe answer
        tm_jump_to_page((tm_page_addr)page, &forget_confirm);
        break;
    default: break;
    }
    return false;
}

static bool forget_cb(const tm_page* page, tm_msg msg) {
    if (msg == TM_MSG_CLICK && forget_confirm.conf_ret) {
        strcpy(ssid, "none");
        network_options[NET_WIFI].val = 0;
    }
    return false;
}

// A stopped wave scrolls through its history; say so when it runs out.
static bool sensor_cb(const tm_page* page, tm_msg msg) {
    const uint8_t wave = sensor_page.selected;
    if (!sensor_page.waves[wave].stopped)
        return false;
    const bool back = tm_msg_amount(msg) < 0; // a horizontal page: up goes forward
    const bool forward = tm_msg_amount(msg) > 0;
    if ((back || forward) && !tm_wave_page_can_scroll(&sensor_page, wave, forward ? 1 : -1)) {
        tm_msg_popup_set_content(&msg_popup, back ? "This is the oldest sample kept."
                                                  : "This is the newest sample.");
        tm_jump_to_page((tm_page_addr)page, &msg_popup);
    }
    return false;
}

// Shows the UI's settings in the Settings rows.
static void settings_rows_from_ui(void) {
    const tm_ui_params* p = &demo_ui->params;
    settings_options[SETTINGS_ANIMATE].val = p->style.animate;
    settings_options[SETTINGS_TRANSITION].content = TRANSITION_CHOICES[p->style.transition].text;
    settings_options[SETTINGS_HIGHLIGHT].content = HIGHLIGHT_CHOICES[p->style.highlight].text;
    for (uint8_t i = 0; i < COUNT(HOME_LAYOUTS); i++)
        if (HOME_LAYOUTS[i] == tm_menu_page_layout(&home_page))
            settings_options[SETTINGS_HOME_AS].content = HOME_AS_CHOICES[i].text;
    settings_options[SETTINGS_ROUNDED].val = p->style.radius != 0;
    settings_options[SETTINGS_FULL_ROW].val = p->style.highlight_width == TM_HIGHLIGHT_FULL_ROW;
    settings_options[SETTINGS_LOOP].val = p->loop_param[TM_LOOP_LIST];
    settings_options[SETTINGS_DIM].content = DIM_CHOICES[p->winbgblur_param[TM_POPUP_BLUR_VALUE]].text;
    settings_options[SETTINGS_MOTION].val = p->ani_param[TM_ANI_INDICATOR];
}

static void set_motion_ms(uint16_t ms) {
    tm_ui_params* p = &demo_ui->params;
    p->ani_param[TM_ANI_INDICATOR] = ms;
    p->ani_param[TM_ANI_TILE] = p->ani_param[TM_ANI_LIST] = p->ani_param[TM_ANI_WAVE] =
        p->ani_param[TM_ANI_POPUP] = (uint16_t)(ms * 2 / 3);
    p->ani_param[TM_ANI_SLIDE] = (uint16_t)(ms * 2 / 3);
}

static void set_popup_dim(tm_blur blur) {
    for (int i = 0; i < TM_POPUP_BLUR_COUNT; i++)
        demo_ui->params.winbgblur_param[i] = blur;
}

static bool settings_cb(const tm_page* page, tm_msg msg) {
    if (msg != TM_MSG_CLICK)
        return false;
    tm_ui_params* p = &demo_ui->params;
    switch (selected(page)->order) {
    case SETTINGS_TITLE: return_home(page); return false;
    case SETTINGS_DIM: open_choices(page, &dim_popup); return false;
    case SETTINGS_TRANSITION: open_choices(page, &transition_popup); return false;
    case SETTINGS_HIGHLIGHT: open_choices(page, &highlight_popup); return false;
    case SETTINGS_HOME_AS: open_choices(page, &home_as_popup); return false;
    case SETTINGS_MOTION: open_value(page, 0, 25, 500); return false;
    case SETTINGS_CLASSIC: // the two presets: a plain menu, and the animated default
        tm_ui_set_preset(demo_ui, TM_PRESET_CLASSIC);
        break;
    case SETTINGS_DEFAULT:
        tm_ui_set_preset(demo_ui, TM_PRESET_DEFAULT);
        break;
    default: // a switch; the list has already flipped its value
        p->style.animate = settings_options[SETTINGS_ANIMATE].val;
        p->style.radius = settings_options[SETTINGS_ROUNDED].val ? tm_default_params.style.radius : 0;
        p->style.highlight_width =
            settings_options[SETTINGS_FULL_ROW].val ? TM_HIGHLIGHT_FULL_ROW : TM_HIGHLIGHT_FIT_TEXT;
        p->loop_param[TM_LOOP_LIST] = p->loop_param[TM_LOOP_LIST_POPUP] = settings_options[SETTINGS_LOOP].val;
        break;
    }
    settings_rows_from_ui();
    return false;
}

static bool transition_cb(const tm_page* page, tm_msg msg) {
    if (msg == TM_MSG_CLICK) {
        demo_ui->params.style.transition = (tm_transition)transition_popup.select_item;
        settings_rows_from_ui();
    }
    return false;
}

static bool highlight_cb(const tm_page* page, tm_msg msg) {
    if (msg == TM_MSG_CLICK) {
        demo_ui->params.style.highlight = (tm_highlight)highlight_popup.select_item;
        settings_rows_from_ui();
    }
    return false;
}

static bool home_as_cb(const tm_page* page, tm_msg msg) {
    if (msg == TM_MSG_CLICK)
        tm_menu_page_set_layout(&home_page, HOME_LAYOUTS[home_as_popup.select_item]);
    return false;
}

static bool dim_cb(const tm_page* page, tm_msg msg) {
    if (msg == TM_MSG_CLICK)
        set_popup_dim((tm_blur)dim_popup.select_item);
    return false;
}

static bool stats_cb(const tm_page* page, tm_msg msg) {
    static const tm_chart_type TYPES[] = {
        [STATS_BARS] = TM_CHART_BAR, [STATS_ROWS] = TM_CHART_HBAR, [STATS_LINE] = TM_CHART_LINE};
    if (msg != TM_MSG_CLICK)
        return false;
    const uint8_t row = selected(page)->order;
    if (row == STATS_TITLE) {
        return_home(page);
    } else if (row == STATS_SPECTRUM) {
        tm_jump_to_page((tm_page_addr)page, &spectrum_chart);
    } else {
        week_chart.type = TYPES[row];
        tm_jump_to_page((tm_page_addr)page, &week_chart);
    }
    return false;
}

static bool week_cb(const tm_page* page, tm_msg msg) {
    if (msg == TM_MSG_CLICK) // the value popup reads the selected day and writes it back
        open_value(page, 0, 1, 100);
    return false;
}

static bool value_cb(const tm_page* page, tm_msg msg) {
    if (msg == TM_MSG_CLICK && value_popup.bg_opt == &settings_options[SETTINGS_MOTION])
        set_motion_ms((uint16_t)value_popup.val);
    return false;
}

static bool about_cb(const tm_page* page, tm_msg msg) {
    if (msg == TM_MSG_CLICK && selected(page)->order == ABOUT_TITLE) {
        return_home(page);
        return false;
    }
    if (msg == TM_MSG_CLICK && selected(page)->order == ABOUT_UPDATE) {
        tm_progress_popup_set_text(&update_popup, "Checking");
        tm_progress_popup_set_busy(&update_popup);
        update_starting = true; // the clock starts on the next feed
        tm_jump_to_page((tm_page_addr)page, &update_popup);
    } else if (msg == TM_MSG_CLICK && selected(page)->kind == TM_OPTION_PLAIN && selected(page)->content)
        tm_jump_to_page((tm_page_addr)page, &msg_popup);
    return false;
}

//-------- Entry points
void tm_demo_init(tm_ui* ui) {
    demo_ui = ui;
    snprintf(screen_size, sizeof(screen_size), "%dx%d", ui->fb.w, ui->fb.h);
    settings_rows_from_ui();

    tm_menu_page_init(&home_page, ui, COUNT(home_options), home_options, TM_MENU_TILE, home_cb);
    tm_list_page_init(&display_page, ui, COUNT(display_options), display_options, TM_LIST_NORMAL,
                      display_cb);
    tm_list_page_init(&sound_page, ui, COUNT(sound_options), sound_options, TM_LIST_NORMAL, sound_cb);
    tm_list_page_init(&ringtone_page, ui, COUNT(ringtone_options), ringtone_options, TM_LIST_RADIO,
                      ringtone_cb);
    tm_list_page_init(&network_page, ui, COUNT(network_options), network_options, TM_LIST_NORMAL,
                      network_cb);
    tm_wave_page_init(&sensor_page, ui, COUNT(sensor_waves), sensor_waves, sensor_cb);
    tm_list_page_init(&stats_page, ui, COUNT(stats_options), stats_options, TM_LIST_NORMAL, stats_cb);
    tm_chart_page_init(&week_chart, ui, COUNT(week), week, TM_CHART_BAR, week_cb);
    week_chart.min = 0; // a fixed axis, so editing a day shows how it compares
    week_chart.max = 100;
    for (int i = 0; i < SPECTRUM_BANDS; i++) {
        const float hz = 50.0f * powf(16000.0f / 50.0f, (float)i / (SPECTRUM_BANDS - 1));
        if (hz < 1000.0f)
            snprintf(spectrum_names[i], sizeof(spectrum_names[i]), "%d Hz", (int)hz);
        else
            snprintf(spectrum_names[i], sizeof(spectrum_names[i]), "%.1fk", hz / 1000.0f);
        spectrum[i].text = spectrum_names[i];
    }
    tm_chart_page_init(&spectrum_chart, ui, SPECTRUM_BANDS, spectrum, TM_CHART_BAR, NULL);
    spectrum_chart.max = 100;
    tm_list_page_init(&settings_page, ui, COUNT(settings_options), settings_options, TM_LIST_NORMAL, settings_cb);
    tm_list_page_init(&about_page, ui, COUNT(about_options), about_options, TM_LIST_NORMAL, about_cb);

    // Shared popups read the row they open on and write the result back to it.
    tm_value_popup_init(&value_popup, ui, NULL, 0, 0, 100, 1, true, true, value_cb);
    tm_text_input_popup_init(&text_input_popup, ui, "SSID", ssid, ssid_draft,
                              sizeof(ssid), NULL, NULL);
    tm_digit_popup_init(&digit_popup, ui, NULL, 0, TM_DECIMALS_0, 0, 0, true, true, NULL);
    tm_confirm_popup_init(&confirm_popup, ui, NULL, NULL, NULL, false, true, true, 2, NULL);
    tm_msg_popup_init(&msg_popup, ui, NULL, true, 2, NULL);
    tm_list_popup_init(&sleep_popup, ui, COUNT(SLEEP_CHOICES), SLEEP_CHOICES, true, NULL);
    tm_list_popup_init(&dim_popup, ui, COUNT(DIM_CHOICES), DIM_CHOICES, true, dim_cb);
    tm_list_popup_init(&transition_popup, ui, COUNT(TRANSITION_CHOICES), TRANSITION_CHOICES, true,
                       transition_cb);
    tm_list_popup_init(&highlight_popup, ui, COUNT(HIGHLIGHT_CHOICES), HIGHLIGHT_CHOICES, true,
                       highlight_cb);
    tm_list_popup_init(&home_as_popup, ui, COUNT(HOME_AS_CHOICES), HOME_AS_CHOICES, true, home_as_cb);
    // Popups with their own behaviour.
    tm_value_popup_init(&volume_popup, ui, NULL, 0, 0, 100, 5, true, true, volume_cb);
    tm_confirm_popup_init(&loud_confirm, ui, "High volume can harm your hearing. Go above 80?",
                          "Yes", "No", false, false, false, 2, loud_confirm_cb);
    tm_progress_popup_init(&update_popup, ui, NULL, 100, true, NULL);
    tm_confirm_popup_init(&forget_confirm, ui, NULL, "Forget", "Keep", false, false, false, 2,
                          forget_cb);
}

// Drives the update popup the way real work would: from the loop, not from a callback.
static void feed_update(uint32_t now_ms) {
    if (update_starting) {
        update_starting = false;
        update_running = true;
        update_start_ms = now_ms;
    }
    if (!update_running)
        return;
    if (update_popup.result == TM_PROGRESS_CANCELLED) {
        update_running = false;
        return;
    }
    uint32_t t = now_ms - update_start_ms;
    if (t < UPDATE_CHECK_MS)
        return;
    t -= UPDATE_CHECK_MS;
    if (update_popup.bar.busy)
        tm_progress_popup_set_text(&update_popup, "Downloading");
    tm_progress_popup_set(&update_popup, (int32_t)(t < UPDATE_DOWNLOAD_MS ? t * 100 / UPDATE_DOWNLOAD_MS : 100));
    if (t >= UPDATE_DOWNLOAD_MS + 300) { // let 100% show briefly
        tm_progress_popup_close(&update_popup);
        update_running = false;
    }
}

void tm_demo_feed(uint32_t now_ms) {
    feed_update(now_ms);
    const float t = now_ms / 1000.0f;
    // Simulated readings: 23.5 C drifting by a degree or so, light fading slowly,
    // and an input switching in a fixed pattern, one bit every 200 ms.
    const int16_t temperature = (int16_t)(235 + 12 * sinf(t * 0.7f) + 4 * sinf(t * 3.1f));
    tm_wave_page_push(&sensor_page, WAVE_TEMPERATURE, temperature);
    const int16_t light = (int16_t)(55 + 30 * sinf(t * 0.4f) + 8 * sinf(t * 2.9f));
    tm_wave_page_push(&sensor_page, WAVE_LIGHT, light);
    // A decaying vibration that rings again every 4 s.
    const float ring = fmodf(t, 4.0f);
    tm_wave_page_push(&sensor_page, WAVE_VIBRATION, (int16_t)(300 * expf(-ring * 1.5f) * sinf(t * 18.0f)));
    static const char LEVELS[] = "1101000111001011";
    const int16_t gpio = LEVELS[(now_ms / 200) % (sizeof(LEVELS) - 1)] - '0';
    tm_wave_page_push(&sensor_page, WAVE_GPIO, gpio);
    // A spectrum: a falling envelope with beats moving across it.
    for (int i = 0; i < SPECTRUM_BANDS; i++) {
        const float level = 70.0f * expf(-i / 14.0f) * (0.55f + 0.45f * sinf(t * 5.0f + i * 0.8f)) +
                            18.0f * fabsf(sinf(t * 2.3f + i * 1.7f));
        spectrum[i].val = (int32_t)(level > 100 ? 100 : level);
    }
    if (tm_ui_current_page(demo_ui) == &spectrum_chart.page) // only redraw while it is showing
        tm_chart_page_refresh(&spectrum_chart);
}
