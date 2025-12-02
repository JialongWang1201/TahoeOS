#include "../Inc/AppData.h"

#include <stdio.h>
#include <string.h>

#include "DataSave.h"

#define APPDATA_STORE_ADDR 0x30u
#define APPDATA_MAGIC_0 0x4Fu
#define APPDATA_MAGIC_1 0x56u
#define APPDATA_VERSION 0x02u
#define APPDATA_VERSION_LEGACY 0x01u

AppDataStore_t g_app_data;
AppRuntimeState_t g_app_runtime;
static uint8_t g_appdata_health_dirty;

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
} AppDataStoreV1_t;

static uint8_t appdata_checksum(const uint8_t *buf, uint16_t length, uint8_t version)
{
    uint8_t checksum = version;
    uint16_t i = 0;

    for (i = 0; i < length; ++i)
    {
        checksum = (uint8_t)(checksum + buf[i]);
    }

    return checksum;
}

static void appdata_mark_health_dirty(void)
{
    g_appdata_health_dirty = 1u;
}

void AppRuntime_Reset(void)
{
    memset(&g_app_runtime, 0, sizeof(g_app_runtime));
}

static uint8_t appdata_same_health_day(const AppHealthDaySummary_t *day,
                                       uint8_t year,
                                       uint8_t month,
                                       uint8_t date)
{
    if (day == 0)
    {
        return 0u;
    }

    return (uint8_t)(day->year == year && day->month == month && day->date == date);
}

static AppHealthDaySummary_t *appdata_get_health_day_slot(uint8_t year,
                                                          uint8_t month,
                                                          uint8_t date,
                                                          uint8_t weekday,
                                                          uint8_t *state)
{
    uint8_t i = 0;

    if (state != 0)
    {
        *state = APPDATA_HEALTH_DAY_UNCHANGED;
    }

    if (g_app_data.health_day_count > 0u && appdata_same_health_day(&g_app_data.health_days[0], year, month, date))
    {
        return &g_app_data.health_days[0];
    }

    if (g_app_data.health_day_count > 0u)
    {
        for (i = APPDATA_HEALTH_DAY_COUNT - 1u; i > 0u; --i)
        {
            g_app_data.health_days[i] = g_app_data.health_days[i - 1u];
        }
        if (g_app_data.health_day_count < APPDATA_HEALTH_DAY_COUNT)
        {
            g_app_data.health_day_count++;
        }
        if (state != 0)
        {
            *state = APPDATA_HEALTH_DAY_ROLLED;
        }
    }
    else
    {
        g_app_data.health_day_count = 1u;
        if (state != 0)
        {
            *state = APPDATA_HEALTH_DAY_INITIALIZED;
        }
    }

    memset(&g_app_data.health_days[0], 0, sizeof(g_app_data.health_days[0]));
    g_app_data.health_days[0].year = year;
    g_app_data.health_days[0].month = month;
    g_app_data.health_days[0].date = date;
    g_app_data.health_days[0].weekday = weekday;
    appdata_mark_health_dirty();
    return &g_app_data.health_days[0];
}

void AppData_ResetDefaults(void)
{
    memset(&g_app_data, 0, sizeof(g_app_data));
    g_appdata_health_dirty = 0u;
    AppRuntime_Reset();

    g_app_data.light_time_seconds = 10;
    g_app_data.turnoff_time_seconds = 15;
    g_app_data.active_nfc_slot = APPDATA_NFC_SLOT_NONE;
    g_app_data.nfc_slot_profile[0] = APPDATA_NFC_PROFILE_CAMPUS;
    g_app_data.nfc_slot_profile[1] = APPDATA_NFC_PROFILE_DOOR;
    g_app_data.nfc_slot_profile[2] = APPDATA_NFC_PROFILE_TRANSIT;
    g_app_data.pin[0] = 1;
    g_app_data.pin[1] = 2;
    g_app_data.pin[2] = 3;
    g_app_data.pin[3] = 4;
    g_app_data.alarm_enabled = 0u;
    g_app_data.alarm_hour = 7u;
    g_app_data.alarm_minute = 0u;
}

void AppRuntime_SetWeather(int8_t temp, int8_t low, int8_t high, const char *text)
{
    memset(g_app_runtime.weather_text, 0, sizeof(g_app_runtime.weather_text));
    if (text != 0)
    {
        strncpy(g_app_runtime.weather_text, text, APPDATA_WEATHER_TEXT_MAX - 1u);
    }

    g_app_runtime.weather_temp = temp;
    g_app_runtime.weather_low = low;
    g_app_runtime.weather_high = high;
    g_app_runtime.weather_valid = 1u;
}

void AppRuntime_ClearWeather(void)
{
    g_app_runtime.weather_valid = 0u;
    g_app_runtime.weather_temp = 0;
    g_app_runtime.weather_low = 0;
    g_app_runtime.weather_high = 0;
    memset(g_app_runtime.weather_text, 0, sizeof(g_app_runtime.weather_text));
}

void AppRuntime_RequestFindWatch(void)
{
    g_app_runtime.find_watch_pending = 1u;
}

uint8_t AppRuntime_ConsumeFindWatchRequest(void)
{
    uint8_t pending = g_app_runtime.find_watch_pending;
    g_app_runtime.find_watch_pending = 0u;
    return pending;
}

void AppRuntime_SetPhoneNotification(const char *title, const char *body)
{
    memset(g_app_runtime.phone_notification_title, 0, sizeof(g_app_runtime.phone_notification_title));
    memset(g_app_runtime.phone_notification_body, 0, sizeof(g_app_runtime.phone_notification_body));

    if (title != 0 && title[0] != '\0')
    {
        strncpy(g_app_runtime.phone_notification_title, title, APPDATA_NOTIFY_TITLE_MAX - 1u);
    }
    else
    {
        strncpy(g_app_runtime.phone_notification_title, "Notification", APPDATA_NOTIFY_TITLE_MAX - 1u);
    }

    if (body != 0 && body[0] != '\0')
    {
        strncpy(g_app_runtime.phone_notification_body, body, APPDATA_NOTIFY_BODY_MAX - 1u);
    }
    else
    {
        strncpy(g_app_runtime.phone_notification_body, "Phone requested attention", APPDATA_NOTIFY_BODY_MAX - 1u);
    }

    g_app_runtime.phone_notification_pending = 1u;
}

uint8_t AppRuntime_ConsumePhoneNotification(char *title_buf, size_t title_size, char *body_buf, size_t body_size)
{
    if (!g_app_runtime.phone_notification_pending)
    {
        return 0u;
    }

    g_app_runtime.phone_notification_pending = 0u;

    if (title_buf != 0 && title_size > 0u)
    {
        strncpy(title_buf, g_app_runtime.phone_notification_title, title_size - 1u);
        title_buf[title_size - 1u] = '\0';
    }

    if (body_buf != 0 && body_size > 0u)
    {
        strncpy(body_buf, g_app_runtime.phone_notification_body, body_size - 1u);
        body_buf[body_size - 1u] = '\0';
    }

    return 1u;
}

uint8_t AppRuntime_BuildWeatherSummary(char *buf, size_t buf_size)
{
    int written = 0;

    if (buf == 0 || buf_size == 0u)
    {
        return 0u;
    }

    buf[0] = '\0';
    if (!g_app_runtime.weather_valid)
    {
        return 0u;
    }

    if (g_app_runtime.weather_text[0] != '\0')
    {
        written = snprintf(buf, buf_size, "%s %dC", g_app_runtime.weather_text, (int)g_app_runtime.weather_temp);
    }
    else
    {
        written = snprintf(buf, buf_size, "%dC", (int)g_app_runtime.weather_temp);
    }

    if (written < 0)
    {
        buf[0] = '\0';
        return 0u;
    }

    buf[buf_size - 1u] = '\0';
    return 1u;
}

uint8_t AppData_Load(void)
{
    uint8_t raw[sizeof(g_app_data) + 4];
    uint8_t checksum = 0;
    AppDataStoreV1_t legacy_data;

    AppData_ResetDefaults();

    if (SettingGet(raw, APPDATA_STORE_ADDR, sizeof(raw)) != 0)
    {
        return 1;
    }

    if (raw[0] != APPDATA_MAGIC_0 || raw[1] != APPDATA_MAGIC_1)
    {
        return 1;
    }

    if (raw[2] == APPDATA_VERSION)
    {
        checksum = appdata_checksum(&raw[4], (uint16_t)sizeof(g_app_data), APPDATA_VERSION);
        if (raw[3] != checksum)
        {
            return 1;
        }

        memcpy(&g_app_data, &raw[4], sizeof(g_app_data));
        if (g_app_data.alarm_enabled > 1u)
        {
            g_app_data.alarm_enabled = 0u;
        }
        if (g_app_data.alarm_hour > 23u)
        {
            g_app_data.alarm_hour = 7u;
        }
        if (g_app_data.alarm_minute > 59u)
        {
            g_app_data.alarm_minute = 0u;
        }
        return 0;
    }

    if (raw[2] == APPDATA_VERSION_LEGACY)
    {
        checksum = appdata_checksum(&raw[4], (uint16_t)sizeof(legacy_data), APPDATA_VERSION_LEGACY);
        if (raw[3] != checksum)
        {
            return 1;
        }

        memset(&legacy_data, 0, sizeof(legacy_data));
        memcpy(&legacy_data, &raw[4], sizeof(legacy_data));
        memcpy(&g_app_data, &legacy_data, sizeof(legacy_data));
        return 0;
    }

    return 1;
}

uint8_t AppData_Save(void)
{
    uint8_t raw[sizeof(g_app_data) + 4];

    raw[0] = APPDATA_MAGIC_0;
    raw[1] = APPDATA_MAGIC_1;
    raw[2] = APPDATA_VERSION;
    memcpy(&raw[4], &g_app_data, sizeof(g_app_data));
    raw[3] = appdata_checksum(&raw[4], (uint16_t)sizeof(g_app_data), APPDATA_VERSION);

    if (SettingSave(raw, APPDATA_STORE_ADDR, sizeof(raw)) == 0u)
    {
        g_appdata_health_dirty = 0u;
        return 0u;
    }

    return 1u;
}

void AppData_AddSportRecord(const AppSportRecord_t *record)
{
    uint8_t i = 0;

    if (record == 0)
    {
        return;
    }

    for (i = APPDATA_SPORT_RECORD_COUNT - 1; i > 0; --i)
    {
        g_app_data.sport_records[i] = g_app_data.sport_records[i - 1];
    }

    g_app_data.sport_records[0] = *record;
    if (g_app_data.sport_record_count < APPDATA_SPORT_RECORD_COUNT)
    {
        g_app_data.sport_record_count++;
    }
}

uint8_t AppData_GetSportRecordCount(void)
{
    return g_app_data.sport_record_count;
}

const AppSportRecord_t *AppData_GetSportRecord(uint8_t index)
{
    if (index >= g_app_data.sport_record_count || index >= APPDATA_SPORT_RECORD_COUNT)
    {
        return 0;
    }

    return &g_app_data.sport_records[index];
}

uint8_t AppData_EnsureHealthDay(uint8_t year, uint8_t month, uint8_t date, uint8_t weekday)
{
    uint8_t state = APPDATA_HEALTH_DAY_UNCHANGED;

    (void)appdata_get_health_day_slot(year, month, date, weekday, &state);
    return state;
}

void AppData_UpdateHealthSteps(uint8_t year, uint8_t month, uint8_t date, uint8_t weekday, uint16_t steps)
{
    AppHealthDaySummary_t *day = appdata_get_health_day_slot(year, month, date, weekday, 0);

    if (day == 0 || day->steps == steps)
    {
        return;
    }

    day->steps = steps;
    appdata_mark_health_dirty();
}

void AppData_AddHealthHrSample(uint8_t year, uint8_t month, uint8_t date, uint8_t weekday, uint8_t hr)
{
    AppHealthDaySummary_t *day = 0;
    uint32_t weighted_sum = 0;

    if (hr < 40u || hr > 220u)
    {
        return;
    }

    day = appdata_get_health_day_slot(year, month, date, weekday, 0);
    if (day == 0)
    {
        return;
    }

    if (day->hr_samples < 0xFFFFu)
    {
        weighted_sum = (uint32_t)day->avg_hr * day->hr_samples + hr;
        day->hr_samples++;
        day->avg_hr = (uint8_t)(weighted_sum / day->hr_samples);
    }
    else
    {
        day->avg_hr = (uint8_t)(((uint16_t)day->avg_hr * 3u + hr) / 4u);
    }

    if (hr > day->max_hr)
    {
        day->max_hr = hr;
    }

    appdata_mark_health_dirty();
}

void AppData_AddHealthEnvSample(uint8_t year, uint8_t month, uint8_t date, uint8_t weekday, int8_t temp, uint8_t humidity)
{
    AppHealthDaySummary_t *day = 0;
    int32_t temp_weighted = 0;
    uint32_t humi_weighted = 0;

    day = appdata_get_health_day_slot(year, month, date, weekday, 0);
    if (day == 0)
    {
        return;
    }

    if (day->env_samples < 0xFFFFu)
    {
        temp_weighted = (int32_t)day->avg_temp * day->env_samples + temp;
        humi_weighted = (uint32_t)day->avg_humidity * day->env_samples + humidity;
        day->env_samples++;
        day->avg_temp = (int8_t)(temp_weighted / day->env_samples);
        day->avg_humidity = (uint8_t)(humi_weighted / day->env_samples);
    }
    else
    {
        day->avg_temp = (int8_t)(((int16_t)day->avg_temp * 3 + temp) / 4);
        day->avg_humidity = (uint8_t)(((uint16_t)day->avg_humidity * 3u + humidity) / 4u);
    }

    appdata_mark_health_dirty();
}

void AppData_AddHealthSportSession(uint8_t year, uint8_t month, uint8_t date, uint8_t weekday, uint16_t duration_sec)
{
    AppHealthDaySummary_t *day = appdata_get_health_day_slot(year, month, date, weekday, 0);
    uint16_t duration_min = (uint16_t)((duration_sec + 59u) / 60u);

    if (day == 0)
    {
        return;
    }

    if (day->sport_sessions < 0xFFu)
    {
        day->sport_sessions++;
    }

    if ((uint32_t)day->sport_minutes + duration_min > 0xFFFFu)
    {
        day->sport_minutes = 0xFFFFu;
    }
    else
    {
        day->sport_minutes = (uint16_t)(day->sport_minutes + duration_min);
    }

    appdata_mark_health_dirty();
}

uint8_t AppData_GetHealthDayCount(void)
{
    return g_app_data.health_day_count;
}

const AppHealthDaySummary_t *AppData_GetHealthDay(uint8_t index)
{
    if (index >= g_app_data.health_day_count || index >= APPDATA_HEALTH_DAY_COUNT)
    {
        return 0;
    }

    return &g_app_data.health_days[index];
}

uint8_t AppData_IsHealthDirty(void)
{
    return g_appdata_health_dirty;
}

uint8_t AppData_IsValidNfcSlot(uint8_t slot_index)
{
    if (slot_index >= APPDATA_NFC_SLOT_COUNT)
    {
        return 0;
    }

    return (uint8_t)(g_app_data.nfc_slot_profile[slot_index] != APPDATA_NFC_PROFILE_NONE);
}

uint8_t AppData_FindFirstNfcSlot(void)
{
    uint8_t i = 0;

    for (i = 0; i < APPDATA_NFC_SLOT_COUNT; ++i)
    {
        if (AppData_IsValidNfcSlot(i))
        {
            return i;
        }
    }

    return APPDATA_NFC_SLOT_NONE;
}

const char *AppData_GetNfcProfileName(uint8_t profile)
{
    switch (profile)
    {
        case APPDATA_NFC_PROFILE_CAMPUS:
            return "CAMPUS";
        case APPDATA_NFC_PROFILE_DOOR:
            return "DOOR";
        case APPDATA_NFC_PROFILE_TRANSIT:
            return "TRANSIT";
        default:
            return "EMPTY";
    }
}

const char *AppData_GetNfcSlotName(uint8_t slot_index)
{
    if (!AppData_IsValidNfcSlot(slot_index))
    {
        return "EMPTY";
    }

    return AppData_GetNfcProfileName(g_app_data.nfc_slot_profile[slot_index]);
}
