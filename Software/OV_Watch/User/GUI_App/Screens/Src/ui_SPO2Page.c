#include "../../ui.h"
#include "../../ui_helpers.h"
#include "../Inc/ui_SPO2Page.h"
#include "../../../Func/Inc/HWDataAccess.h"

///////////////////// Page Manager //////////////////
Page_t Page_SPO2 = {ui_SPO2Page_screen_init, ui_SPO2Page_screen_deinit, &ui_SPO2Page};

///////////////////// VARIABLES ////////////////////
lv_obj_t * ui_SPO2Page;
lv_obj_t * ui_SPO2NumLabel;
lv_obj_t * ui_SPO2UnitLabel;
lv_obj_t * ui_SPO2NoticeLabel;
lv_obj_t * ui_SPO2MetaLabel;
lv_obj_t * ui_SPO2HintLabel;
lv_obj_t * ui_SPO2Icon;

lv_timer_t * ui_SPO2PageTimer;

static void SPO2Page_refresh(void)
{
    char value_strbuf[8];
    char meta_strbuf[80];
    const char *notice_text;
    const char *hint_text;
    lv_color_t icon_color;

    if(HWInterface.HR_meter.SPO2 > 0U)
    {
        snprintf(value_strbuf, sizeof(value_strbuf), "%u", HWInterface.HR_meter.SPO2);
        lv_label_set_text(ui_SPO2NumLabel, value_strbuf);
        lv_label_set_text(ui_SPO2UnitLabel, "%");
    }
    else
    {
        lv_label_set_text(ui_SPO2NumLabel, "--");
        lv_label_set_text(ui_SPO2UnitLabel, "N/A");
    }

    notice_text = "Place finger";
    hint_text = "Green LED PPG";
    icon_color = lv_color_hex(0x808080);
    switch(HWInterface.HR_meter.SPO2State)
    {
    case HW_SPO2_STATE_MEASURING:
        notice_text = "Sampling PPG...";
        hint_text = "Vendor algorithm warming up";
        icon_color = lv_color_hex(0x0080FF);
        break;
    case HW_SPO2_STATE_SIGNAL_WEAK:
        notice_text = "Signal weak";
        hint_text = "Cover the sensor fully";
        icon_color = lv_color_hex(0xFF8A00);
        break;
    case HW_SPO2_STATE_UNCALIBRATED:
        notice_text = "No stable estimate";
        hint_text = "Green LED estimate only";
        icon_color = lv_color_hex(0x00A99D);
        break;
    case HW_SPO2_STATE_READY:
        notice_text = "Estimated SpO2";
        hint_text = "Hold steady for better confidence";
        icon_color = lv_color_hex(0x00A99D);
        break;
    default:
        break;
    }

    lv_label_set_text(ui_SPO2NoticeLabel, notice_text);
    lv_label_set_text(ui_SPO2HintLabel, hint_text);
    lv_obj_set_style_text_color(ui_SPO2Icon, icon_color, LV_PART_MAIN | LV_STATE_DEFAULT);

    snprintf(meta_strbuf, sizeof(meta_strbuf), "RAW %u  AMP %u  Q %u%%\nALG HR %u  SPO %u  WR %u",
        HWInterface.HR_meter.RawSample,
        HWInterface.HR_meter.SampleAmplitude,
        HWInterface.HR_meter.SignalQuality,
        HWInterface.HR_meter.VendorHr,
        HWInterface.HR_meter.VendorSpo2,
        HWInterface.HR_meter.VendorWearState);
    lv_label_set_text(ui_SPO2MetaLabel, meta_strbuf);
}

/////////////////// private Timer ///////////////////
// need to be destroyed when the page is destroyed
static void SPO2Page_timer_cb(lv_timer_t * timer)
{
    LV_UNUSED(timer);
    SPO2Page_refresh();
}

///////////////////// SCREEN init ////////////////////
void ui_SPO2Page_screen_init(void)
{
    ui_SPO2Page = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_SPO2Page, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_SPO2NumLabel = lv_label_create(ui_SPO2Page);
    lv_obj_set_width(ui_SPO2NumLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_SPO2NumLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_y(ui_SPO2NumLabel, 8);
    lv_obj_set_align(ui_SPO2NumLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_SPO2NumLabel, "--");
    lv_obj_set_style_text_font(ui_SPO2NumLabel, &ui_font_Cuyuan80, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SPO2UnitLabel = lv_label_create(ui_SPO2Page);
    lv_obj_set_width(ui_SPO2UnitLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_SPO2UnitLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_SPO2UnitLabel, 82);
    lv_obj_set_y(ui_SPO2UnitLabel, 18);
    lv_obj_set_align(ui_SPO2UnitLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_SPO2UnitLabel, "N/A");
    lv_obj_set_style_text_font(ui_SPO2UnitLabel, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SPO2NoticeLabel = lv_label_create(ui_SPO2Page);
    lv_obj_set_width(ui_SPO2NoticeLabel, 200);
    lv_obj_set_height(ui_SPO2NoticeLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_SPO2NoticeLabel, 0);
    lv_obj_set_y(ui_SPO2NoticeLabel, 84);
    lv_obj_set_align(ui_SPO2NoticeLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_SPO2NoticeLabel, "Place finger");
    lv_obj_set_style_text_align(ui_SPO2NoticeLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_SPO2NoticeLabel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_SPO2NoticeLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_SPO2NoticeLabel, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SPO2MetaLabel = lv_label_create(ui_SPO2Page);
    lv_obj_set_width(ui_SPO2MetaLabel, 210);
    lv_obj_set_height(ui_SPO2MetaLabel, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_SPO2MetaLabel, 0);
    lv_obj_set_y(ui_SPO2MetaLabel, 110);
    lv_obj_set_align(ui_SPO2MetaLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_SPO2MetaLabel, "RAW 0  AMP 0  Q 0%\nALG HR 0  SPO 0  WR 0");
    lv_obj_set_style_text_align(ui_SPO2MetaLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_SPO2MetaLabel, lv_color_hex(0x6A6A6A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_SPO2MetaLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SPO2HintLabel = lv_label_create(ui_SPO2Page);
    lv_obj_set_width(ui_SPO2HintLabel, 200);
    lv_obj_set_height(ui_SPO2HintLabel, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_SPO2HintLabel, 0);
    lv_obj_set_y(ui_SPO2HintLabel, 148);
    lv_obj_set_align(ui_SPO2HintLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_SPO2HintLabel, "Green LED PPG");
    lv_obj_set_style_text_align(ui_SPO2HintLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_SPO2HintLabel, lv_color_hex(0xA0A0A0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_SPO2HintLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SPO2Icon = lv_label_create(ui_SPO2Page);
    lv_obj_set_width(ui_SPO2Icon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_SPO2Icon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_SPO2Icon, 0);
    lv_obj_set_y(ui_SPO2Icon, 26);
    lv_obj_set_align(ui_SPO2Icon, LV_ALIGN_TOP_MID);
    lv_label_set_text(ui_SPO2Icon, "");
    lv_obj_set_style_text_color(ui_SPO2Icon, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_SPO2Icon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_SPO2Icon, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    //timer
    ui_SPO2PageTimer = lv_timer_create(SPO2Page_timer_cb, 500,  NULL);
    SPO2Page_refresh();

}

/////////////////// SCREEN deinit ////////////////////
void ui_SPO2Page_screen_deinit(void)
{
    lv_timer_del(ui_SPO2PageTimer);
}
