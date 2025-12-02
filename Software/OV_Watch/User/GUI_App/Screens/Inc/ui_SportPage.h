#ifndef _UI_SPORTPAGE_H
#define _UI_SPORTPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../ui.h"

extern lv_obj_t * ui_SportPage;
extern lv_obj_t * ui_SportHistoryPage;
extern lv_obj_t * ui_HealthTrendPage;
extern Page_t Page_Sport;
extern Page_t Page_SportHistory;
extern Page_t Page_HealthTrend;

void ui_SportPage_screen_init(void);
void ui_SportPage_screen_deinit(void);
void ui_SportHistoryPage_screen_init(void);
void ui_SportHistoryPage_screen_deinit(void);
void ui_HealthTrendPage_screen_init(void);
void ui_HealthTrendPage_screen_deinit(void);

uint8_t ui_SportSessionIsActive(void);
uint8_t ui_SportSessionIsRunning(void);
uint8_t ui_SportSessionCanLeave(void);
void ui_SportSessionHandleHrSample(uint8_t hr);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
