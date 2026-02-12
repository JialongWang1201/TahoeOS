#ifndef __USER_SENSUPDATETASK_H__
#define __USER_SENSUPDATETASK_H__

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * Sensor dispatch table
 *
 * Each entry describes one sensor polling job.  SensorDataUpdateTask
 * loops the table at SENSOR_TASK_TICK_MS and calls poll_fn() when the
 * entry's poll_interval_ms has elapsed AND *error_flag == 0.
 *
 * To add a new sensor:
 *   1. Write a static poll function in user_SensUpdateTask.c.
 *   2. Add one sensor_dispatch_entry_t literal to sensor_table[].
 *
 * NOTE: poll_fn must be safe to call from a single FreeRTOS task.
 * For multi-field HWInterface writes, use taskENTER_CRITICAL /
 * taskEXIT_CRITICAL inside poll_fn to ensure consistency.
 *
 * Flow (50 ms tick):
 *
 *   SensorDataUpdateTask
 *   └── for each entry in sensor_table[]
 *         ├── skip if *error_flag != 0
 *         ├── skip if (now - last_tick) < poll_interval_ms
 *         └── call poll_fn(); update last_tick
 * ----------------------------------------------------------------------- */

/** Task loop granularity in ms.  All poll_interval_ms values must be a
 *  multiple of this constant. */
#define SENSOR_TASK_TICK_MS  50U

typedef void (*sensor_poll_fn_t)(void);

typedef struct {
    uint32_t          poll_interval_ms; /**< Minimum ms between calls to poll_fn */
    sensor_poll_fn_t  poll_fn;          /**< Sensor read function                */
    uint8_t          *error_flag;       /**< &HWInterface.xxx.ConnectionError;
                                             NULL = always run regardless of error */
    uint32_t          last_tick;        /**< osKernelGetTickCount() at last poll  */
} sensor_dispatch_entry_t;

/* Health tracking intervals (derived from SENSOR_TASK_TICK_MS) */
#define SENSOR_HEALTH_STEP_SYNC_TICKS   (10000U  / SENSOR_TASK_TICK_MS)  /* 10 s  */
#define SENSOR_HEALTH_SAVE_TICKS        (600000U / SENSOR_TASK_TICK_MS)  /* 10 min */

extern uint32_t user_HR_timecount;

void SensorDataUpdateTask(void *argument);
void HRDataUpdateTask(void *argument);
void MPUCheckTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif

