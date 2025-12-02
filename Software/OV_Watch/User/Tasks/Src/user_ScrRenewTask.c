/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "user_ScrRenewTask.h"
#include "main.h"
#include "lvgl.h"
#include "ui_HomePage.h"
#include "ui_LockPage.h"
#include "ui_MenuPage.h"
#include "ui_SportPage.h"
#include "ui_DateTimeSetPage.h"
#include "ui_Megboxes.h"
#include "stm32f4xx_it.h"
#include "AppData.h"
#include "em70x8.h"
#include "LSM303.h"


/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
extern osMessageQueueId_t Key_MessageQueue;

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  Screen renew task
  * @param  argument: Not used
  * @retval None
  */
void ScrRenewTask(void *argument)
{
	uint8_t keystr=0;
	uint8_t IdleBreakstr=0;
    char notification_title[APPDATA_NOTIFY_TITLE_MAX];
    char notification_body[APPDATA_NOTIFY_BODY_MAX];
	while(1)
	{
        if(HardInt_alarm_flag)
        {
            HardInt_alarm_flag = 0;
            osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1);
            if(!ui_AlarmReminderIsActive())
            {
                ui_AlarmReminderTrigger();
            }
        }

        if(!ui_AlarmReminderIsActive() &&
           AppRuntime_ConsumePhoneNotification(notification_title, sizeof(notification_title), notification_body,
                                              sizeof(notification_body)))
        {
            osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1);
            ui_RemoteAlertShow(notification_title, notification_body);
        }

        if(!ui_AlarmReminderIsActive() && AppRuntime_ConsumeFindWatchRequest())
        {
            osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1);
            ui_RemoteAlertTrigger();
        }

		if(osMessageQueueGet(Key_MessageQueue,&keystr,NULL,0)==osOK)
		{
            if(ui_AlarmReminderIsActive())
            {
                ui_AlarmReminderDismiss();
                osDelay(10);
                continue;
            }

            if(ui_RemoteAlertIsActive())
            {
                ui_RemoteAlertDismiss();
                osDelay(10);
                continue;
            }

			//key1 pressed
			if(keystr == 1)
			{
                if(ui_LockScreenIsActive())
                {
                    osDelay(10);
                    continue;
                }

                if(Page_Get_NowPage()->page_obj == &ui_SportPage && !ui_SportSessionCanLeave())
                {
                    ui_mbox_create((uint8_t *)"Stop first");
                    osDelay(10);
                    continue;
                }

				Page_Back();
				if(Page_Get_NowPage()->page_obj == &ui_MenuPage)
				{
					//HR sensor sleep
					EM7028_hrs_DisEnable();
					//sensor sleep
					LSM303DLH_Sleep();
					// SPL_Sleep();
				}
			}
			//key2 pressed
			else if(keystr == 2)
			{
                if(ui_LockScreenIsActive())
                {
                    osDelay(10);
                    continue;
                }

                if(Page_Get_NowPage()->page_obj == &ui_SportPage && !ui_SportSessionCanLeave())
                {
                    ui_mbox_create((uint8_t *)"Stop first");
                    osDelay(10);
                    continue;
                }

				Page_Back_Bottom();
				//HR sensor sleep
				EM7028_hrs_DisEnable();
  				//sensor sleep
				LSM303DLH_Sleep();
				// SPL_Sleep();
			}
		}
		osDelay(10);
	}
}
