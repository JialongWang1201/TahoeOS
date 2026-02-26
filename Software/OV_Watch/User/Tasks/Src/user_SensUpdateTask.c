/* Private includes -----------------------------------------------------------*/
//includes
#include "string.h"

#include "user_TasksInit.h"
#include "user_ScrRenewTask.h"
#include "user_SensUpdateTask.h"
#include "ui_HomePage.h"
#include "ui_MenuPage.h"
#include "ui_SetPage.h"
#include "ui_HRPage.h"
#include "ui_SPO2Page.h"
#include "ui_EnvPage.h"
#include "ui_CompassPage.h"
#include "ui_SportPage.h"
#include "main.h"

#include "AHT21.h"
#include "LSM303.h"
#include "SPL06_001.h"
#include "em70x8.h"
#include "HrAlgorythm.h"

#include "AppData.h"
#include "HWDataAccess.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define USER_SPO2_WINDOW_SIZE            24U
#define USER_SPO2_MIN_WINDOW_SAMPLES     8U
#define USER_SPO2_VALID_AMPLITUDE        150U
#define USER_SPO2_STRONG_AMPLITUDE       600U
#define USER_SPO2_MIN_VALUE              70U
#define USER_SPO2_MAX_VALUE              100U
#define USER_SPO2_STABLE_VALID_COUNT     3U
#define USER_SPO2_MISS_LIMIT             6U
#define USER_HEALTH_STEP_SYNC_LOOPS      20U
#define USER_HEALTH_SAVE_SYNC_LOOPS      1200U
#define USER_HEALTH_HR_SAMPLE_DIVIDER    20U

/* Private variables ---------------------------------------------------------*/
uint32_t user_HR_timecount=0;

typedef struct
{
    uint16_t samples[USER_SPO2_WINDOW_SIZE];
    uint8_t index;
    uint8_t count;
    uint8_t active;
    uint8_t stable_valid_count;
    uint8_t miss_count;
} user_spo2_measurement_t;

static user_spo2_measurement_t user_spo2_measurement;

/* Private function prototypes -----------------------------------------------*/
// 这是EM7028官方lib的库函数, 没有lib用不了
extern uint8_t GET_BP_MAX (void);
extern uint8_t GET_BP_MIN (void);
extern void Blood_Process (void);
extern void Blood_50ms_process (void);
extern void Blood_500ms_process(void);
extern int em70xx_bpm_dynamic(int RECEIVED_BYTE, int g_sensor_x, int g_sensor_y, int g_sensor_z);
extern int em70xx_reset(int ref);
extern void hrspo_sensor_handler(void);
extern void hrspo_sensor_get(uint8_t *hr_value, uint8_t *spo2_value, uint8_t *wear_state);

static uint8_t user_SPO2IsPlausibleValue(uint8_t value)
{
    return (uint8_t)(value >= USER_SPO2_MIN_VALUE && value <= USER_SPO2_MAX_VALUE);
}

static uint8_t user_SPO2ReadVendorEstimate(uint8_t *spo2_value)
{
    uint8_t hr_value = 0u;
    uint8_t vendor_spo2 = 0u;
    uint8_t wear_state = 0u;

    if (spo2_value == 0)
    {
        return 0u;
    }

    hrspo_sensor_handler();
    hrspo_sensor_get(&hr_value, &vendor_spo2, &wear_state);

    HWInterface.HR_meter.VendorWearState = wear_state;
    HWInterface.HR_meter.VendorHr = hr_value;
    HWInterface.HR_meter.VendorSpo2 = vendor_spo2;

    if (!user_SPO2IsPlausibleValue(vendor_spo2))
    {
        *spo2_value = 0u;
        return 0u;
    }

    *spo2_value = vendor_spo2;
    return 1u;
}

static void user_SPO2ResetMeasurement(void)
{
    memset(&user_spo2_measurement, 0, sizeof(user_spo2_measurement));
    em70xx_reset(0);
    HWInterface.HR_meter.SPO2 = 0;
    HWInterface.HR_meter.SPO2State = HW_SPO2_STATE_IDLE;
    HWInterface.HR_meter.SignalQuality = 0;
    HWInterface.HR_meter.VendorWearState = 0;
    HWInterface.HR_meter.VendorHr = 0;
    HWInterface.HR_meter.VendorSpo2 = 0;
    HWInterface.HR_meter.RawSample = 0;
    HWInterface.HR_meter.SampleAmplitude = 0;
}

static void user_SPO2UpdateMeasurement(uint16_t sample)
{
    uint8_t i;
    uint16_t min_sample;
    uint16_t max_sample;
    uint16_t current_sample;
    uint16_t amplitude;
    uint8_t vendor_spo2 = 0u;
    uint8_t quality;

    user_spo2_measurement.active = 1;
    user_spo2_measurement.samples[user_spo2_measurement.index] = sample;
    user_spo2_measurement.index++;
    if(user_spo2_measurement.index >= USER_SPO2_WINDOW_SIZE)
    {
        user_spo2_measurement.index = 0;
    }
    if(user_spo2_measurement.count < USER_SPO2_WINDOW_SIZE)
    {
        user_spo2_measurement.count++;
    }

    HWInterface.HR_meter.RawSample = sample;

    user_SPO2ReadVendorEstimate(&vendor_spo2);

    if(user_spo2_measurement.count < USER_SPO2_MIN_WINDOW_SAMPLES)
    {
        HWInterface.HR_meter.SPO2State = HW_SPO2_STATE_MEASURING;
        HWInterface.HR_meter.SignalQuality = 0;
        HWInterface.HR_meter.SampleAmplitude = 0;
        return;
    }

    min_sample = user_spo2_measurement.samples[0];
    max_sample = user_spo2_measurement.samples[0];
    for(i = 1; i < user_spo2_measurement.count; i++)
    {
        current_sample = user_spo2_measurement.samples[i];

        if(current_sample < min_sample)
        {
            min_sample = current_sample;
        }
        if(current_sample > max_sample)
        {
            max_sample = current_sample;
        }
    }

    amplitude = max_sample - min_sample;
    HWInterface.HR_meter.SampleAmplitude = amplitude;

    if(amplitude < USER_SPO2_VALID_AMPLITUDE)
    {
        quality = (uint8_t)(((uint32_t)amplitude * 20U) / USER_SPO2_VALID_AMPLITUDE);
        user_spo2_measurement.stable_valid_count = 0u;
        user_spo2_measurement.miss_count = 0u;
        HWInterface.HR_meter.SPO2 = 0u;
        HWInterface.HR_meter.SPO2State = HW_SPO2_STATE_SIGNAL_WEAK;
    }
    else
    {
        if (vendor_spo2 > 0u)
        {
            if(amplitude >= USER_SPO2_STRONG_AMPLITUDE)
            {
                quality = 100U;
            }
            else
            {
                quality = (uint8_t)(20U + (((uint32_t)(amplitude - USER_SPO2_VALID_AMPLITUDE) * 80U) /
                    (USER_SPO2_STRONG_AMPLITUDE - USER_SPO2_VALID_AMPLITUDE)));
            }

            if(HWInterface.HR_meter.SPO2 == 0u)
            {
                HWInterface.HR_meter.SPO2 = vendor_spo2;
            }
            else
            {
                HWInterface.HR_meter.SPO2 = (uint8_t)(((uint16_t)HWInterface.HR_meter.SPO2 * 3u + vendor_spo2 + 2u) / 4u);
            }

            user_spo2_measurement.miss_count = 0u;
            if(user_spo2_measurement.stable_valid_count < 0xFFu)
            {
                user_spo2_measurement.stable_valid_count++;
            }

            if(user_spo2_measurement.stable_valid_count >= USER_SPO2_STABLE_VALID_COUNT)
            {
                HWInterface.HR_meter.SPO2State = HW_SPO2_STATE_READY;
            }
            else
            {
                HWInterface.HR_meter.SPO2State = HW_SPO2_STATE_MEASURING;
            }
        }
        else
        {
            if(amplitude >= USER_SPO2_STRONG_AMPLITUDE)
            {
                quality = 100U;
            }
            else
            {
                quality = (uint8_t)(20U + (((uint32_t)(amplitude - USER_SPO2_VALID_AMPLITUDE) * 80U) /
                    (USER_SPO2_STRONG_AMPLITUDE - USER_SPO2_VALID_AMPLITUDE)));
            }

            user_spo2_measurement.stable_valid_count = 0u;
            if(HWInterface.HR_meter.SPO2 > 0u && user_spo2_measurement.miss_count < USER_SPO2_MISS_LIMIT)
            {
                user_spo2_measurement.miss_count++;
                HWInterface.HR_meter.SPO2State = HW_SPO2_STATE_READY;
            }
            else
            {
                user_spo2_measurement.miss_count = 0u;
                HWInterface.HR_meter.SPO2 = 0u;
                HWInterface.HR_meter.SPO2State = HW_SPO2_STATE_UNCALIBRATED;
            }
        }
    }

    HWInterface.HR_meter.SignalQuality = quality;
}

static uint8_t user_HealthPrepareDay(HW_DateTimeTypeDef *nowdatetime)
{
    if (nowdatetime == 0)
    {
        return APPDATA_HEALTH_DAY_UNCHANGED;
    }

    HWInterface.RealTimeClock.GetTimeDate(nowdatetime);
    return AppData_EnsureHealthDay(nowdatetime->Year, nowdatetime->Month, nowdatetime->Date, nowdatetime->WeekDay);
}

static void user_HealthTrackEnvSample(int8_t temp, uint8_t humidity)
{
    HW_DateTimeTypeDef nowdatetime;
    uint8_t state = user_HealthPrepareDay(&nowdatetime);

    AppData_AddHealthEnvSample(nowdatetime.Year, nowdatetime.Month, nowdatetime.Date, nowdatetime.WeekDay, temp, humidity);
    if (state != APPDATA_HEALTH_DAY_UNCHANGED)
    {
        User_RequestDataSave();
    }
}

static void user_HealthTrackHrSample(uint8_t hr)
{
    HW_DateTimeTypeDef nowdatetime;
    uint8_t state;

    if (hr < 40u || hr > 220u)
    {
        return;
    }

    state = user_HealthPrepareDay(&nowdatetime);
    AppData_AddHealthHrSample(nowdatetime.Year, nowdatetime.Month, nowdatetime.Date, nowdatetime.WeekDay, hr);
    if (state != APPDATA_HEALTH_DAY_UNCHANGED)
    {
        User_RequestDataSave();
    }
}


/**
  * @brief  MPU6050 Check the state
  * @param  argument: Not used
  * @retval None
  */
void MPUCheckTask(void *argument)
{
	while(1)
	{
		if(HWInterface.IMU.wrist_is_enabled)
		{
			if(MPU_isHorizontal())
			{
				HWInterface.IMU.wrist_state = WRIST_UP;
			}
			else
			{
				if(WRIST_UP == HWInterface.IMU.wrist_state)
				{
					HWInterface.IMU.wrist_state = WRIST_DOWN;
					if( Page_Get_NowPage()->page_obj == &ui_HomePage ||
						Page_Get_NowPage()->page_obj == &ui_MenuPage ||
						Page_Get_NowPage()->page_obj == &ui_SetPage )
					{
						uint8_t Stopstr = 1;
						osMessageQueuePut(Stop_MessageQueue, &Stopstr, 0, 1);//sleep
					}
				}
				HWInterface.IMU.wrist_state = WRIST_DOWN;
			}
		}
		osDelay(300);
	}
}

/**
  * @brief  HR data renew task
  * @param  argument: Not used
  * @retval None
  */
void HRDataUpdateTask(void *argument)
{
	uint8_t IdleBreakstr=0;
    uint16_t spo2_sample=0;
	uint8_t hr_temp=0;
    uint8_t hr_history_divider = 0u;
	while(1)
	{
        Page_t *now_page = Page_Get_NowPage();

        if(now_page->page_obj != &ui_SPO2Page && user_spo2_measurement.active)
        {
            user_SPO2ResetMeasurement();
        }

		if(now_page->page_obj == &ui_HRPage)
		{
			osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1);
			//sensor wake up
			EM7028_hrs_EnableContinuous();
			//receive the sensor wakeup message, sensor wakeup
			if(!HWInterface.HR_meter.ConnectionError)
			{
				//Hr messure
				vTaskSuspendAll();
				hr_temp = HR_Calculate(EM7028_Get_HRS1(),user_HR_timecount);
				xTaskResumeAll();
					if(HWInterface.HR_meter.HrRate != hr_temp && hr_temp>50 && hr_temp<120)
					{
						HWInterface.HR_meter.HrRate = hr_temp;
					}
	                ui_SportSessionHandleHrSample(HWInterface.HR_meter.HrRate);
                    if (++hr_history_divider >= USER_HEALTH_HR_SAMPLE_DIVIDER)
                    {
                        hr_history_divider = 0u;
                        user_HealthTrackHrSample(HWInterface.HR_meter.HrRate);
                    }
				}
			}
	        else if(ui_SportSessionIsRunning())
	        {
            EM7028_hrs_EnableContinuous();
            if(!HWInterface.HR_meter.ConnectionError)
            {
                vTaskSuspendAll();
                hr_temp = HR_Calculate(EM7028_Get_HRS1(),user_HR_timecount);
                xTaskResumeAll();
	                if(HWInterface.HR_meter.HrRate != hr_temp && hr_temp>50 && hr_temp<120)
	                {
	                    HWInterface.HR_meter.HrRate = hr_temp;
	                }
	                ui_SportSessionHandleHrSample(HWInterface.HR_meter.HrRate);
                    if (++hr_history_divider >= USER_HEALTH_HR_SAMPLE_DIVIDER)
                    {
                        hr_history_divider = 0u;
                        user_HealthTrackHrSample(HWInterface.HR_meter.HrRate);
                    }
	            }
	        }
	        else if(now_page->page_obj == &ui_SPO2Page)
	        {
            osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1);
            EM7028_hrs_EnableContinuous();
            if(!HWInterface.HR_meter.ConnectionError)
            {
                spo2_sample = EM7028_Get_HRS1();
                vTaskSuspendAll();
                hr_temp = HR_Calculate(spo2_sample,user_HR_timecount);
                xTaskResumeAll();
                if(HWInterface.HR_meter.HrRate != hr_temp && hr_temp>50 && hr_temp<120)
                {
                    HWInterface.HR_meter.HrRate = hr_temp;
                }
                user_SPO2UpdateMeasurement(spo2_sample);
            }
            else
            {
	                user_SPO2ResetMeasurement();
	            }
	        }
            else
            {
                hr_history_divider = 0u;
            }
			osDelay(50);
	}
}


/**
  * @brief  Sensor data update task
  * @param  argument: Not used
  * @retval None
  */
void SensorDataUpdateTask(void *argument)
{
	uint8_t value_strbuf[6];
	uint8_t IdleBreakstr=0;
    uint16_t health_sync_counter = 0u;
    uint16_t health_save_counter = 0u;
	while(1)
	{
        if (health_sync_counter == 0u)
        {
            HW_DateTimeTypeDef nowdatetime;
            uint8_t health_day_state = user_HealthPrepareDay(&nowdatetime);

            if (health_day_state == APPDATA_HEALTH_DAY_ROLLED && !HWInterface.IMU.ConnectionError)
            {
                HWInterface.IMU.SetSteps(0);
                HWInterface.IMU.Steps = 0u;
                AppData_UpdateHealthSteps(nowdatetime.Year, nowdatetime.Month, nowdatetime.Date, nowdatetime.WeekDay, 0u);
                User_RequestDataSave();
            }
            else if (!HWInterface.IMU.ConnectionError)
            {
                HWInterface.IMU.Steps = HWInterface.IMU.GetSteps();
                AppData_UpdateHealthSteps(nowdatetime.Year, nowdatetime.Month, nowdatetime.Date, nowdatetime.WeekDay, HWInterface.IMU.Steps);
                if (health_day_state == APPDATA_HEALTH_DAY_INITIALIZED)
                {
                    User_RequestDataSave();
                }
            }
            else if (health_day_state != APPDATA_HEALTH_DAY_UNCHANGED)
            {
                User_RequestDataSave();
            }
        }

		// Update the sens data showed in Home
		uint8_t HomeUpdataStr;
		if(osMessageQueueGet(HomeUpdata_MessageQueue, &HomeUpdataStr, NULL, 0)==osOK)
		{
			//bat
			uint8_t value_strbuf[5];

			HWInterface.Power.power_remain = HWInterface.Power.BatCalculate();
			if(HWInterface.Power.power_remain>0 && HWInterface.Power.power_remain<=100)
			{}
			else
			{HWInterface.Power.power_remain = 0;}

			//steps
			if(!(HWInterface.IMU.ConnectionError))
			{
				HWInterface.IMU.Steps = HWInterface.IMU.GetSteps();
			}

			//temp and humi
			if(!(HWInterface.AHT21.ConnectionError))
			{
				//temp and humi messure
				float humi,temp;
				HWInterface.AHT21.GetHumiTemp(&humi,&temp);
				//check
				if(temp>-10 && temp<50 && humi>0 && humi<100)
				{
					// ui_EnvTempValue = (int8_t)temp;
					// ui_EnvHumiValue = (int8_t)humi;
					HWInterface.AHT21.humidity = (uint8_t)humi;
					HWInterface.AHT21.temperature = (int8_t)temp;
                    user_HealthTrackEnvSample((int8_t)temp, (uint8_t)humi);
				}
			}

			//send data save message queue
			uint8_t Datastr = 3;
			osMessageQueuePut(DataSave_MessageQueue, &Datastr, 0, 1);

		}


		// SPO2 Page
		if(Page_Get_NowPage()->page_obj == &ui_SPO2Page)
		{
			osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1);
			// SPO2 samples are updated by HRDataUpdateTask at 50ms cadence.
		}
		// Env Page
		else if(Page_Get_NowPage()->page_obj == &ui_EnvPage)
		{
			osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1);
			//receive the sensor wakeup message, sensor wakeup
			if(!HWInterface.AHT21.ConnectionError)
			{
				//temp and humi messure
				float humi,temp;
				HWInterface.AHT21.GetHumiTemp(&humi,&temp);
				//check
				if(temp>-10 && temp<50 && humi>0 && humi<100)
				{
					HWInterface.AHT21.temperature = (int8_t)temp;
					HWInterface.AHT21.humidity = (int8_t)humi;
                    user_HealthTrackEnvSample((int8_t)temp, (uint8_t)humi);
				}
			}

		}
        else if(Page_Get_NowPage()->page_obj == &ui_SportPage || ui_SportSessionIsRunning())
        {
            osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1);
            if(!HWInterface.IMU.ConnectionError)
            {
                HWInterface.IMU.Steps = HWInterface.IMU.GetSteps();
            }
            if(!HWInterface.Barometer.ConnectionError)
            {
                HWInterface.Barometer.altitude = (int16_t)Altitude_Calculate();
            }
        }
		// Compass page
		else if(Page_Get_NowPage()->page_obj == &ui_CompassPage)
		{
			osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1);
			//receive the sensor wakeup message, sensor wakeup
			LSM303DLH_Wakeup();
			//SPL_Wakeup();
			//if the sensor is no problem
			if(!HWInterface.Ecompass.ConnectionError)
			{
				//messure
				int16_t Xa,Ya,Za,Xm,Ym,Zm;
				LSM303_ReadAcceleration(&Xa,&Ya,&Za);
				LSM303_ReadMagnetic(&Xm,&Ym,&Zm);
				float temp = Azimuth_Calculate(Xa,Ya,Za,Xm,Ym,Zm)+0;//0 offset
				if(temp<0)
				{temp+=360;}
				//check
				if(temp>=0 && temp<=360)
				{
					HWInterface.Ecompass.direction = (uint16_t)temp;
				}
			}
			//if the sensor is no problem
			if(!HWInterface.Barometer.ConnectionError)
			{
				//messure
				float alti = Altitude_Calculate();
				//check
				if(1)
				{
					HWInterface.Barometer.altitude = (int16_t)alti;
				}
			}
		}

        health_sync_counter++;
        if (health_sync_counter >= USER_HEALTH_STEP_SYNC_LOOPS)
        {
            health_sync_counter = 0u;
        }

        if (AppData_IsHealthDirty())
        {
            health_save_counter++;
            if (health_save_counter >= USER_HEALTH_SAVE_SYNC_LOOPS)
            {
                health_save_counter = 0u;
                User_RequestDataSave();
            }
        }
        else
        {
            health_save_counter = 0u;
        }

		osDelay(500);
	}
}
