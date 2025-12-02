#ifndef APPDATA_H
#define APPDATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#define APPDATA_NFC_SLOT_COUNT 3
#define APPDATA_SPORT_RECORD_COUNT 4
#define APPDATA_HEALTH_DAY_COUNT 7
#define APPDATA_NFC_SLOT_NONE 0xFFu

#define APPDATA_HEALTH_DAY_UNCHANGED 0u
#define APPDATA_HEALTH_DAY_INITIALIZED 1u
#define APPDATA_HEALTH_DAY_ROLLED 2u
#define APPDATA_WEATHER_TEXT_MAX 16u
#define APPDATA_NOTIFY_TITLE_MAX 24u
#define APPDATA_NOTIFY_BODY_MAX 96u

typedef enum
{
    APPDATA_NFC_PROFILE_NONE = 0,
    APPDATA_NFC_PROFILE_CAMPUS = 1,
    APPDATA_NFC_PROFILE_DOOR = 2,
    APPDATA_NFC_PROFILE_TRANSIT = 3
} AppDataNfcProfile_t;

typedef struct
{
    uint8_t month;
    uint8_t date;
    uint8_t hour;
    uint8_t minute;
    uint16_t duration_sec;
    uint16_t steps;
    uint8_t avg_hr;
    uint8_t max_hr;
    int16_t altitude_delta;
} AppSportRecord_t;

typedef struct
{
    uint8_t year;
    uint8_t month;
    uint8_t date;
    uint8_t weekday;
    uint16_t steps;
    uint8_t avg_hr;
    uint8_t max_hr;
    uint16_t hr_samples;
    int8_t avg_temp;
    uint8_t avg_humidity;
    uint16_t env_samples;
    uint8_t sport_sessions;
    uint8_t reserved;
    uint16_t sport_minutes;
} AppHealthDaySummary_t;

typedef struct
{
    uint8_t light_time_seconds;
    uint8_t turnoff_time_seconds;
    uint8_t wrist_enabled;
    uint8_t app_sync_enabled;
    uint8_t password_enabled;
    uint8_t pin[4];
    uint8_t nfc_enabled;
    uint8_t active_nfc_slot;
    uint8_t nfc_slot_profile[APPDATA_NFC_SLOT_COUNT];
    uint8_t sport_record_count;
    uint8_t reserved[3];
    AppSportRecord_t sport_records[APPDATA_SPORT_RECORD_COUNT];
    uint8_t health_day_count;
    uint8_t alarm_enabled;
    uint8_t alarm_hour;
    uint8_t alarm_minute;
    AppHealthDaySummary_t health_days[APPDATA_HEALTH_DAY_COUNT];
} AppDataStore_t;

typedef struct
{
    uint8_t weather_valid;
    int8_t weather_temp;
    int8_t weather_low;
    int8_t weather_high;
    char weather_text[APPDATA_WEATHER_TEXT_MAX];
    uint8_t find_watch_pending;
    uint8_t phone_notification_pending;
    char phone_notification_title[APPDATA_NOTIFY_TITLE_MAX];
    char phone_notification_body[APPDATA_NOTIFY_BODY_MAX];
} AppRuntimeState_t;

extern AppDataStore_t g_app_data;
extern AppRuntimeState_t g_app_runtime;

void AppData_ResetDefaults(void);
uint8_t AppData_Load(void);
uint8_t AppData_Save(void);
void AppData_AddSportRecord(const AppSportRecord_t *record);
uint8_t AppData_GetSportRecordCount(void);
const AppSportRecord_t *AppData_GetSportRecord(uint8_t index);
uint8_t AppData_EnsureHealthDay(uint8_t year, uint8_t month, uint8_t date, uint8_t weekday);
void AppData_UpdateHealthSteps(uint8_t year, uint8_t month, uint8_t date, uint8_t weekday, uint16_t steps);
void AppData_AddHealthHrSample(uint8_t year, uint8_t month, uint8_t date, uint8_t weekday, uint8_t hr);
void AppData_AddHealthEnvSample(uint8_t year, uint8_t month, uint8_t date, uint8_t weekday, int8_t temp, uint8_t humidity);
void AppData_AddHealthSportSession(uint8_t year, uint8_t month, uint8_t date, uint8_t weekday, uint16_t duration_sec);
uint8_t AppData_GetHealthDayCount(void);
const AppHealthDaySummary_t *AppData_GetHealthDay(uint8_t index);
uint8_t AppData_IsHealthDirty(void);
uint8_t AppData_IsValidNfcSlot(uint8_t slot_index);
uint8_t AppData_FindFirstNfcSlot(void);
const char *AppData_GetNfcProfileName(uint8_t profile);
const char *AppData_GetNfcSlotName(uint8_t slot_index);
void AppRuntime_Reset(void);
void AppRuntime_SetWeather(int8_t temp, int8_t low, int8_t high, const char *text);
void AppRuntime_ClearWeather(void);
void AppRuntime_RequestFindWatch(void);
uint8_t AppRuntime_ConsumeFindWatchRequest(void);
uint8_t AppRuntime_BuildWeatherSummary(char *buf, size_t buf_size);
void AppRuntime_SetPhoneNotification(const char *title, const char *body);
uint8_t AppRuntime_ConsumePhoneNotification(char *title_buf, size_t title_size, char *body_buf, size_t body_size);

#ifdef __cplusplus
}
#endif

#endif
