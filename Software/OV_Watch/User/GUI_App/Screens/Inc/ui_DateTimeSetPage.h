#ifndef _DATEDAYSETPAGE_UI_H
#define _DATEDAYSETPAGE_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"
#include "../../ui.h"

extern lv_obj_t * ui_DateTimeSetPage;
extern lv_obj_t * ui_APPSyPanel;
extern lv_obj_t * ui_APPSySwitch;
extern lv_obj_t * ui_APPSyLabel;
extern lv_obj_t * ui_DateSetPanel;
extern lv_obj_t * ui_DateSetLabel;
extern lv_obj_t * ui_DateSetLabel1;
extern lv_obj_t * ui_DateSetLabel2;
extern lv_obj_t * ui_DateSetLabel3;
extern lv_obj_t * ui_TimeSetPanel;
extern lv_obj_t * ui_TimeSetLabel;
extern lv_obj_t * ui_TimeSetLabel1;
extern lv_obj_t * ui_TimeSetLabel2;
extern lv_obj_t * ui_TimeSetLabel3;
extern lv_obj_t * ui_DateSetPage;
extern lv_obj_t * ui_YearSetRoller;
extern lv_obj_t * ui_MonthSetRoller;
extern lv_obj_t * ui_DaySetRoller;
extern lv_obj_t * ui_DateSetOKButton;
extern lv_obj_t * ui_DateSetOKicon;
extern lv_obj_t * ui_TimeSetPage;
extern lv_obj_t * ui_HourSetRoller;
extern lv_obj_t * ui_TimePoint;
extern lv_obj_t * ui_MinSetRoller;
extern lv_obj_t * ui_TimePoint1;
extern lv_obj_t * ui_SecSetRoller;
extern lv_obj_t * ui_TimeSetOKButton;
extern lv_obj_t * ui_TimeSetOKicon;
extern lv_obj_t * ui_AlarmSetPanel;
extern lv_obj_t * ui_AlarmSetLabel;
extern lv_obj_t * ui_AlarmValueLabel;
extern lv_obj_t * ui_AlarmSetPage;
extern lv_obj_t * ui_AlarmEnablePanel;
extern lv_obj_t * ui_AlarmEnableSwitch;
extern lv_obj_t * ui_AlarmEnableLabel;
extern lv_obj_t * ui_AlarmHourSetRoller;
extern lv_obj_t * ui_AlarmPoint;
extern lv_obj_t * ui_AlarmMinSetRoller;
extern lv_obj_t * ui_AlarmSaveButton;
extern lv_obj_t * ui_AlarmSaveLabel;
extern lv_obj_t * ui_AlarmReminderPage;
extern lv_obj_t * ui_AlarmReminderTitle;
extern lv_obj_t * ui_AlarmReminderTimeLabel;
extern lv_obj_t * ui_AlarmReminderHintLabel;
extern lv_obj_t * ui_AlarmReminderStopButton;
extern lv_obj_t * ui_AlarmReminderStopLabel;

extern Page_t Page_DateTimeSet;
extern Page_t Page_DateSet;
extern Page_t Page_TimeSet;
extern Page_t Page_AlarmSet;
extern Page_t Page_AlarmReminder;

void ui_DateTimeSetPage_screen_init(void);
void ui_DateSetPage_screen_init(void);
void ui_TimeSetPage_screen_init(void);
void ui_AlarmSetPage_screen_init(void);
void ui_AlarmReminderPage_screen_init(void);

void ui_DateTimeSetPage_screen_deinit(void);
void ui_DateSetPage_screen_deinit(void);
void ui_TimeSetPage_screen_deinit(void);
void ui_AlarmSetPage_screen_deinit(void);
void ui_AlarmReminderPage_screen_deinit(void);

void ui_AlarmReminderTrigger(void);
void ui_AlarmReminderDismiss(void);
uint8_t ui_AlarmReminderIsActive(void);

extern uint8_t ui_APPSy_EN;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
