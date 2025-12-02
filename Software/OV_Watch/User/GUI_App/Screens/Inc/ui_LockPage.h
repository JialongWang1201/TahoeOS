#ifndef _UI_LOCKPAGE_H
#define _UI_LOCKPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../ui.h"

extern lv_obj_t * ui_PasswordSetPage;
extern lv_obj_t * ui_LockPage;
extern Page_t Page_PasswordSet;
extern Page_t Page_LockScreen;

void ui_PasswordSetPage_screen_init(void);
void ui_PasswordSetPage_screen_deinit(void);
void ui_LockScreen_screen_init(void);
void ui_LockScreen_screen_deinit(void);

uint8_t ui_LockScreenIsActive(void);
void ui_LockScreenShowIfNeeded(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
