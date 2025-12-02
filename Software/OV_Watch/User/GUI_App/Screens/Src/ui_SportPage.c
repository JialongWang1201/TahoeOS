#include "../../ui.h"
#include "../../ui_helpers.h"
#include "../Inc/ui_SportPage.h"
#include "../Inc/ui_Megboxes.h"

#include "../../../Func/Inc/AppData.h"
#include "../../../Func/Inc/HWDataAccess.h"
#include "../../../Tasks/Inc/user_TasksInit.h"

#include "task.h"
#include <stdio.h>
#include <string.h>

typedef enum
{
    SPORT_STATE_IDLE = 0,
    SPORT_STATE_RUNNING,
    SPORT_STATE_PAUSED
} SportState_t;

typedef struct
{
    SportState_t state;
    TickType_t tick_anchor;
    uint32_t elapsed_ms;
    uint16_t start_steps;
    int16_t start_altitude;
    uint32_t hr_sum;
    uint16_t hr_samples;
    uint8_t max_hr;
    HW_DateTimeTypeDef start_time;
} SportSession_t;

typedef struct
{
    lv_obj_t * panel;
    lv_obj_t * date_label;
    lv_obj_t * steps_label;
    lv_obj_t * meta_label;
    lv_obj_t * bar_bg;
    lv_obj_t * bar_fill;
} HealthTrendRowWidgets_t;

Page_t Page_Sport = {ui_SportPage_screen_init, ui_SportPage_screen_deinit, &ui_SportPage};
Page_t Page_SportHistory = {ui_SportHistoryPage_screen_init, ui_SportHistoryPage_screen_deinit, &ui_SportHistoryPage};
Page_t Page_HealthTrend = {ui_HealthTrendPage_screen_init, ui_HealthTrendPage_screen_deinit, &ui_HealthTrendPage};

lv_obj_t * ui_SportPage;
lv_obj_t * ui_SportHistoryPage;
lv_obj_t * ui_HealthTrendPage;

static lv_obj_t * ui_SportTitleLabel;
static lv_obj_t * ui_SportStatusLabel;
static lv_obj_t * ui_SportTimeValueLabel;
static lv_obj_t * ui_SportStepsLabel;
static lv_obj_t * ui_SportHrLabel;
static lv_obj_t * ui_SportAvgHrLabel;
static lv_obj_t * ui_SportAltLabel;
static lv_obj_t * ui_SportActionBtn;
static lv_obj_t * ui_SportActionLabel;
static lv_obj_t * ui_SportStopBtn;
static lv_obj_t * ui_SportStopLabel;
static lv_obj_t * ui_SportHistoryBtn;
static lv_obj_t * ui_SportHistoryLabel;
static lv_obj_t * ui_SportHistoryText;
static lv_obj_t * ui_HealthTrendTitleLabel;
static lv_obj_t * ui_HealthTrendHintLabel;
static lv_obj_t * ui_HealthTrendTodayPanel;
static lv_obj_t * ui_HealthTrendTodayLabel;
static HealthTrendRowWidgets_t ui_HealthTrendRows[APPDATA_HEALTH_DAY_COUNT];

static lv_timer_t * ui_SportPageTimer;
static lv_timer_t * ui_HealthTrendPageTimer;
static SportSession_t g_sport_session;

static uint32_t ui_sport_elapsed_ms(void)
{
    if (g_sport_session.state == SPORT_STATE_RUNNING)
    {
        return g_sport_session.elapsed_ms +
               ((uint32_t)(xTaskGetTickCount() - g_sport_session.tick_anchor) * portTICK_PERIOD_MS);
    }

    return g_sport_session.elapsed_ms;
}

static uint16_t ui_sport_step_delta(void)
{
    if (HWInterface.IMU.Steps < g_sport_session.start_steps)
    {
        return 0;
    }

    return (uint16_t)(HWInterface.IMU.Steps - g_sport_session.start_steps);
}

static int16_t ui_sport_alt_delta(void)
{
    return (int16_t)(HWInterface.Barometer.altitude - g_sport_session.start_altitude);
}

static void ui_sport_reset_session(void)
{
    memset(&g_sport_session, 0, sizeof(g_sport_session));
    g_sport_session.state = SPORT_STATE_IDLE;
}

static void ui_sport_start_session(void)
{
    ui_sport_reset_session();
    HWInterface.RealTimeClock.GetTimeDate(&g_sport_session.start_time);
    g_sport_session.tick_anchor = xTaskGetTickCount();
    g_sport_session.start_steps = HWInterface.IMU.ConnectionError ? 0u : HWInterface.IMU.GetSteps();
    g_sport_session.start_altitude = HWInterface.Barometer.ConnectionError ? 0 : (int16_t)Altitude_Calculate();
    g_sport_session.state = SPORT_STATE_RUNNING;
}

static void ui_sport_pause_session(void)
{
    g_sport_session.elapsed_ms = ui_sport_elapsed_ms();
    g_sport_session.state = SPORT_STATE_PAUSED;
}

static void ui_sport_resume_session(void)
{
    g_sport_session.tick_anchor = xTaskGetTickCount();
    g_sport_session.state = SPORT_STATE_RUNNING;
}

static void ui_sport_store_record(void)
{
    AppSportRecord_t record;
    uint32_t elapsed_sec = ui_sport_elapsed_ms() / 1000u;

    if (elapsed_sec == 0u && ui_sport_step_delta() == 0u && g_sport_session.hr_samples == 0u)
    {
        ui_sport_reset_session();
        return;
    }

    memset(&record, 0, sizeof(record));
    record.month = g_sport_session.start_time.Month;
    record.date = g_sport_session.start_time.Date;
    record.hour = g_sport_session.start_time.Hours;
    record.minute = g_sport_session.start_time.Minutes;
    record.duration_sec = (uint16_t)elapsed_sec;
    record.steps = ui_sport_step_delta();
    record.avg_hr = (g_sport_session.hr_samples == 0u) ? 0u :
        (uint8_t)(g_sport_session.hr_sum / g_sport_session.hr_samples);
    record.max_hr = g_sport_session.max_hr;
    record.altitude_delta = ui_sport_alt_delta();

    AppData_AddSportRecord(&record);
    AppData_AddHealthSportSession(g_sport_session.start_time.Year,
                                  g_sport_session.start_time.Month,
                                  g_sport_session.start_time.Date,
                                  g_sport_session.start_time.WeekDay,
                                  record.duration_sec);
    User_RequestDataSave();
    ui_sport_reset_session();
}

static void ui_sport_refresh_history_text(void)
{
    char text[256];
    char line[64];
    uint8_t i = 0;

    if (AppData_GetSportRecordCount() == 0u)
    {
        lv_label_set_text(ui_SportHistoryText, "NO RECORDS");
        return;
    }

    text[0] = '\0';
    for (i = 0; i < AppData_GetSportRecordCount(); ++i)
    {
        const AppSportRecord_t *record = AppData_GetSportRecord(i);
        if (record == 0)
        {
            continue;
        }

        snprintf(line, sizeof(line),
                 "%02u/%02u %02u:%02u  %02us\nSTEP %u  HR %u/%u  ALT %+dm\n\n",
                 record->month,
                 record->date,
                 record->hour,
                 record->minute,
                 record->duration_sec,
                 record->steps,
                 record->avg_hr,
                 record->max_hr,
                 record->altitude_delta);
        strncat(text, line, sizeof(text) - strlen(text) - 1u);
    }

    lv_label_set_text(ui_SportHistoryText, text);
}

static const char * ui_health_weekday_name(uint8_t weekday)
{
    static const char *names[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

    if (weekday < 1u || weekday > 7u)
    {
        return "---";
    }

    return names[weekday - 1u];
}

static void ui_health_refresh_screen(void)
{
    char text[160];
    char date_text[24];
    char steps_text[24];
    char meta_text[64];
    uint16_t max_steps = 1000u;
    uint8_t i = 0;
    uint8_t day_count = AppData_GetHealthDayCount();
    const AppHealthDaySummary_t *today = AppData_GetHealthDay(0u);

    if (ui_HealthTrendPage == 0)
    {
        return;
    }

    if (today == 0)
    {
        lv_label_set_text(ui_HealthTrendTodayLabel, "No history yet.\nUse the watch for a day to build the 7-day summary.");
    }
    else
    {
        snprintf(text, sizeof(text),
                 "Today  %u step  Sport %u\nHR %u/%u  Env %dC  %u%%  %umin",
                 today->steps,
                 today->sport_sessions,
                 today->avg_hr,
                 today->max_hr,
                 (int)today->avg_temp,
                 today->avg_humidity,
                 today->sport_minutes);
        lv_label_set_text(ui_HealthTrendTodayLabel, text);
    }

    for (i = 0; i < day_count; ++i)
    {
        const AppHealthDaySummary_t *day = AppData_GetHealthDay(i);
        if (day != 0 && day->steps > max_steps)
        {
            max_steps = day->steps;
        }
    }

    for (i = 0; i < APPDATA_HEALTH_DAY_COUNT; ++i)
    {
        HealthTrendRowWidgets_t *row = &ui_HealthTrendRows[i];
        const AppHealthDaySummary_t *day = AppData_GetHealthDay(i);
        lv_coord_t bar_width = 8;

        if (row->panel == 0)
        {
            continue;
        }

        if (day == 0)
        {
            lv_label_set_text(row->date_label, "--/--");
            lv_label_set_text(row->steps_label, "--");
            lv_label_set_text(row->meta_label, "No data");
            lv_obj_set_width(row->bar_fill, bar_width);
            continue;
        }

        snprintf(date_text, sizeof(date_text), "%s %02u/%02u",
                 ui_health_weekday_name(day->weekday), day->month, day->date);
        snprintf(steps_text, sizeof(steps_text), "%u", day->steps);
        snprintf(meta_text, sizeof(meta_text), "S%u  HR %u  %dC/%u%%  %umin",
                 day->sport_sessions, day->avg_hr, (int)day->avg_temp, day->avg_humidity, day->sport_minutes);

        bar_width = (lv_coord_t)(8u + ((uint32_t)day->steps * 136u) / max_steps);
        if (bar_width > 144)
        {
            bar_width = 144;
        }

        lv_label_set_text(row->date_label, date_text);
        lv_label_set_text(row->steps_label, steps_text);
        lv_label_set_text(row->meta_label, meta_text);
        lv_obj_set_width(row->bar_fill, bar_width);
    }
}

static void ui_sport_refresh_screen(lv_timer_t * timer)
{
    char buf[32];
    uint32_t elapsed_sec = ui_sport_elapsed_ms() / 1000u;
    uint32_t hour = elapsed_sec / 3600u;
    uint32_t minute = (elapsed_sec % 3600u) / 60u;
    uint32_t second = elapsed_sec % 60u;
    uint8_t avg_hr = (g_sport_session.hr_samples == 0u) ? 0u :
        (uint8_t)(g_sport_session.hr_sum / g_sport_session.hr_samples);

    LV_UNUSED(timer);

    if (ui_SportPage == 0)
    {
        return;
    }

    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu",
             (unsigned long)hour,
             (unsigned long)minute,
             (unsigned long)second);
    lv_label_set_text(ui_SportTimeValueLabel, buf);

    snprintf(buf, sizeof(buf), "STEP %u", ui_sport_step_delta());
    lv_label_set_text(ui_SportStepsLabel, buf);

    snprintf(buf, sizeof(buf), "HR %u", HWInterface.HR_meter.HrRate);
    lv_label_set_text(ui_SportHrLabel, buf);

    snprintf(buf, sizeof(buf), "AVG %u", avg_hr);
    lv_label_set_text(ui_SportAvgHrLabel, buf);

    snprintf(buf, sizeof(buf), "ALT %+dm", ui_sport_alt_delta());
    lv_label_set_text(ui_SportAltLabel, buf);

    switch (g_sport_session.state)
    {
        case SPORT_STATE_RUNNING:
            lv_label_set_text(ui_SportStatusLabel, "RUNNING");
            lv_label_set_text(ui_SportActionLabel, "PAUSE");
            break;
        case SPORT_STATE_PAUSED:
            lv_label_set_text(ui_SportStatusLabel, "PAUSED");
            lv_label_set_text(ui_SportActionLabel, "RESUME");
            break;
        default:
            lv_label_set_text(ui_SportStatusLabel, "READY");
            lv_label_set_text(ui_SportActionLabel, "START");
            break;
    }
}

static void ui_health_refresh_timer(lv_timer_t * timer)
{
    LV_UNUSED(timer);
    ui_health_refresh_screen();
}

static void ui_event_SportPage(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_GESTURE &&
        lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_RIGHT)
    {
        if (ui_SportSessionCanLeave())
        {
            Page_Back();
        }
        else
        {
            ui_mbox_create((uint8_t *)"Stop first");
        }
    }
}

static void ui_event_SportActionBtn(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    {
        return;
    }

    if (g_sport_session.state == SPORT_STATE_IDLE)
    {
        ui_sport_start_session();
    }
    else if (g_sport_session.state == SPORT_STATE_RUNNING)
    {
        ui_sport_pause_session();
    }
    else
    {
        ui_sport_resume_session();
    }
}

static void ui_event_SportStopBtn(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    {
        return;
    }

    if (g_sport_session.state == SPORT_STATE_IDLE)
    {
        return;
    }

    if (g_sport_session.state == SPORT_STATE_RUNNING)
    {
        ui_sport_pause_session();
    }

    ui_sport_store_record();
}

static void ui_event_SportHistoryBtn(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    {
        return;
    }

    if (!ui_SportSessionCanLeave())
    {
        ui_mbox_create((uint8_t *)"Stop first");
        return;
    }

    Page_Load(&Page_SportHistory);
}

static void ui_event_SportHistoryPage(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_GESTURE &&
        lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_RIGHT)
    {
        Page_Back();
    }
}

static void ui_event_HealthTrendPage(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_GESTURE &&
        lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_RIGHT)
    {
        Page_Back();
    }
}

void ui_SportPage_screen_init(void)
{
    ui_SportPage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_SportPage, LV_OBJ_FLAG_SCROLLABLE);

    ui_SportTitleLabel = lv_label_create(ui_SportPage);
    lv_obj_align(ui_SportTitleLabel, LV_ALIGN_TOP_LEFT, 16, 14);
    lv_label_set_text(ui_SportTitleLabel, "SPORT");
    lv_obj_set_style_text_font(ui_SportTitleLabel, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SportStatusLabel = lv_label_create(ui_SportPage);
    lv_obj_align(ui_SportStatusLabel, LV_ALIGN_TOP_RIGHT, -16, 16);
    lv_obj_set_style_text_color(ui_SportStatusLabel, lv_color_hex(0x1980E1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_SportStatusLabel, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SportTimeValueLabel = lv_label_create(ui_SportPage);
    lv_obj_align(ui_SportTimeValueLabel, LV_ALIGN_TOP_MID, 0, 56);
    lv_obj_set_style_text_font(ui_SportTimeValueLabel, &ui_font_Cuyuan48, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SportStepsLabel = lv_label_create(ui_SportPage);
    lv_obj_align(ui_SportStepsLabel, LV_ALIGN_TOP_LEFT, 20, 136);
    lv_obj_set_style_text_font(ui_SportStepsLabel, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SportHrLabel = lv_label_create(ui_SportPage);
    lv_obj_align(ui_SportHrLabel, LV_ALIGN_TOP_RIGHT, -20, 136);
    lv_obj_set_style_text_font(ui_SportHrLabel, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SportAvgHrLabel = lv_label_create(ui_SportPage);
    lv_obj_align(ui_SportAvgHrLabel, LV_ALIGN_TOP_LEFT, 20, 176);
    lv_obj_set_style_text_font(ui_SportAvgHrLabel, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SportAltLabel = lv_label_create(ui_SportPage);
    lv_obj_align(ui_SportAltLabel, LV_ALIGN_TOP_RIGHT, -20, 176);
    lv_obj_set_style_text_font(ui_SportAltLabel, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SportActionBtn = lv_btn_create(ui_SportPage);
    lv_obj_set_size(ui_SportActionBtn, 90, 44);
    lv_obj_align(ui_SportActionBtn, LV_ALIGN_BOTTOM_LEFT, 16, -18);
    ui_SportActionLabel = lv_label_create(ui_SportActionBtn);
    lv_obj_center(ui_SportActionLabel);
    lv_obj_set_style_text_font(ui_SportActionLabel, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SportStopBtn = lv_btn_create(ui_SportPage);
    lv_obj_set_size(ui_SportStopBtn, 90, 44);
    lv_obj_align(ui_SportStopBtn, LV_ALIGN_BOTTOM_MID, 0, -18);
    ui_SportStopLabel = lv_label_create(ui_SportStopBtn);
    lv_obj_center(ui_SportStopLabel);
    lv_label_set_text(ui_SportStopLabel, "STOP");
    lv_obj_set_style_text_font(ui_SportStopLabel, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SportHistoryBtn = lv_btn_create(ui_SportPage);
    lv_obj_set_size(ui_SportHistoryBtn, 90, 44);
    lv_obj_align(ui_SportHistoryBtn, LV_ALIGN_BOTTOM_RIGHT, -16, -18);
    ui_SportHistoryLabel = lv_label_create(ui_SportHistoryBtn);
    lv_obj_center(ui_SportHistoryLabel);
    lv_label_set_text(ui_SportHistoryLabel, "HIST");
    lv_obj_set_style_text_font(ui_SportHistoryLabel, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_SportPage, ui_event_SportPage, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SportActionBtn, ui_event_SportActionBtn, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SportStopBtn, ui_event_SportStopBtn, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_SportHistoryBtn, ui_event_SportHistoryBtn, LV_EVENT_ALL, NULL);

    ui_SportPageTimer = lv_timer_create(ui_sport_refresh_screen, 250, NULL);
    ui_sport_refresh_screen(NULL);
}

void ui_SportHistoryPage_screen_init(void)
{
    ui_SportHistoryPage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_SportHistoryPage, LV_OBJ_FLAG_SCROLLABLE);

    ui_SportTitleLabel = lv_label_create(ui_SportHistoryPage);
    lv_obj_align(ui_SportTitleLabel, LV_ALIGN_TOP_LEFT, 16, 14);
    lv_label_set_text(ui_SportTitleLabel, "HISTORY");
    lv_obj_set_style_text_font(ui_SportTitleLabel, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_SportHistoryText = lv_label_create(ui_SportHistoryPage);
    lv_obj_set_width(ui_SportHistoryText, 208);
    lv_obj_align(ui_SportHistoryText, LV_ALIGN_TOP_LEFT, 16, 52);
    lv_label_set_long_mode(ui_SportHistoryText, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(ui_SportHistoryText, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_SportHistoryPage, ui_event_SportHistoryPage, LV_EVENT_ALL, NULL);
    ui_sport_refresh_history_text();
}

void ui_HealthTrendPage_screen_init(void)
{
    uint8_t i = 0;

    ui_HealthTrendPage = lv_obj_create(NULL);
    lv_obj_set_scroll_dir(ui_HealthTrendPage, LV_DIR_VER);
    lv_obj_set_style_pad_bottom(ui_HealthTrendPage, 18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_HealthTrendTitleLabel = lv_label_create(ui_HealthTrendPage);
    lv_obj_align(ui_HealthTrendTitleLabel, LV_ALIGN_TOP_LEFT, 16, 14);
    lv_label_set_text(ui_HealthTrendTitleLabel, "HEALTH");
    lv_obj_set_style_text_font(ui_HealthTrendTitleLabel, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_HealthTrendHintLabel = lv_label_create(ui_HealthTrendPage);
    lv_obj_align(ui_HealthTrendHintLabel, LV_ALIGN_TOP_LEFT, 16, 42);
    lv_label_set_text(ui_HealthTrendHintLabel, "7-day steps / HR / env / sport");
    lv_obj_set_style_text_font(ui_HealthTrendHintLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_HealthTrendHintLabel, lv_color_hex(0x7A7A7A), LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_HealthTrendTodayPanel = lv_obj_create(ui_HealthTrendPage);
    lv_obj_set_size(ui_HealthTrendTodayPanel, 208, 74);
    lv_obj_set_pos(ui_HealthTrendTodayPanel, 16, 72);
    lv_obj_clear_flag(ui_HealthTrendTodayPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ui_HealthTrendTodayPanel, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_HealthTrendTodayPanel, lv_color_hex(0x102E36), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_HealthTrendTodayPanel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_HealthTrendTodayPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_HealthTrendTodayLabel = lv_label_create(ui_HealthTrendTodayPanel);
    lv_obj_set_width(ui_HealthTrendTodayLabel, 176);
    lv_obj_align(ui_HealthTrendTodayLabel, LV_ALIGN_TOP_LEFT, 14, 12);
    lv_label_set_long_mode(ui_HealthTrendTodayLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(ui_HealthTrendTodayLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_HealthTrendTodayLabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    for (i = 0; i < APPDATA_HEALTH_DAY_COUNT; ++i)
    {
        HealthTrendRowWidgets_t *row = &ui_HealthTrendRows[i];
        lv_coord_t top = (lv_coord_t)(164 + i * 58);

        memset(row, 0, sizeof(*row));

        row->panel = lv_obj_create(ui_HealthTrendPage);
        lv_obj_set_size(row->panel, 208, 52);
        lv_obj_set_pos(row->panel, 16, top);
        lv_obj_clear_flag(row->panel, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(row->panel, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(row->panel, lv_color_hex(0xF3F5F7), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(row->panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(row->panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        row->date_label = lv_label_create(row->panel);
        lv_obj_align(row->date_label, LV_ALIGN_TOP_LEFT, 12, 8);
        lv_obj_set_style_text_font(row->date_label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

        row->steps_label = lv_label_create(row->panel);
        lv_obj_align(row->steps_label, LV_ALIGN_TOP_RIGHT, -12, 8);
        lv_obj_set_style_text_font(row->steps_label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(row->steps_label, lv_color_hex(0x1A7F72), LV_PART_MAIN | LV_STATE_DEFAULT);

        row->meta_label = lv_label_create(row->panel);
        lv_obj_set_width(row->meta_label, 184);
        lv_obj_align(row->meta_label, LV_ALIGN_BOTTOM_LEFT, 12, -10);
        lv_label_set_long_mode(row->meta_label, LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_font(row->meta_label, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(row->meta_label, lv_color_hex(0x6A6A6A), LV_PART_MAIN | LV_STATE_DEFAULT);

        row->bar_bg = lv_obj_create(row->panel);
        lv_obj_set_size(row->bar_bg, 144, 4);
        lv_obj_align(row->bar_bg, LV_ALIGN_TOP_LEFT, 12, 28);
        lv_obj_clear_flag(row->bar_bg, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(row->bar_bg, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(row->bar_bg, lv_color_hex(0xD7DEE3), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(row->bar_bg, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(row->bar_bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(row->bar_bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        row->bar_fill = lv_obj_create(row->bar_bg);
        lv_obj_set_size(row->bar_fill, 8, 4);
        lv_obj_set_pos(row->bar_fill, 0, 0);
        lv_obj_clear_flag(row->bar_fill, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(row->bar_fill, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(row->bar_fill, lv_color_hex(0x18A999), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(row->bar_fill, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(row->bar_fill, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(row->bar_fill, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    lv_obj_add_event_cb(ui_HealthTrendPage, ui_event_HealthTrendPage, LV_EVENT_ALL, NULL);
    ui_HealthTrendPageTimer = lv_timer_create(ui_health_refresh_timer, 2000, NULL);
    ui_health_refresh_screen();
}

void ui_SportPage_screen_deinit(void)
{
    if (ui_SportPageTimer != 0)
    {
        lv_timer_del(ui_SportPageTimer);
        ui_SportPageTimer = 0;
    }
}

void ui_SportHistoryPage_screen_deinit(void)
{}

void ui_HealthTrendPage_screen_deinit(void)
{
    if (ui_HealthTrendPageTimer != 0)
    {
        lv_timer_del(ui_HealthTrendPageTimer);
        ui_HealthTrendPageTimer = 0;
    }
}

uint8_t ui_SportSessionIsActive(void)
{
    return (uint8_t)(g_sport_session.state != SPORT_STATE_IDLE);
}

uint8_t ui_SportSessionIsRunning(void)
{
    return (uint8_t)(g_sport_session.state == SPORT_STATE_RUNNING);
}

uint8_t ui_SportSessionCanLeave(void)
{
    return (uint8_t)(g_sport_session.state == SPORT_STATE_IDLE);
}

void ui_SportSessionHandleHrSample(uint8_t hr)
{
    if (g_sport_session.state != SPORT_STATE_RUNNING || hr < 40u || hr > 220u)
    {
        return;
    }

    g_sport_session.hr_sum += hr;
    g_sport_session.hr_samples++;
    if (hr > g_sport_session.max_hr)
    {
        g_sport_session.max_hr = hr;
    }
}
