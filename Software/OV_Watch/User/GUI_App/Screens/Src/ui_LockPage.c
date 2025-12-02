#include "../../ui.h"
#include "../../ui_helpers.h"
#include "../Inc/ui_LockPage.h"

#include "../../../Func/Inc/AppData.h"
#include "../../../Tasks/Inc/user_TasksInit.h"

#include <stdint.h>
#include <string.h>

Page_t Page_PasswordSet = {ui_PasswordSetPage_screen_init, ui_PasswordSetPage_screen_deinit, &ui_PasswordSetPage};
Page_t Page_LockScreen = {ui_LockScreen_screen_init, ui_LockScreen_screen_deinit, &ui_LockPage};

lv_obj_t * ui_PasswordSetPage;
lv_obj_t * ui_LockPage;

static lv_obj_t * ui_PasswordStatusLabel;
static lv_obj_t * ui_PasswordPinLabel;
static lv_obj_t * ui_PasswordNoticeLabel;

static lv_obj_t * ui_LockPinLabel;
static lv_obj_t * ui_LockNoticeLabel;

static uint8_t g_password_pin_buffer[4];
static uint8_t g_password_pin_length;
static uint8_t g_unlock_pin_buffer[4];
static uint8_t g_unlock_pin_length;

static void ui_pin_format(const uint8_t *digits, uint8_t length, uint8_t reveal, char *out, uint8_t out_size)
{
    uint8_t i = 0;
    uint8_t pos = 0;

    if (out_size == 0u)
    {
        return;
    }

    for (i = 0; i < 4u && (uint8_t)(pos + 2u) < out_size; ++i)
    {
        if (i < length)
        {
            out[pos++] = reveal ? (char)('0' + digits[i]) : '*';
        }
        else
        {
            out[pos++] = '_';
        }

        if (i < 3u)
        {
            out[pos++] = ' ';
        }
    }

    out[pos] = '\0';
}

static void ui_password_refresh(void)
{
    char pin_buf[16];

    if (ui_PasswordSetPage == 0)
    {
        return;
    }

    ui_pin_format(g_password_pin_buffer, g_password_pin_length, 1u, pin_buf, sizeof(pin_buf));
    lv_label_set_text(ui_PasswordPinLabel, pin_buf);
    lv_label_set_text(ui_PasswordStatusLabel, g_app_data.password_enabled ? "LOCK ON" : "LOCK OFF");

    if (g_password_pin_length == 4u)
    {
        lv_label_set_text(ui_PasswordNoticeLabel, "SAVE to enable this PIN");
    }
    else
    {
        lv_label_set_text(ui_PasswordNoticeLabel, "Enter a 4-digit PIN");
    }
}

static void ui_unlock_refresh(const char *notice)
{
    char pin_buf[16];

    if (ui_LockPage == 0)
    {
        return;
    }

    ui_pin_format(g_unlock_pin_buffer, g_unlock_pin_length, 0u, pin_buf, sizeof(pin_buf));
    lv_label_set_text(ui_LockPinLabel, pin_buf);
    lv_label_set_text(ui_LockNoticeLabel, notice);
}

static void ui_password_push_digit(uint8_t digit)
{
    if (g_password_pin_length >= 4u)
    {
        g_password_pin_length = 0u;
        memset(g_password_pin_buffer, 0, sizeof(g_password_pin_buffer));
    }

    g_password_pin_buffer[g_password_pin_length++] = digit;
    ui_password_refresh();
}

static void ui_unlock_push_digit(uint8_t digit)
{
    if (g_unlock_pin_length >= 4u)
    {
        return;
    }

    g_unlock_pin_buffer[g_unlock_pin_length++] = digit;
    ui_unlock_refresh("Enter PIN");
}

static void ui_password_clear(void)
{
    g_password_pin_length = 0u;
    memset(g_password_pin_buffer, 0, sizeof(g_password_pin_buffer));
    ui_password_refresh();
}

static void ui_unlock_clear(const char *notice)
{
    g_unlock_pin_length = 0u;
    memset(g_unlock_pin_buffer, 0, sizeof(g_unlock_pin_buffer));
    ui_unlock_refresh(notice);
}

static void ui_password_save(void)
{
    if (g_password_pin_length != 4u)
    {
        lv_label_set_text(ui_PasswordNoticeLabel, "PIN needs 4 digits");
        return;
    }

    memcpy(g_app_data.pin, g_password_pin_buffer, sizeof(g_app_data.pin));
    g_app_data.password_enabled = 1u;
    User_RequestDataSave();
    Page_Back();
}

static void ui_password_disable(void)
{
    g_app_data.password_enabled = 0u;
    User_RequestDataSave();
    ui_password_refresh();
}

static void ui_unlock_try(void)
{
    if (g_unlock_pin_length != 4u)
    {
        ui_unlock_refresh("Need 4 digits");
        return;
    }

    if (memcmp(g_unlock_pin_buffer, g_app_data.pin, sizeof(g_app_data.pin)) == 0)
    {
        ui_unlock_clear("Unlocked");
        Page_Back();
    }
    else
    {
        ui_unlock_clear("Try again");
    }
}

static lv_obj_t * ui_create_key_btn(lv_obj_t *parent,
                                    lv_coord_t x,
                                    lv_coord_t y,
                                    const char *text,
                                    lv_event_cb_t event_cb,
                                    uintptr_t user_data)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_t *label = 0;

    lv_obj_set_size(btn, 58, 38);
    lv_obj_set_pos(btn, x, y);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x202020), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    label = lv_label_create(btn);
    lv_obj_center(label);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_ALL, (void *)user_data);
    return btn;
}

static void ui_event_password_keypad(lv_event_t * e)
{
    uintptr_t key = 0;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    {
        return;
    }

    key = (uintptr_t)lv_event_get_user_data(e);
    if (key <= 9u)
    {
        ui_password_push_digit((uint8_t)key);
    }
    else if (key == 10u)
    {
        ui_password_clear();
    }
    else
    {
        ui_password_save();
    }
}

static void ui_event_unlock_keypad(lv_event_t * e)
{
    uintptr_t key = 0;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    {
        return;
    }

    key = (uintptr_t)lv_event_get_user_data(e);
    if (key <= 9u)
    {
        ui_unlock_push_digit((uint8_t)key);
    }
    else if (key == 10u)
    {
        ui_unlock_clear("Enter PIN");
    }
    else
    {
        ui_unlock_try();
    }
}

static void ui_event_password_disable(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        ui_password_disable();
    }
}

void ui_PasswordSetPage_screen_init(void)
{
    lv_obj_t * title = 0;
    lv_obj_t * disable_btn = 0;
    lv_obj_t * disable_label = 0;

    memcpy(g_password_pin_buffer, g_app_data.pin, sizeof(g_password_pin_buffer));
    g_password_pin_length = 4u;

    ui_PasswordSetPage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_PasswordSetPage, LV_OBJ_FLAG_SCROLLABLE);

    title = lv_label_create(ui_PasswordSetPage);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 16, 14);
    lv_label_set_text(title, "PASSWORD");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_PasswordStatusLabel = lv_label_create(ui_PasswordSetPage);
    lv_obj_align(ui_PasswordStatusLabel, LV_ALIGN_TOP_RIGHT, -16, 18);
    lv_obj_set_style_text_color(ui_PasswordStatusLabel, lv_color_hex(0x1980E1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_PasswordStatusLabel, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_PasswordPinLabel = lv_label_create(ui_PasswordSetPage);
    lv_obj_align(ui_PasswordPinLabel, LV_ALIGN_TOP_MID, 0, 56);
    lv_obj_set_style_text_font(ui_PasswordPinLabel, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_PasswordNoticeLabel = lv_label_create(ui_PasswordSetPage);
    lv_obj_align(ui_PasswordNoticeLabel, LV_ALIGN_TOP_MID, 0, 98);
    lv_obj_set_style_text_color(ui_PasswordNoticeLabel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_PasswordNoticeLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    disable_btn = lv_btn_create(ui_PasswordSetPage);
    lv_obj_set_size(disable_btn, 72, 30);
    lv_obj_align(disable_btn, LV_ALIGN_TOP_RIGHT, -16, 52);
    lv_obj_set_style_bg_color(disable_btn, lv_color_hex(0xB04040), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(disable_btn, LV_OBJ_FLAG_SCROLLABLE);

    disable_label = lv_label_create(disable_btn);
    lv_obj_center(disable_label);
    lv_label_set_text(disable_label, "OFF");
    lv_obj_set_style_text_font(disable_label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(disable_btn, ui_event_password_disable, LV_EVENT_ALL, NULL);

    ui_create_key_btn(ui_PasswordSetPage, 24, 128, "1", ui_event_password_keypad, (uintptr_t)1u);
    ui_create_key_btn(ui_PasswordSetPage, 91, 128, "2", ui_event_password_keypad, (uintptr_t)2u);
    ui_create_key_btn(ui_PasswordSetPage, 158, 128, "3", ui_event_password_keypad, (uintptr_t)3u);
    ui_create_key_btn(ui_PasswordSetPage, 24, 172, "4", ui_event_password_keypad, (uintptr_t)4u);
    ui_create_key_btn(ui_PasswordSetPage, 91, 172, "5", ui_event_password_keypad, (uintptr_t)5u);
    ui_create_key_btn(ui_PasswordSetPage, 158, 172, "6", ui_event_password_keypad, (uintptr_t)6u);
    ui_create_key_btn(ui_PasswordSetPage, 24, 216, "7", ui_event_password_keypad, (uintptr_t)7u);
    ui_create_key_btn(ui_PasswordSetPage, 91, 216, "8", ui_event_password_keypad, (uintptr_t)8u);
    ui_create_key_btn(ui_PasswordSetPage, 158, 216, "9", ui_event_password_keypad, (uintptr_t)9u);
    ui_create_key_btn(ui_PasswordSetPage, 24, 260, "CLR", ui_event_password_keypad, (uintptr_t)10u);
    ui_create_key_btn(ui_PasswordSetPage, 91, 260, "0", ui_event_password_keypad, (uintptr_t)0u);
    ui_create_key_btn(ui_PasswordSetPage, 158, 260, "SAVE", ui_event_password_keypad, (uintptr_t)11u);

    ui_password_refresh();
}

void ui_LockScreen_screen_init(void)
{
    lv_obj_t * title = 0;

    ui_unlock_clear("Enter PIN");

    ui_LockPage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_LockPage, LV_OBJ_FLAG_SCROLLABLE);

    title = lv_label_create(ui_LockPage);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);
    lv_label_set_text(title, "LOCKED");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LockPinLabel = lv_label_create(ui_LockPage);
    lv_obj_align(ui_LockPinLabel, LV_ALIGN_TOP_MID, 0, 62);
    lv_obj_set_style_text_font(ui_LockPinLabel, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_LockNoticeLabel = lv_label_create(ui_LockPage);
    lv_obj_align(ui_LockNoticeLabel, LV_ALIGN_TOP_MID, 0, 102);
    lv_obj_set_style_text_color(ui_LockNoticeLabel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_LockNoticeLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_create_key_btn(ui_LockPage, 24, 128, "1", ui_event_unlock_keypad, (uintptr_t)1u);
    ui_create_key_btn(ui_LockPage, 91, 128, "2", ui_event_unlock_keypad, (uintptr_t)2u);
    ui_create_key_btn(ui_LockPage, 158, 128, "3", ui_event_unlock_keypad, (uintptr_t)3u);
    ui_create_key_btn(ui_LockPage, 24, 172, "4", ui_event_unlock_keypad, (uintptr_t)4u);
    ui_create_key_btn(ui_LockPage, 91, 172, "5", ui_event_unlock_keypad, (uintptr_t)5u);
    ui_create_key_btn(ui_LockPage, 158, 172, "6", ui_event_unlock_keypad, (uintptr_t)6u);
    ui_create_key_btn(ui_LockPage, 24, 216, "7", ui_event_unlock_keypad, (uintptr_t)7u);
    ui_create_key_btn(ui_LockPage, 91, 216, "8", ui_event_unlock_keypad, (uintptr_t)8u);
    ui_create_key_btn(ui_LockPage, 158, 216, "9", ui_event_unlock_keypad, (uintptr_t)9u);
    ui_create_key_btn(ui_LockPage, 24, 260, "CLR", ui_event_unlock_keypad, (uintptr_t)10u);
    ui_create_key_btn(ui_LockPage, 91, 260, "0", ui_event_unlock_keypad, (uintptr_t)0u);
    ui_create_key_btn(ui_LockPage, 158, 260, "OK", ui_event_unlock_keypad, (uintptr_t)11u);

    ui_unlock_refresh("Enter PIN");
}

void ui_PasswordSetPage_screen_deinit(void)
{}

void ui_LockScreen_screen_deinit(void)
{}

uint8_t ui_LockScreenIsActive(void)
{
    Page_t *now_page = Page_Get_NowPage();
    return (uint8_t)(now_page != 0 && now_page->page_obj == &ui_LockPage);
}

void ui_LockScreenShowIfNeeded(void)
{
    Page_t *now_page = Page_Get_NowPage();

    if (!g_app_data.password_enabled || now_page == 0 || now_page->page_obj == &ui_LockPage)
    {
        return;
    }

    Page_Load(&Page_LockScreen);
}
