#include "../../ui.h"
#include "../../ui_helpers.h"
#include "../Inc/ui_Megboxes.h"
#include "../Inc/ui_NFCCardPage.h"

#include "../../../Func/Inc/AppData.h"
#include "../../../Tasks/Inc/user_TasksInit.h"

#include <stdio.h>

Page_t Page_NFCCard = {ui_NFCCardPage_screen_init, ui_NFCCardPage_screen_deinit, &ui_NFCCardPage};

lv_obj_t * ui_NFCCardPage;
lv_obj_t * ui_Card1Panel;
lv_obj_t * ui_Card1Btn;
lv_obj_t * ui_Card1icon;
lv_obj_t * ui_Card1Label;
lv_obj_t * ui_WriteBtn1;
lv_obj_t * ui_WriteLabel1;
lv_obj_t * ui_ChooseBtn1;
lv_obj_t * ui_ChooseLabel1;

lv_obj_t * ui_Card2Panel;
lv_obj_t * ui_Card2Btn;
lv_obj_t * ui_Card2icon;
lv_obj_t * ui_Card2Label;
lv_obj_t * ui_WriteBtn2;
lv_obj_t * ui_WriteLabel2;
lv_obj_t * ui_ChooseBtn2;
lv_obj_t * ui_ChooseLabel2;

lv_obj_t * ui_Card3Panel;
lv_obj_t * ui_Card3Btn;
lv_obj_t * ui_Card3icon;
lv_obj_t * ui_Card3Label;
lv_obj_t * ui_WriteBtn3;
lv_obj_t * ui_WriteLabel3;
lv_obj_t * ui_ChooseBtn3;
lv_obj_t * ui_ChooseLabel3;

static lv_obj_t * ui_NFCTitleLabel;
static lv_obj_t * ui_NFCStatusLabel;
static lv_obj_t * ui_NFCHintLabel;

static uint32_t ui_nfc_panel_color(uint8_t slot_index)
{
    switch (slot_index)
    {
        case 1:
            return 0x0F800Fu;
        case 2:
            return 0xF59A23u;
        default:
            return 0x0F80FFu;
    }
}

static uint8_t ui_nfc_next_profile(uint8_t profile)
{
    switch (profile)
    {
        case APPDATA_NFC_PROFILE_CAMPUS:
            return APPDATA_NFC_PROFILE_DOOR;
        case APPDATA_NFC_PROFILE_DOOR:
            return APPDATA_NFC_PROFILE_TRANSIT;
        case APPDATA_NFC_PROFILE_TRANSIT:
            return APPDATA_NFC_PROFILE_NONE;
        default:
            return APPDATA_NFC_PROFILE_CAMPUS;
    }
}

static void ui_nfc_normalize_active_slot(void)
{
    if (AppData_IsValidNfcSlot(g_app_data.active_nfc_slot))
    {
        return;
    }

    g_app_data.active_nfc_slot = AppData_FindFirstNfcSlot();
    if (g_app_data.active_nfc_slot == APPDATA_NFC_SLOT_NONE)
    {
        g_app_data.nfc_enabled = 0;
    }
}

static void ui_nfc_style_action_btn(lv_obj_t *btn)
{
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, 110, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn, lv_color_hex(0xD0D0D0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void ui_nfc_refresh_panel(uint8_t slot_index,
                                 lv_obj_t *panel,
                                 lv_obj_t *label,
                                 lv_obj_t *write_btn,
                                 lv_obj_t *write_label,
                                 lv_obj_t *choose_btn,
                                 lv_obj_t *choose_label)
{
    uint8_t slot_valid = AppData_IsValidNfcSlot(slot_index);
    uint8_t slot_active = (uint8_t)(g_app_data.active_nfc_slot == slot_index);
    uint8_t slot_enabled = (uint8_t)(slot_valid && slot_active && g_app_data.nfc_enabled);

    lv_label_set_text(label, AppData_GetNfcSlotName(slot_index));
    lv_label_set_text(write_label, slot_valid ? "CYCLE" : "WRITE");

    if (!slot_valid)
    {
        lv_label_set_text(choose_label, "EMPTY");
        lv_obj_set_style_bg_opa(panel, 120, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(panel, lv_color_hex(0x606060), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(choose_btn, lv_color_hex(0x505050), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(choose_btn, 180, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(choose_label, lv_color_hex(0xF0F0F0), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    else if (slot_enabled)
    {
        lv_label_set_text(choose_label, "ACTIVE");
        lv_obj_set_style_bg_opa(panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(panel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(panel, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(choose_btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(choose_btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(choose_label, lv_color_hex(ui_nfc_panel_color(slot_index)), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    else
    {
        lv_label_set_text(choose_label, "ENABLE");
        lv_obj_set_style_bg_opa(panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(panel,
                                      slot_active ? lv_color_hex(0xD0D0D0) : lv_color_hex(0x3C3C3C),
                                      LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(panel, slot_active ? 2 : 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(choose_btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(choose_btn, 110, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(choose_label, lv_color_hex(0x202020), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    lv_obj_set_style_bg_color(write_btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(write_btn, 110, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void ui_nfc_refresh_status(void)
{
    char status_buf[32];

    if (AppData_IsValidNfcSlot(g_app_data.active_nfc_slot))
    {
        snprintf(status_buf,
                 sizeof(status_buf),
                 "%s %s",
                 g_app_data.nfc_enabled ? "NFC ON" : "DEFAULT",
                 AppData_GetNfcSlotName(g_app_data.active_nfc_slot));
        lv_label_set_text(ui_NFCStatusLabel, status_buf);
    }
    else
    {
        lv_label_set_text(ui_NFCStatusLabel, "NO ACTIVE CARD");
    }

    lv_label_set_text(ui_NFCHintLabel, "CYCLE rewrites the slot");
    ui_nfc_refresh_panel(0, ui_Card1Panel, ui_Card1Label, ui_WriteBtn1, ui_WriteLabel1, ui_ChooseBtn1, ui_ChooseLabel1);
    ui_nfc_refresh_panel(1, ui_Card2Panel, ui_Card2Label, ui_WriteBtn2, ui_WriteLabel2, ui_ChooseBtn2, ui_ChooseLabel2);
    ui_nfc_refresh_panel(2, ui_Card3Panel, ui_Card3Label, ui_WriteBtn3, ui_WriteLabel3, ui_ChooseBtn3, ui_ChooseLabel3);
}

static void ui_nfc_cycle_slot(uint8_t slot_index)
{
    g_app_data.nfc_slot_profile[slot_index] = ui_nfc_next_profile(g_app_data.nfc_slot_profile[slot_index]);
    ui_nfc_normalize_active_slot();
    if (g_app_data.active_nfc_slot == APPDATA_NFC_SLOT_NONE &&
        AppData_IsValidNfcSlot(slot_index))
    {
        g_app_data.active_nfc_slot = slot_index;
    }

    User_RequestDataSave();
    ui_nfc_refresh_status();
    ui_mbox_create((uint8_t *)(AppData_IsValidNfcSlot(slot_index) ? "Slot updated" : "Slot cleared"));
}

static void ui_nfc_select_slot(uint8_t slot_index)
{
    if (!AppData_IsValidNfcSlot(slot_index))
    {
        ui_mbox_create((uint8_t *)"Empty slot");
        return;
    }

    g_app_data.active_nfc_slot = slot_index;
    g_app_data.nfc_enabled = 1;
    User_RequestDataSave();
    ui_nfc_refresh_status();
    ui_mbox_create((uint8_t *)"NFC ready");
}

static void ui_event_WriteBtn1(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        ui_nfc_cycle_slot(0);
    }
}

static void ui_event_ChooseBtn1(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        ui_nfc_select_slot(0);
    }
}

static void ui_event_WriteBtn2(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        ui_nfc_cycle_slot(1);
    }
}

static void ui_event_ChooseBtn2(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        ui_nfc_select_slot(1);
    }
}

static void ui_event_WriteBtn3(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        ui_nfc_cycle_slot(2);
    }
}

static void ui_event_ChooseBtn3(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        ui_nfc_select_slot(2);
    }
}

static void ui_event_NFCCardPage(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_GESTURE &&
        lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_RIGHT)
    {
        Page_Back();
    }
}

void ui_NFCCardPage_screen_init(void)
{
    ui_nfc_normalize_active_slot();
    ui_NFCCardPage = lv_obj_create(NULL);
    lv_obj_set_scroll_dir(ui_NFCCardPage, LV_DIR_VER);
    lv_obj_set_style_pad_bottom(ui_NFCCardPage, 20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_NFCTitleLabel = lv_label_create(ui_NFCCardPage);
    lv_obj_align(ui_NFCTitleLabel, LV_ALIGN_TOP_LEFT, 16, 12);
    lv_label_set_text(ui_NFCTitleLabel, "CARD WALLET");
    lv_obj_set_style_text_font(ui_NFCTitleLabel, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_NFCStatusLabel = lv_label_create(ui_NFCCardPage);
    lv_obj_align(ui_NFCStatusLabel, LV_ALIGN_TOP_LEFT, 16, 40);
    lv_obj_set_style_text_color(ui_NFCStatusLabel, lv_color_hex(0x1980E1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_NFCStatusLabel, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_NFCHintLabel = lv_label_create(ui_NFCCardPage);
    lv_obj_align(ui_NFCHintLabel, LV_ALIGN_TOP_LEFT, 16, 64);
    lv_obj_set_style_text_color(ui_NFCHintLabel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_NFCHintLabel, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Card1Panel = lv_obj_create(ui_NFCCardPage);
    lv_obj_set_size(ui_Card1Panel, 200, 96);
    lv_obj_set_pos(ui_Card1Panel, 20, 92);
    lv_obj_clear_flag(ui_Card1Panel, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Card1Panel, lv_color_hex(ui_nfc_panel_color(0)), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Card1Panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Card1Btn = lv_btn_create(ui_Card1Panel);
    lv_obj_set_size(ui_Card1Btn, 40, 40);
    lv_obj_set_pos(ui_Card1Btn, 12, 10);
    lv_obj_clear_flag(ui_Card1Btn, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(ui_Card1Btn, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Card1Btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Card1Btn, 200, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Card1icon = lv_label_create(ui_Card1Btn);
    lv_obj_center(ui_Card1icon);
    lv_label_set_text(ui_Card1icon, "");
    lv_obj_set_style_text_color(ui_Card1icon, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Card1icon, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Card1Label = lv_label_create(ui_Card1Panel);
    lv_obj_set_pos(ui_Card1Label, 62, 16);
    lv_obj_set_style_text_color(ui_Card1Label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Card1Label, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_WriteBtn1 = lv_btn_create(ui_Card1Panel);
    lv_obj_set_size(ui_WriteBtn1, 80, 28);
    lv_obj_align(ui_WriteBtn1, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    ui_nfc_style_action_btn(ui_WriteBtn1);

    ui_WriteLabel1 = lv_label_create(ui_WriteBtn1);
    lv_obj_center(ui_WriteLabel1);
    lv_obj_set_style_text_font(ui_WriteLabel1, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ChooseBtn1 = lv_btn_create(ui_Card1Panel);
    lv_obj_set_size(ui_ChooseBtn1, 80, 28);
    lv_obj_align(ui_ChooseBtn1, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    ui_nfc_style_action_btn(ui_ChooseBtn1);

    ui_ChooseLabel1 = lv_label_create(ui_ChooseBtn1);
    lv_obj_center(ui_ChooseLabel1);
    lv_obj_set_style_text_font(ui_ChooseLabel1, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Card2Panel = lv_obj_create(ui_NFCCardPage);
    lv_obj_set_size(ui_Card2Panel, 200, 96);
    lv_obj_set_pos(ui_Card2Panel, 20, 204);
    lv_obj_clear_flag(ui_Card2Panel, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Card2Panel, lv_color_hex(ui_nfc_panel_color(1)), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Card2Panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Card2Btn = lv_btn_create(ui_Card2Panel);
    lv_obj_set_size(ui_Card2Btn, 40, 40);
    lv_obj_set_pos(ui_Card2Btn, 12, 10);
    lv_obj_clear_flag(ui_Card2Btn, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(ui_Card2Btn, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Card2Btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Card2Btn, 200, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Card2icon = lv_label_create(ui_Card2Btn);
    lv_obj_center(ui_Card2icon);
    lv_label_set_text(ui_Card2icon, "");
    lv_obj_set_style_text_color(ui_Card2icon, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Card2icon, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Card2Label = lv_label_create(ui_Card2Panel);
    lv_obj_set_pos(ui_Card2Label, 62, 16);
    lv_obj_set_style_text_color(ui_Card2Label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Card2Label, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_WriteBtn2 = lv_btn_create(ui_Card2Panel);
    lv_obj_set_size(ui_WriteBtn2, 80, 28);
    lv_obj_align(ui_WriteBtn2, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    ui_nfc_style_action_btn(ui_WriteBtn2);

    ui_WriteLabel2 = lv_label_create(ui_WriteBtn2);
    lv_obj_center(ui_WriteLabel2);
    lv_obj_set_style_text_font(ui_WriteLabel2, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ChooseBtn2 = lv_btn_create(ui_Card2Panel);
    lv_obj_set_size(ui_ChooseBtn2, 80, 28);
    lv_obj_align(ui_ChooseBtn2, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    ui_nfc_style_action_btn(ui_ChooseBtn2);

    ui_ChooseLabel2 = lv_label_create(ui_ChooseBtn2);
    lv_obj_center(ui_ChooseLabel2);
    lv_obj_set_style_text_font(ui_ChooseLabel2, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Card3Panel = lv_obj_create(ui_NFCCardPage);
    lv_obj_set_size(ui_Card3Panel, 200, 96);
    lv_obj_set_pos(ui_Card3Panel, 20, 316);
    lv_obj_clear_flag(ui_Card3Panel, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Card3Panel, lv_color_hex(ui_nfc_panel_color(2)), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Card3Panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Card3Btn = lv_btn_create(ui_Card3Panel);
    lv_obj_set_size(ui_Card3Btn, 40, 40);
    lv_obj_set_pos(ui_Card3Btn, 12, 10);
    lv_obj_clear_flag(ui_Card3Btn, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(ui_Card3Btn, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Card3Btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Card3Btn, 200, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Card3icon = lv_label_create(ui_Card3Btn);
    lv_obj_center(ui_Card3icon);
    lv_label_set_text(ui_Card3icon, "");
    lv_obj_set_style_text_color(ui_Card3icon, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Card3icon, &ui_font_iconfont30, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Card3Label = lv_label_create(ui_Card3Panel);
    lv_obj_set_pos(ui_Card3Label, 62, 16);
    lv_obj_set_style_text_color(ui_Card3Label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Card3Label, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_WriteBtn3 = lv_btn_create(ui_Card3Panel);
    lv_obj_set_size(ui_WriteBtn3, 80, 28);
    lv_obj_align(ui_WriteBtn3, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    ui_nfc_style_action_btn(ui_WriteBtn3);

    ui_WriteLabel3 = lv_label_create(ui_WriteBtn3);
    lv_obj_center(ui_WriteLabel3);
    lv_obj_set_style_text_font(ui_WriteLabel3, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_ChooseBtn3 = lv_btn_create(ui_Card3Panel);
    lv_obj_set_size(ui_ChooseBtn3, 80, 28);
    lv_obj_align(ui_ChooseBtn3, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    ui_nfc_style_action_btn(ui_ChooseBtn3);

    ui_ChooseLabel3 = lv_label_create(ui_ChooseBtn3);
    lv_obj_center(ui_ChooseLabel3);
    lv_obj_set_style_text_font(ui_ChooseLabel3, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_NFCCardPage, ui_event_NFCCardPage, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_WriteBtn1, ui_event_WriteBtn1, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_ChooseBtn1, ui_event_ChooseBtn1, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_WriteBtn2, ui_event_WriteBtn2, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_ChooseBtn2, ui_event_ChooseBtn2, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_WriteBtn3, ui_event_WriteBtn3, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_ChooseBtn3, ui_event_ChooseBtn3, LV_EVENT_ALL, NULL);

    ui_nfc_refresh_status();
}

void ui_NFCCardPage_screen_deinit(void)
{}
