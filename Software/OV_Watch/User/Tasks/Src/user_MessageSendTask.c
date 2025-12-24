/* Private includes -----------------------------------------------------------*/
// includes
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#include "main.h"
#include "stm32f4xx_it.h"
#include "rtc.h"
#include "usart.h"

#include "user_TasksInit.h"
#include "user_MessageSendTask.h"

#include "ui.h"
#include "ui_EnvPage.h"
#include "ui_HRPage.h"
#include "ui_SPO2Page.h"
#include "ui_HomePage.h"
#include "ui_DateTimeSetPage.h"
#include "ui_LockPage.h"
#include "ui_OffTimePage.h"

#include "AppData.h"
#include "HWDataAccess.h"
#include "version.h"

/* Private define ------------------------------------------------------------*/
#define BLE_PROTO_ERR_OK 0U
#define BLE_PROTO_ERR_FORMAT 1U
#define BLE_PROTO_ERR_LENGTH 2U
#define BLE_PROTO_ERR_CRC 3U
#define BLE_PROTO_ERR_CMD 4U
#define BLE_PROTO_ERR_PAYLOAD 5U

#define BLE_PROTO_CMD_MAX 24U
#define BLE_PROTO_PAYLOAD_MAX 192U

/* Private variables ---------------------------------------------------------*/
typedef struct
{
  RTC_DateTypeDef nowdate;
  RTC_TimeTypeDef nowtime;
  int8_t humi;
  int8_t temp;
  uint8_t HR;
  uint8_t SPO2;
  uint16_t stepNum;
} BLEMessage_t;

typedef struct
{
  uint8_t seq;
  char cmd[BLE_PROTO_CMD_MAX];
  char payload[BLE_PROTO_PAYLOAD_MAX];
} BleProtoReq_t;

static BLEMessage_t BLEMessage;

static struct
{
  RTC_DateTypeDef nowdate;
  RTC_TimeTypeDef nowtime;
} TimeSetMessage;

/* Private function prototypes -----------------------------------------------*/
static const char * user_SPO2StateToString(uint8_t state)
{
	switch(state)
	{
	case HW_SPO2_STATE_READY:
		return "ESTIMATED";
	case HW_SPO2_STATE_MEASURING:
		return "MEASURING";
	case HW_SPO2_STATE_SIGNAL_WEAK:
		return "SIGNAL_WEAK";
	case HW_SPO2_STATE_UNCALIBRATED:
		return "PPG_ONLY";
	default:
		return "IDLE";
	}
}

void StrCMD_Get(uint8_t *str, uint8_t *cmd)
{
  uint8_t i = 0;
  if (str == NULL || cmd == NULL)
  {
    return;
  }
  while (str[i] != '\0' && str[i] != '=')
  {
    cmd[i] = str[i];
    i++;
  }
  cmd[i] = '\0';
}

// set time // OV+ST=20230629125555
uint8_t TimeFormat_Get(uint8_t *str)
{
  TimeSetMessage.nowdate.Year = (str[8] - '0') * 10 + str[9] - '0';
  TimeSetMessage.nowdate.Month = (str[10] - '0') * 10 + str[11] - '0';
  TimeSetMessage.nowdate.Date = (str[12] - '0') * 10 + str[13] - '0';
  TimeSetMessage.nowtime.Hours = (str[14] - '0') * 10 + str[15] - '0';
  TimeSetMessage.nowtime.Minutes = (str[16] - '0') * 10 + str[17] - '0';
  TimeSetMessage.nowtime.Seconds = (str[18] - '0') * 10 + str[19] - '0';
  if (TimeSetMessage.nowdate.Year > 0 && TimeSetMessage.nowdate.Year < 99 && TimeSetMessage.nowdate.Month > 0 &&
      TimeSetMessage.nowdate.Month <= 12 && TimeSetMessage.nowdate.Date > 0 && TimeSetMessage.nowdate.Date <= 31 &&
      TimeSetMessage.nowtime.Hours >= 0 && TimeSetMessage.nowtime.Hours <= 23 &&
      TimeSetMessage.nowtime.Minutes >= 0 && TimeSetMessage.nowtime.Minutes <= 59 &&
      TimeSetMessage.nowtime.Seconds >= 0 && TimeSetMessage.nowtime.Seconds <= 59)
  {
    RTC_SetDate(TimeSetMessage.nowdate.Year, TimeSetMessage.nowdate.Month, TimeSetMessage.nowdate.Date);
    RTC_SetTime(TimeSetMessage.nowtime.Hours, TimeSetMessage.nowtime.Minutes, TimeSetMessage.nowtime.Seconds);
    if (g_app_data.alarm_enabled)
    {
      RTC_SetDailyAlarm(g_app_data.alarm_hour, g_app_data.alarm_minute);
    }
    printf("TIMESETOK\r\n");
    return 1;
  }
  return 0;
}

static char *BleProto_Trim(char *text)
{
  char *start = text;
  char *end;
  if (start == NULL)
  {
    return NULL;
  }
  while (*start == ' ' || *start == '\r' || *start == '\n' || *start == '\t')
  {
    start++;
  }
  end = start + strlen(start);
  while (end > start && (end[-1] == ' ' || end[-1] == '\r' || end[-1] == '\n' || end[-1] == '\t'))
  {
    end--;
  }
  *end = '\0';
  return start;
}

static uint8_t BleProto_ParseU32(const char *text, uint32_t *out)
{
  char *endptr;
  unsigned long value;
  if (text == NULL || out == NULL || text[0] == '\0')
  {
    return 0;
  }
  value = strtoul(text, &endptr, 10);
  if (*endptr != '\0')
  {
    return 0;
  }
  *out = (uint32_t)value;
  return 1;
}

static uint8_t BleProto_ParseI32(const char *text, int32_t *out)
{
  char *endptr;
  long value;
  if (text == NULL || out == NULL || text[0] == '\0')
  {
    return 0;
  }
  value = strtol(text, &endptr, 10);
  if (*endptr != '\0')
  {
    return 0;
  }
  *out = (int32_t)value;
  return 1;
}

static uint8_t BleProto_ParseTimeHm(const char *text, uint8_t *hours, uint8_t *minutes)
{
  uint32_t hours_u32 = 0;
  uint32_t minutes_u32 = 0;
  char hour_buf[3];
  char min_buf[3];

  if (text == NULL || hours == NULL || minutes == NULL)
  {
    return 0;
  }

  if (strlen(text) == 4U)
  {
    memcpy(hour_buf, text, 2U);
    memcpy(min_buf, text + 2U, 2U);
    hour_buf[2] = '\0';
    min_buf[2] = '\0';
  }
  else if (strlen(text) == 5U && text[2] == ':')
  {
    memcpy(hour_buf, text, 2U);
    memcpy(min_buf, text + 3U, 2U);
    hour_buf[2] = '\0';
    min_buf[2] = '\0';
  }
  else
  {
    return 0;
  }

  if (!BleProto_ParseU32(hour_buf, &hours_u32) || !BleProto_ParseU32(min_buf, &minutes_u32) || hours_u32 > 23U ||
      minutes_u32 > 59U)
  {
    return 0;
  }

  *hours = (uint8_t)hours_u32;
  *minutes = (uint8_t)minutes_u32;
  return 1;
}

static uint8_t BleProto_ParseHex16(const char *text, uint16_t *out)
{
  char *endptr;
  unsigned long value;
  if (text == NULL || out == NULL || text[0] == '\0')
  {
    return 0;
  }
  value = strtoul(text, &endptr, 16);
  if (*endptr != '\0' || value > 0xFFFFUL)
  {
    return 0;
  }
  *out = (uint16_t)value;
  return 1;
}

static uint8_t BleProto_ParsePin4(const char *text, uint8_t pin_out[4])
{
  uint8_t idx;

  if (text == NULL || pin_out == NULL || strlen(text) != 4U)
  {
    return 0;
  }

  for (idx = 0; idx < 4U; idx++)
  {
    if (text[idx] < '0' || text[idx] > '9')
    {
      return 0;
    }

    pin_out[idx] = (uint8_t)(text[idx] - '0');
  }

  return 1;
}

static uint8_t BleProto_ParseDateTime14(const char *text, RTC_DateTypeDef *out_date, RTC_TimeTypeDef *out_time)
{
  uint16_t year;
  uint8_t month;
  uint8_t date;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
  uint8_t idx;

  if (text == NULL || out_date == NULL || out_time == NULL || strlen(text) != 14U)
  {
    return 0;
  }

  for (idx = 0; idx < 14U; idx++)
  {
    if (text[idx] < '0' || text[idx] > '9')
    {
      return 0;
    }
  }

  year = (uint16_t)((text[0] - '0') * 1000U + (text[1] - '0') * 100U + (text[2] - '0') * 10U + (text[3] - '0'));
  month = (uint8_t)((text[4] - '0') * 10U + (text[5] - '0'));
  date = (uint8_t)((text[6] - '0') * 10U + (text[7] - '0'));
  hours = (uint8_t)((text[8] - '0') * 10U + (text[9] - '0'));
  minutes = (uint8_t)((text[10] - '0') * 10U + (text[11] - '0'));
  seconds = (uint8_t)((text[12] - '0') * 10U + (text[13] - '0'));

  if (year < 2000U || year > 2099U || month < 1U || month > 12U || date < 1U || date > 31U || hours > 23U ||
      minutes > 59U || seconds > 59U)
  {
    return 0;
  }

  out_date->Year = (uint8_t)(year - 2000U);
  out_date->Month = month;
  out_date->Date = date;
  out_time->Hours = hours;
  out_time->Minutes = minutes;
  out_time->Seconds = seconds;
  return 1;
}

static uint16_t BleProto_Crc16Ccitt(const uint8_t *data, uint16_t len)
{
  uint16_t crc = 0xFFFFU;
  uint16_t i;
  uint8_t bit;
  for (i = 0; i < len; i++)
  {
    crc ^= (uint16_t)data[i] << 8;
    for (bit = 0; bit < 8; bit++)
    {
      if (crc & 0x8000U)
      {
        crc = (crc << 1) ^ 0x1021U;
      }
      else
      {
        crc <<= 1;
      }
    }
  }
  return crc;
}

static void BleProto_SendFrame(uint8_t seq, uint8_t ack, uint8_t code, const char *payload)
{
  char body[320];
  char frame[380];
  const char *ack_text = ack ? "ACK" : "NACK";
  uint16_t body_len;
  uint16_t crc;
  int n;
  if (payload == NULL)
  {
    payload = "";
  }

  n = snprintf(body, sizeof(body), "%u|%s|%u|%s", (unsigned int)seq, ack_text, (unsigned int)code, payload);
  if (n < 0)
  {
    return;
  }
  body[sizeof(body) - 1] = '\0';
  body_len = (uint16_t)strlen(body);
  crc = BleProto_Crc16Ccitt((const uint8_t *)body, body_len);

  n = snprintf(frame, sizeof(frame), "#%03u|%s|%04X\r\n", (unsigned int)body_len, body, (unsigned int)crc);
  if (n < 0)
  {
    return;
  }
  frame[sizeof(frame) - 1] = '\0';
  HAL_UART_Transmit(&huart1, (uint8_t *)frame, (uint16_t)strlen(frame), 0xFFFFU);
}

static uint8_t BleProto_ParseFrame(char *frame_text, BleProtoReq_t *req, uint8_t *error_code, uint8_t *error_seq)
{
  char *len_sep;
  char *crc_sep;
  char *body;
  char *seq_sep;
  char *cmd_sep;
  char *seq_text;
  char *cmd_text;
  char *payload_text;
  uint32_t frame_len_u32 = 0;
  uint32_t seq_u32 = 0;
  uint16_t frame_crc = 0;
  uint16_t calc_crc = 0;
  uint8_t seq_found = 0;

  *error_code = BLE_PROTO_ERR_FORMAT;
  *error_seq = 0;
  if (frame_text == NULL || req == NULL)
  {
    return 0;
  }
  if (frame_text[0] != '#')
  {
    return 0;
  }

  len_sep = strchr(frame_text, '|');
  if (len_sep == NULL)
  {
    return 0;
  }
  *len_sep = '\0';
  if (!BleProto_ParseU32(frame_text + 1, &frame_len_u32) || frame_len_u32 > (HARDINT_RX_BUF_SIZE - 1U))
  {
    return 0;
  }

  body = len_sep + 1;
  crc_sep = strrchr(body, '|');
  if (crc_sep == NULL)
  {
    return 0;
  }
  *crc_sep = '\0';
  if (!BleProto_ParseHex16(crc_sep + 1, &frame_crc))
  {
    return 0;
  }

  seq_sep = strchr(body, '|');
  if (seq_sep != NULL)
  {
    char seq_buf[8];
    uint16_t seq_len = (uint16_t)(seq_sep - body);
    if (seq_len < sizeof(seq_buf))
    {
      memcpy(seq_buf, body, seq_len);
      seq_buf[seq_len] = '\0';
      if (BleProto_ParseU32(seq_buf, &seq_u32) && seq_u32 <= 255U)
      {
        *error_seq = (uint8_t)seq_u32;
        seq_found = 1;
      }
    }
  }

  if (frame_len_u32 != strlen(body))
  {
    *error_code = BLE_PROTO_ERR_LENGTH;
    return 0;
  }

  calc_crc = BleProto_Crc16Ccitt((const uint8_t *)body, (uint16_t)strlen(body));
  if (calc_crc != frame_crc)
  {
    *error_code = BLE_PROTO_ERR_CRC;
    return 0;
  }

  seq_sep = strchr(body, '|');
  if (seq_sep == NULL)
  {
    return 0;
  }
  cmd_sep = strchr(seq_sep + 1, '|');
  if (cmd_sep == NULL)
  {
    return 0;
  }

  *seq_sep = '\0';
  *cmd_sep = '\0';
  seq_text = body;
  cmd_text = seq_sep + 1;
  payload_text = cmd_sep + 1;

  if (!BleProto_ParseU32(seq_text, &seq_u32) || seq_u32 > 255U)
  {
    return 0;
  }

  if (strlen(cmd_text) >= BLE_PROTO_CMD_MAX || strlen(payload_text) >= BLE_PROTO_PAYLOAD_MAX)
  {
    *error_code = BLE_PROTO_ERR_LENGTH;
    if (!seq_found)
    {
      *error_seq = (uint8_t)seq_u32;
    }
    return 0;
  }

  req->seq = (uint8_t)seq_u32;
  strcpy(req->cmd, cmd_text);
  strcpy(req->payload, payload_text);
  *error_seq = req->seq;
  *error_code = BLE_PROTO_ERR_OK;
  return 1;
}

static uint8_t BleProto_GetBleEnabled(void)
{
  return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == GPIO_PIN_SET ? 1U : 0U;
}

static void BleProto_BuildAlarmPayload(char *payload, uint16_t payload_size)
{
  snprintf(payload, payload_size, "enabled=%u,time=%02u%02u", (unsigned int)g_app_data.alarm_enabled,
           (unsigned int)g_app_data.alarm_hour, (unsigned int)g_app_data.alarm_minute);
}

static void BleProto_BuildLockPayload(char *payload, uint16_t payload_size)
{
  snprintf(payload, payload_size, "enabled=%u,pin_set=1", (unsigned int)g_app_data.password_enabled);
}

static void BleProto_BuildNfcPayload(char *payload, uint16_t payload_size)
{
  uint8_t slot = g_app_data.active_nfc_slot;

  if (!AppData_IsValidNfcSlot(slot))
  {
    slot = APPDATA_NFC_SLOT_NONE;
  }

  snprintf(payload, payload_size, "enabled=%u,slot=%u,profile=%s", (unsigned int)g_app_data.nfc_enabled,
           (unsigned int)slot, AppData_GetNfcSlotName(slot));
}

static uint8_t BleProto_BuildHistoryPayload(char *payload, uint16_t payload_size)
{
  uint8_t count = AppData_GetHealthDayCount();
  uint8_t i;
  uint16_t offset = 0U;

  if (payload == NULL || payload_size == 0U)
  {
    return 0;
  }

  payload[0] = '\0';
  for (i = 0; i < count; i++)
  {
    const AppHealthDaySummary_t *day = AppData_GetHealthDay(i);
    int written;
    if (day == NULL)
    {
      continue;
    }

    if (i == 0U)
    {
      written = snprintf(payload + offset, payload_size - offset, "%u;%02u%02u,%u,%u,%d,%u,%u,%u",
                         (unsigned int)count, (unsigned int)day->month, (unsigned int)day->date,
                         (unsigned int)day->steps, (unsigned int)day->avg_hr, (int)day->avg_temp,
                         (unsigned int)day->avg_humidity, (unsigned int)day->sport_sessions,
                         (unsigned int)day->sport_minutes);
    }
    else
    {
      written = snprintf(payload + offset, payload_size - offset, ";%02u%02u,%u,%u,%d,%u,%u,%u",
                         (unsigned int)day->month, (unsigned int)day->date, (unsigned int)day->steps,
                         (unsigned int)day->avg_hr, (int)day->avg_temp, (unsigned int)day->avg_humidity,
                         (unsigned int)day->sport_sessions, (unsigned int)day->sport_minutes);
    }
    if (written < 0 || (uint16_t)written >= (payload_size - offset))
    {
      return 0;
    }
    offset = (uint16_t)(offset + written);
  }

  if (count == 0U)
  {
    snprintf(payload, payload_size, "0");
  }
  return 1;
}

static uint8_t BleProto_BuildSportRecordPayload(char *payload, uint16_t payload_size)
{
  uint8_t count = AppData_GetSportRecordCount();
  uint8_t i;
  uint16_t offset = 0U;

  if (payload == NULL || payload_size == 0U)
  {
    return 0;
  }

  payload[0] = '\0';
  for (i = 0; i < count; i++)
  {
    const AppSportRecord_t *record = AppData_GetSportRecord(i);
    int written;
    if (record == NULL)
    {
      continue;
    }

    if (i == 0U)
    {
      written = snprintf(payload + offset, payload_size - offset, "%u;%02u%02u%02u%02u,%u,%u,%u,%u,%d",
                         (unsigned int)count, (unsigned int)record->month, (unsigned int)record->date,
                         (unsigned int)record->hour, (unsigned int)record->minute,
                         (unsigned int)record->duration_sec, (unsigned int)record->steps,
                         (unsigned int)record->avg_hr, (unsigned int)record->max_hr,
                         (int)record->altitude_delta);
    }
    else
    {
      written = snprintf(payload + offset, payload_size - offset, ";%02u%02u%02u%02u,%u,%u,%u,%u,%d",
                         (unsigned int)record->month, (unsigned int)record->date,
                         (unsigned int)record->hour, (unsigned int)record->minute,
                         (unsigned int)record->duration_sec, (unsigned int)record->steps,
                         (unsigned int)record->avg_hr, (unsigned int)record->max_hr,
                         (int)record->altitude_delta);
    }
    if (written < 0 || (uint16_t)written >= (payload_size - offset))
    {
      return 0;
    }
    offset = (uint16_t)(offset + written);
  }

  if (count == 0U)
  {
    snprintf(payload, payload_size, "0");
  }
  return 1;
}

static void BleProto_BuildWeatherPayload(char *payload, uint16_t payload_size)
{
  if (payload == NULL || payload_size == 0U)
  {
    return;
  }

  if (!g_app_runtime.weather_valid)
  {
    snprintf(payload, payload_size, "valid=0");
    return;
  }

  snprintf(payload, payload_size, "valid=1,temp=%d,low=%d,high=%d,text=%s", (int)g_app_runtime.weather_temp,
           (int)g_app_runtime.weather_low, (int)g_app_runtime.weather_high, g_app_runtime.weather_text);
}

static void BleProto_BuildStatusPayload(char *payload, uint16_t payload_size)
{
  RTC_DateTypeDef nowdate;
  RTC_TimeTypeDef nowtime;
  HAL_RTC_GetTime(&hrtc, &nowtime, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &nowdate, RTC_FORMAT_BIN);
  snprintf(payload, payload_size,
           "ver=%u.%u.%u,bat=%u,chg=%u,ble=%u,wrist=%u,app=%u,lt=%u,tt=%u,li=%u,st=%u,hr=%u,o2=%u,tp=%d,hm=%d,t=%02u%02u%02u,d=%02u%02u,al=%u,ah=%u,am=%u,wv=%u,wt=%d,wl=%d,wh=%d,ws=%s",
           (unsigned int)watch_version_major(), (unsigned int)watch_version_minor(),
           (unsigned int)watch_version_patch(), (unsigned int)HWInterface.Power.power_remain,
           (unsigned int)ChargeCheck(), (unsigned int)BleProto_GetBleEnabled(),
           (unsigned int)HWInterface.IMU.wrist_is_enabled, (unsigned int)ui_APPSy_EN,
           (unsigned int)ui_LTimeValue, (unsigned int)ui_TTimeValue, (unsigned int)ui_LightSliderValue,
           (unsigned int)HWInterface.IMU.Steps, (unsigned int)HWInterface.HR_meter.HrRate,
           (unsigned int)HWInterface.HR_meter.SPO2, (int)HWInterface.AHT21.temperature,
           (int)HWInterface.AHT21.humidity, (unsigned int)nowtime.Hours, (unsigned int)nowtime.Minutes,
           (unsigned int)nowtime.Seconds, (unsigned int)nowdate.Month, (unsigned int)nowdate.Date,
           (unsigned int)g_app_data.alarm_enabled, (unsigned int)g_app_data.alarm_hour,
           (unsigned int)g_app_data.alarm_minute, (unsigned int)g_app_runtime.weather_valid,
           (int)g_app_runtime.weather_temp, (int)g_app_runtime.weather_low, (int)g_app_runtime.weather_high,
           g_app_runtime.weather_text);
}

static uint8_t BleProto_HandleSetCfg(const char *payload, char *resp_payload, uint16_t resp_size)
{
  char payload_buf[BLE_PROTO_PAYLOAD_MAX];
  char *token;
  uint8_t has_update = 0;
  uint8_t need_save = 0;
  uint32_t value_u32 = 0;
  uint8_t new_ltime = ui_LTimeValue;
  uint8_t new_ttime = ui_TTimeValue;

  if (payload == NULL || payload[0] == '\0' || strlen(payload) >= sizeof(payload_buf))
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }

  strcpy(payload_buf, payload);
  token = strtok(payload_buf, ",;");
  while (token != NULL)
  {
    char *eq = strchr(token, '=');
    char *key;
    char *value;
    if (eq == NULL)
    {
      return BLE_PROTO_ERR_PAYLOAD;
    }
    *eq = '\0';
    key = BleProto_Trim(token);
    value = BleProto_Trim(eq + 1);
    if (key == NULL || value == NULL || key[0] == '\0' || value[0] == '\0')
    {
      return BLE_PROTO_ERR_PAYLOAD;
    }
    if (!strcmp(key, "ble"))
    {
      if (!BleProto_ParseU32(value, &value_u32) || value_u32 > 1U)
      {
        return BLE_PROTO_ERR_PAYLOAD;
      }
      if (value_u32 == 1U)
      {
        HWInterface.BLE.Enable();
      }
      else
      {
        HWInterface.BLE.Disable();
      }
      has_update = 1;
    }
    else if (!strcmp(key, "wrist"))
    {
      if (!BleProto_ParseU32(value, &value_u32) || value_u32 > 1U)
      {
        return BLE_PROTO_ERR_PAYLOAD;
      }
      if (value_u32 == 1U)
      {
        HWInterface.IMU.WristEnable();
      }
      else
      {
        HWInterface.IMU.WristDisable();
      }
      has_update = 1;
      need_save = 1;
    }
    else if (!strcmp(key, "app"))
    {
      if (!BleProto_ParseU32(value, &value_u32) || value_u32 > 1U)
      {
        return BLE_PROTO_ERR_PAYLOAD;
      }
      ui_APPSy_EN = (uint8_t)value_u32;
      has_update = 1;
      need_save = 1;
    }
    else if (!strcmp(key, "light"))
    {
      if (!BleProto_ParseU32(value, &value_u32) || value_u32 > 100U)
      {
        return BLE_PROTO_ERR_PAYLOAD;
      }
      ui_LightSliderValue = (uint8_t)value_u32;
      HWInterface.LCD.SetLight(ui_LightSliderValue);
      has_update = 1;
    }
    else if (!strcmp(key, "ltoff"))
    {
      if (!BleProto_ParseU32(value, &value_u32) || value_u32 < 1U || value_u32 > 59U)
      {
        return BLE_PROTO_ERR_PAYLOAD;
      }
      new_ltime = (uint8_t)value_u32;
      has_update = 1;
    }
    else if (!strcmp(key, "ttoff"))
    {
      if (!BleProto_ParseU32(value, &value_u32) || value_u32 < 2U || value_u32 > 60U)
      {
        return BLE_PROTO_ERR_PAYLOAD;
      }
      new_ttime = (uint8_t)value_u32;
      has_update = 1;
    }
    else
    {
      return BLE_PROTO_ERR_PAYLOAD;
    }
    token = strtok(NULL, ",;");
  }

  if (!has_update || new_ltime >= new_ttime)
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }

  ui_LTimeValue = new_ltime;
  ui_TTimeValue = new_ttime;

  if (need_save)
  {
    uint8_t save_msg = 1;
    osMessageQueuePut(DataSave_MessageQueue, &save_msg, 0, 1);
  }

  snprintf(resp_payload, resp_size, "ble=%u,wrist=%u,app=%u,lt=%u,tt=%u,li=%u", (unsigned int)BleProto_GetBleEnabled(),
           (unsigned int)HWInterface.IMU.wrist_is_enabled, (unsigned int)ui_APPSy_EN, (unsigned int)ui_LTimeValue,
           (unsigned int)ui_TTimeValue, (unsigned int)ui_LightSliderValue);
  return BLE_PROTO_ERR_OK;
}

static uint8_t BleProto_HandleSetTime(const char *payload, char *resp_payload, uint16_t resp_size)
{
  RTC_DateTypeDef setdate = {0};
  RTC_TimeTypeDef settime = {0};
  const char *datetime_text = payload;

  if (resp_payload != NULL && resp_size > 0U)
  {
    resp_payload[0] = '\0';
  }

  if (payload == NULL || resp_payload == NULL || resp_size == 0U)
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }

  if (!ui_APPSy_EN)
  {
    snprintf(resp_payload, resp_size, "app_sync_disabled");
    return BLE_PROTO_ERR_PAYLOAD;
  }

  if (!strncmp(payload, "datetime=", 9U))
  {
    datetime_text = payload + 9U;
  }

  if (!BleProto_ParseDateTime14(datetime_text, &setdate, &settime))
  {
    snprintf(resp_payload, resp_size, "bad_datetime");
    return BLE_PROTO_ERR_PAYLOAD;
  }

  RTC_SetDate(setdate.Year, setdate.Month, setdate.Date);
  RTC_SetTime(settime.Hours, settime.Minutes, settime.Seconds);
  if (g_app_data.alarm_enabled)
  {
    RTC_SetDailyAlarm(g_app_data.alarm_hour, g_app_data.alarm_minute);
  }

  snprintf(resp_payload, resp_size, "datetime=%04u%02u%02u%02u%02u%02u", (unsigned int)(2000U + setdate.Year),
           (unsigned int)setdate.Month, (unsigned int)setdate.Date, (unsigned int)settime.Hours,
           (unsigned int)settime.Minutes, (unsigned int)settime.Seconds);
  return BLE_PROTO_ERR_OK;
}

static uint8_t BleProto_HandleGetAlarm(char *resp_payload, uint16_t resp_size)
{
  if (resp_payload == NULL || resp_size == 0U)
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }

  BleProto_BuildAlarmPayload(resp_payload, resp_size);
  return BLE_PROTO_ERR_OK;
}

static uint8_t BleProto_HandleSetAlarm(const char *payload, char *resp_payload, uint16_t resp_size)
{
  char payload_buf[BLE_PROTO_PAYLOAD_MAX];
  char *token;
  uint32_t value_u32 = 0;
  uint8_t enabled = g_app_data.alarm_enabled;
  uint8_t hour = g_app_data.alarm_hour;
  uint8_t minute = g_app_data.alarm_minute;
  uint8_t has_update = 0U;

  if (resp_payload != NULL && resp_size > 0U)
  {
    resp_payload[0] = '\0';
  }

  if (payload == NULL || resp_payload == NULL || resp_size == 0U || payload[0] == '\0' ||
      strlen(payload) >= sizeof(payload_buf))
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }

  if (!ui_APPSy_EN)
  {
    snprintf(resp_payload, resp_size, "app_sync_disabled");
    return BLE_PROTO_ERR_PAYLOAD;
  }

  strcpy(payload_buf, payload);
  token = strtok(payload_buf, ",;");
  while (token != NULL)
  {
    char *eq = strchr(token, '=');
    char *key;
    char *value;
    if (eq == NULL)
    {
      return BLE_PROTO_ERR_PAYLOAD;
    }
    *eq = '\0';
    key = BleProto_Trim(token);
    value = BleProto_Trim(eq + 1);
    if (key == NULL || value == NULL || key[0] == '\0' || value[0] == '\0')
    {
      return BLE_PROTO_ERR_PAYLOAD;
    }

    if (!strcmp(key, "enabled"))
    {
      if (!BleProto_ParseU32(value, &value_u32) || value_u32 > 1U)
      {
        return BLE_PROTO_ERR_PAYLOAD;
      }
      enabled = (uint8_t)value_u32;
      has_update = 1U;
    }
    else if (!strcmp(key, "hour"))
    {
      if (!BleProto_ParseU32(value, &value_u32) || value_u32 > 23U)
      {
        return BLE_PROTO_ERR_PAYLOAD;
      }
      hour = (uint8_t)value_u32;
      has_update = 1U;
    }
    else if (!strcmp(key, "minute"))
    {
      if (!BleProto_ParseU32(value, &value_u32) || value_u32 > 59U)
      {
        return BLE_PROTO_ERR_PAYLOAD;
      }
      minute = (uint8_t)value_u32;
      has_update = 1U;
    }
    else if (!strcmp(key, "time"))
    {
      if (!BleProto_ParseTimeHm(value, &hour, &minute))
      {
        return BLE_PROTO_ERR_PAYLOAD;
      }
      has_update = 1U;
    }
    else
    {
      return BLE_PROTO_ERR_PAYLOAD;
    }
    token = strtok(NULL, ",;");
  }

  if (!has_update)
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }

  g_app_data.alarm_enabled = enabled;
  g_app_data.alarm_hour = hour;
  g_app_data.alarm_minute = minute;
  if (g_app_data.alarm_enabled)
  {
    RTC_SetDailyAlarm(g_app_data.alarm_hour, g_app_data.alarm_minute);
  }
  else
  {
    RTC_DisableAlarm();
  }
  User_RequestDataSave();
  BleProto_BuildAlarmPayload(resp_payload, resp_size);
  return BLE_PROTO_ERR_OK;
}

static uint8_t BleProto_HandleGetLock(char *resp_payload, uint16_t resp_size)
{
  if (resp_payload == NULL || resp_size == 0U)
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }

  BleProto_BuildLockPayload(resp_payload, resp_size);
  return BLE_PROTO_ERR_OK;
}

static uint8_t BleProto_HandleGetNfc(char *resp_payload, uint16_t resp_size)
{
  if (resp_payload == NULL || resp_size == 0U)
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }

  BleProto_BuildNfcPayload(resp_payload, resp_size);
  return BLE_PROTO_ERR_OK;
}

static uint8_t BleProto_HandleSetLock(const char *payload, char *resp_payload, uint16_t resp_size)
{
  char payload_buf[BLE_PROTO_PAYLOAD_MAX];
  char *token;
  uint32_t value_u32 = 0;
  uint8_t enabled = g_app_data.password_enabled;
  uint8_t new_pin[4];
  uint8_t has_update = 0U;
  uint8_t has_pin = 0U;

  if (resp_payload != NULL && resp_size > 0U)
  {
    resp_payload[0] = '\0';
  }

  memcpy(new_pin, g_app_data.pin, sizeof(new_pin));

  if (payload == NULL || resp_payload == NULL || resp_size == 0U || payload[0] == '\0' ||
      strlen(payload) >= sizeof(payload_buf))
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }

  if (!ui_APPSy_EN)
  {
    snprintf(resp_payload, resp_size, "app_sync_disabled");
    return BLE_PROTO_ERR_PAYLOAD;
  }

  strcpy(payload_buf, payload);
  token = strtok(payload_buf, ",;");
  while (token != NULL)
  {
    char *eq = strchr(token, '=');
    char *key;
    char *value;
    if (eq == NULL)
    {
      return BLE_PROTO_ERR_PAYLOAD;
    }
    *eq = '\0';
    key = BleProto_Trim(token);
    value = BleProto_Trim(eq + 1);
    if (key == NULL || value == NULL || key[0] == '\0' || value[0] == '\0')
    {
      return BLE_PROTO_ERR_PAYLOAD;
    }

    if (!strcmp(key, "enabled"))
    {
      if (!BleProto_ParseU32(value, &value_u32) || value_u32 > 1U)
      {
        return BLE_PROTO_ERR_PAYLOAD;
      }
      enabled = (uint8_t)value_u32;
      has_update = 1U;
    }
    else if (!strcmp(key, "pin"))
    {
      if (!BleProto_ParsePin4(value, new_pin))
      {
        snprintf(resp_payload, resp_size, "bad_pin");
        return BLE_PROTO_ERR_PAYLOAD;
      }
      has_pin = 1U;
      has_update = 1U;
    }
    else
    {
      return BLE_PROTO_ERR_PAYLOAD;
    }

    token = strtok(NULL, ",;");
  }

  if (!has_update)
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }

  if (has_pin)
  {
    memcpy(g_app_data.pin, new_pin, sizeof(g_app_data.pin));
  }
  g_app_data.password_enabled = enabled;
  User_RequestDataSave();

  if (!g_app_data.password_enabled && ui_LockScreenIsActive())
  {
    Page_Back();
  }

  BleProto_BuildLockPayload(resp_payload, resp_size);
  return BLE_PROTO_ERR_OK;
}

static uint8_t BleProto_HandleSetNfc(const char *payload, char *resp_payload, uint16_t resp_size)
{
  char payload_buf[BLE_PROTO_PAYLOAD_MAX];
  char *token;
  uint32_t value_u32 = 0;
  uint8_t enabled = g_app_data.nfc_enabled;
  uint8_t slot = g_app_data.active_nfc_slot;
  uint8_t has_update = 0U;

  if (resp_payload != NULL && resp_size > 0U)
  {
    resp_payload[0] = '\0';
  }

  if (payload == NULL || resp_payload == NULL || resp_size == 0U || payload[0] == '\0' ||
      strlen(payload) >= sizeof(payload_buf))
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }

  if (!ui_APPSy_EN)
  {
    snprintf(resp_payload, resp_size, "app_sync_disabled");
    return BLE_PROTO_ERR_PAYLOAD;
  }

  strcpy(payload_buf, payload);
  token = strtok(payload_buf, ",;");
  while (token != NULL)
  {
    char *eq = strchr(token, '=');
    char *key;
    char *value;
    if (eq == NULL)
    {
      return BLE_PROTO_ERR_PAYLOAD;
    }
    *eq = '\0';
    key = BleProto_Trim(token);
    value = BleProto_Trim(eq + 1);
    if (key == NULL || value == NULL || key[0] == '\0' || value[0] == '\0')
    {
      return BLE_PROTO_ERR_PAYLOAD;
    }

    if (!strcmp(key, "enabled"))
    {
      if (!BleProto_ParseU32(value, &value_u32) || value_u32 > 1U)
      {
        return BLE_PROTO_ERR_PAYLOAD;
      }
      enabled = (uint8_t)value_u32;
      has_update = 1U;
    }
    else if (!strcmp(key, "slot"))
    {
      if (!BleProto_ParseU32(value, &value_u32) || value_u32 >= APPDATA_NFC_SLOT_COUNT)
      {
        snprintf(resp_payload, resp_size, "bad_slot");
        return BLE_PROTO_ERR_PAYLOAD;
      }
      slot = (uint8_t)value_u32;
      has_update = 1U;
    }
    else
    {
      return BLE_PROTO_ERR_PAYLOAD;
    }

    token = strtok(NULL, ",;");
  }

  if (!has_update)
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }

  if (!AppData_IsValidNfcSlot(slot))
  {
    if (enabled)
    {
      slot = AppData_FindFirstNfcSlot();
      if (!AppData_IsValidNfcSlot(slot))
      {
        snprintf(resp_payload, resp_size, "no_slot");
        return BLE_PROTO_ERR_PAYLOAD;
      }
    }
    else
    {
      slot = AppData_FindFirstNfcSlot();
    }
  }

  g_app_data.active_nfc_slot = AppData_IsValidNfcSlot(slot) ? slot : APPDATA_NFC_SLOT_NONE;
  g_app_data.nfc_enabled = (uint8_t)(enabled && AppData_IsValidNfcSlot(g_app_data.active_nfc_slot));
  User_RequestDataSave();

  BleProto_BuildNfcPayload(resp_payload, resp_size);
  return BLE_PROTO_ERR_OK;
}

static uint8_t BleProto_HandleGetHistory(char *resp_payload, uint16_t resp_size)
{
  if (resp_payload == NULL || resp_size == 0U)
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }
  if (!BleProto_BuildHistoryPayload(resp_payload, resp_size))
  {
    snprintf(resp_payload, resp_size, "history_overflow");
    return BLE_PROTO_ERR_LENGTH;
  }
  return BLE_PROTO_ERR_OK;
}

static uint8_t BleProto_HandleGetSportRecords(char *resp_payload, uint16_t resp_size)
{
  if (resp_payload == NULL || resp_size == 0U)
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }
  if (!BleProto_BuildSportRecordPayload(resp_payload, resp_size))
  {
    snprintf(resp_payload, resp_size, "sport_overflow");
    return BLE_PROTO_ERR_LENGTH;
  }
  return BLE_PROTO_ERR_OK;
}

static uint8_t BleProto_HandleGetWeather(char *resp_payload, uint16_t resp_size)
{
  if (resp_payload == NULL || resp_size == 0U)
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }

  BleProto_BuildWeatherPayload(resp_payload, resp_size);
  return BLE_PROTO_ERR_OK;
}

static uint8_t BleProto_HandleSetWeather(const char *payload, char *resp_payload, uint16_t resp_size)
{
  char payload_buf[BLE_PROTO_PAYLOAD_MAX];
  char weather_text[APPDATA_WEATHER_TEXT_MAX];
  char *token;
  int32_t value_i32 = 0;
  int8_t temp = 0;
  int8_t low = 0;
  int8_t high = 0;
  uint8_t has_temp = 0U;
  uint8_t has_text = 0U;

  if (resp_payload != NULL && resp_size > 0U)
  {
    resp_payload[0] = '\0';
  }
  memset(weather_text, 0, sizeof(weather_text));

  if (payload == NULL || resp_payload == NULL || resp_size == 0U || payload[0] == '\0' ||
      strlen(payload) >= sizeof(payload_buf))
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }

  if (!ui_APPSy_EN)
  {
    snprintf(resp_payload, resp_size, "app_sync_disabled");
    return BLE_PROTO_ERR_PAYLOAD;
  }

  strcpy(payload_buf, payload);
  token = strtok(payload_buf, ",;");
  while (token != NULL)
  {
    char *eq = strchr(token, '=');
    char *key;
    char *value;
    if (eq == NULL)
    {
      return BLE_PROTO_ERR_PAYLOAD;
    }
    *eq = '\0';
    key = BleProto_Trim(token);
    value = BleProto_Trim(eq + 1);
    if (key == NULL || value == NULL || key[0] == '\0' || value[0] == '\0')
    {
      return BLE_PROTO_ERR_PAYLOAD;
    }

    if (!strcmp(key, "temp"))
    {
      if (!BleProto_ParseI32(value, &value_i32) || value_i32 < -40L || value_i32 > 80L)
      {
        return BLE_PROTO_ERR_PAYLOAD;
      }
      temp = (int8_t)value_i32;
      has_temp = 1U;
    }
    else if (!strcmp(key, "low"))
    {
      if (!BleProto_ParseI32(value, &value_i32) || value_i32 < -40L || value_i32 > 80L)
      {
        return BLE_PROTO_ERR_PAYLOAD;
      }
      low = (int8_t)value_i32;
    }
    else if (!strcmp(key, "high"))
    {
      if (!BleProto_ParseI32(value, &value_i32) || value_i32 < -40L || value_i32 > 80L)
      {
        return BLE_PROTO_ERR_PAYLOAD;
      }
      high = (int8_t)value_i32;
    }
    else if (!strcmp(key, "text"))
    {
      strncpy(weather_text, value, sizeof(weather_text) - 1U);
      has_text = 1U;
    }
    else if (!strcmp(key, "clear"))
    {
      if (!strcmp(value, "1"))
      {
        AppRuntime_ClearWeather();
        snprintf(resp_payload, resp_size, "weather=cleared");
        return BLE_PROTO_ERR_OK;
      }
      return BLE_PROTO_ERR_PAYLOAD;
    }
    else
    {
      return BLE_PROTO_ERR_PAYLOAD;
    }

    token = strtok(NULL, ",;");
  }

  if (!has_temp)
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }

  if (!has_text)
  {
    strcpy(weather_text, "Weather");
  }

  if (low == 0 && high == 0)
  {
    low = temp;
    high = temp;
  }

  AppRuntime_SetWeather(temp, low, high, weather_text);
  snprintf(resp_payload, resp_size, "weather=%s,%d,%d,%d", g_app_runtime.weather_text, (int)g_app_runtime.weather_temp,
           (int)g_app_runtime.weather_low, (int)g_app_runtime.weather_high);
  return BLE_PROTO_ERR_OK;
}

static uint8_t BleProto_HandleFindWatch(const char *payload, char *resp_payload, uint16_t resp_size)
{
  if (resp_payload != NULL && resp_size > 0U)
  {
    resp_payload[0] = '\0';
  }

  if (resp_payload == NULL || resp_size == 0U)
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }

  if (payload != NULL && payload[0] != '\0' && strcmp(payload, "state=1") != 0)
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }

  AppRuntime_RequestFindWatch();
  snprintf(resp_payload, resp_size, "active=1");
  return BLE_PROTO_ERR_OK;
}

static uint8_t BleProto_HandlePushNotify(const char *payload, char *resp_payload, uint16_t resp_size)
{
  char payload_buf[BLE_PROTO_PAYLOAD_MAX];
  char notify_title[APPDATA_NOTIFY_TITLE_MAX];
  char notify_body[APPDATA_NOTIFY_BODY_MAX];
  char *token;
  uint8_t has_body = 0U;

  if (resp_payload != NULL && resp_size > 0U)
  {
    resp_payload[0] = '\0';
  }

  memset(notify_title, 0, sizeof(notify_title));
  memset(notify_body, 0, sizeof(notify_body));

  if (payload == NULL || resp_payload == NULL || resp_size == 0U || payload[0] == '\0' ||
      strlen(payload) >= sizeof(payload_buf))
  {
    return BLE_PROTO_ERR_PAYLOAD;
  }

  if (!ui_APPSy_EN)
  {
    snprintf(resp_payload, resp_size, "app_sync_disabled");
    return BLE_PROTO_ERR_PAYLOAD;
  }

  strcpy(payload_buf, payload);
  token = strtok(payload_buf, ",;");
  while (token != NULL)
  {
    char *eq = strchr(token, '=');
    char *key;
    char *value;
    if (eq == NULL)
    {
      return BLE_PROTO_ERR_PAYLOAD;
    }
    *eq = '\0';
    key = BleProto_Trim(token);
    value = BleProto_Trim(eq + 1);
    if (key == NULL || value == NULL || key[0] == '\0' || value[0] == '\0')
    {
      return BLE_PROTO_ERR_PAYLOAD;
    }

    if (!strcmp(key, "title"))
    {
      strncpy(notify_title, value, sizeof(notify_title) - 1U);
    }
    else if (!strcmp(key, "body") || !strcmp(key, "text"))
    {
      strncpy(notify_body, value, sizeof(notify_body) - 1U);
      has_body = 1U;
    }
    else
    {
      return BLE_PROTO_ERR_PAYLOAD;
    }

    token = strtok(NULL, ",;");
  }

  if (!has_body)
  {
    snprintf(resp_payload, resp_size, "body_required");
    return BLE_PROTO_ERR_PAYLOAD;
  }

  AppRuntime_SetPhoneNotification(notify_title, notify_body);
  snprintf(resp_payload, resp_size, "queued=1,title=%s", g_app_runtime.phone_notification_title);
  return BLE_PROTO_ERR_OK;
}

static void BleProto_HandleFrame(char *frame_text)
{
  BleProtoReq_t req;
  uint8_t parse_err;
  uint8_t err_seq;
  char resp_payload[BLE_PROTO_PAYLOAD_MAX];
  uint8_t cfg_ret;
  uint8_t time_ret;
  uint8_t alarm_ret;
  uint8_t lock_ret;
  uint8_t nfc_ret;
  uint8_t history_ret;
  uint8_t sport_ret;
  uint8_t weather_get_ret;
  uint8_t weather_ret;
  uint8_t find_ret;
  uint8_t notify_ret;

  if (!BleProto_ParseFrame(frame_text, &req, &parse_err, &err_seq))
  {
    BleProto_SendFrame(err_seq, 0, parse_err, "parse_error");
    return;
  }

  resp_payload[0] = '\0';

  if (!strcmp(req.cmd, "GET_STATUS"))
  {
    BleProto_BuildStatusPayload(resp_payload, sizeof(resp_payload));
    BleProto_SendFrame(req.seq, 1, BLE_PROTO_ERR_OK, resp_payload);
  }
  else if (!strcmp(req.cmd, "SET_CFG"))
  {
    cfg_ret = BleProto_HandleSetCfg(req.payload, resp_payload, sizeof(resp_payload));
    if (cfg_ret == BLE_PROTO_ERR_OK)
    {
      BleProto_SendFrame(req.seq, 1, BLE_PROTO_ERR_OK, resp_payload);
    }
    else
    {
      BleProto_SendFrame(req.seq, 0, cfg_ret, resp_payload[0] != '\0' ? resp_payload : "bad_payload");
    }
  }
  else if (!strcmp(req.cmd, "SET_TIME"))
  {
    time_ret = BleProto_HandleSetTime(req.payload, resp_payload, sizeof(resp_payload));
    if (time_ret == BLE_PROTO_ERR_OK)
    {
      BleProto_SendFrame(req.seq, 1, BLE_PROTO_ERR_OK, resp_payload);
    }
    else
    {
      BleProto_SendFrame(req.seq, 0, time_ret, resp_payload[0] != '\0' ? resp_payload : "bad_payload");
    }
  }
  else if (!strcmp(req.cmd, "GET_ALARM"))
  {
    alarm_ret = BleProto_HandleGetAlarm(resp_payload, sizeof(resp_payload));
    BleProto_SendFrame(req.seq, alarm_ret == BLE_PROTO_ERR_OK ? 1U : 0U, alarm_ret,
                       resp_payload[0] != '\0' ? resp_payload : "bad_payload");
  }
  else if (!strcmp(req.cmd, "GET_LOCK"))
  {
    lock_ret = BleProto_HandleGetLock(resp_payload, sizeof(resp_payload));
    BleProto_SendFrame(req.seq, lock_ret == BLE_PROTO_ERR_OK ? 1U : 0U, lock_ret,
                       resp_payload[0] != '\0' ? resp_payload : "bad_payload");
  }
  else if (!strcmp(req.cmd, "GET_NFC"))
  {
    nfc_ret = BleProto_HandleGetNfc(resp_payload, sizeof(resp_payload));
    BleProto_SendFrame(req.seq, nfc_ret == BLE_PROTO_ERR_OK ? 1U : 0U, nfc_ret,
                       resp_payload[0] != '\0' ? resp_payload : "bad_payload");
  }
  else if (!strcmp(req.cmd, "SET_ALARM"))
  {
    alarm_ret = BleProto_HandleSetAlarm(req.payload, resp_payload, sizeof(resp_payload));
    BleProto_SendFrame(req.seq, alarm_ret == BLE_PROTO_ERR_OK ? 1U : 0U, alarm_ret,
                       resp_payload[0] != '\0' ? resp_payload : "bad_payload");
  }
  else if (!strcmp(req.cmd, "SET_LOCK"))
  {
    lock_ret = BleProto_HandleSetLock(req.payload, resp_payload, sizeof(resp_payload));
    BleProto_SendFrame(req.seq, lock_ret == BLE_PROTO_ERR_OK ? 1U : 0U, lock_ret,
                       resp_payload[0] != '\0' ? resp_payload : "bad_payload");
  }
  else if (!strcmp(req.cmd, "SET_NFC"))
  {
    nfc_ret = BleProto_HandleSetNfc(req.payload, resp_payload, sizeof(resp_payload));
    BleProto_SendFrame(req.seq, nfc_ret == BLE_PROTO_ERR_OK ? 1U : 0U, nfc_ret,
                       resp_payload[0] != '\0' ? resp_payload : "bad_payload");
  }
  else if (!strcmp(req.cmd, "GET_HISTORY"))
  {
    history_ret = BleProto_HandleGetHistory(resp_payload, sizeof(resp_payload));
    BleProto_SendFrame(req.seq, history_ret == BLE_PROTO_ERR_OK ? 1U : 0U, history_ret,
                       resp_payload[0] != '\0' ? resp_payload : "bad_payload");
  }
  else if (!strcmp(req.cmd, "GET_SPORT_RECORDS"))
  {
    sport_ret = BleProto_HandleGetSportRecords(resp_payload, sizeof(resp_payload));
    BleProto_SendFrame(req.seq, sport_ret == BLE_PROTO_ERR_OK ? 1U : 0U, sport_ret,
                       resp_payload[0] != '\0' ? resp_payload : "bad_payload");
  }
  else if (!strcmp(req.cmd, "GET_WEATHER"))
  {
    weather_get_ret = BleProto_HandleGetWeather(resp_payload, sizeof(resp_payload));
    BleProto_SendFrame(req.seq, weather_get_ret == BLE_PROTO_ERR_OK ? 1U : 0U, weather_get_ret,
                       resp_payload[0] != '\0' ? resp_payload : "bad_payload");
  }
  else if (!strcmp(req.cmd, "SET_WEATHER"))
  {
    weather_ret = BleProto_HandleSetWeather(req.payload, resp_payload, sizeof(resp_payload));
    BleProto_SendFrame(req.seq, weather_ret == BLE_PROTO_ERR_OK ? 1U : 0U, weather_ret,
                       resp_payload[0] != '\0' ? resp_payload : "bad_payload");
  }
  else if (!strcmp(req.cmd, "FIND_WATCH"))
  {
    find_ret = BleProto_HandleFindWatch(req.payload, resp_payload, sizeof(resp_payload));
    BleProto_SendFrame(req.seq, find_ret == BLE_PROTO_ERR_OK ? 1U : 0U, find_ret,
                       resp_payload[0] != '\0' ? resp_payload : "bad_payload");
  }
  else if (!strcmp(req.cmd, "PUSH_NOTIFY"))
  {
    notify_ret = BleProto_HandlePushNotify(req.payload, resp_payload, sizeof(resp_payload));
    BleProto_SendFrame(req.seq, notify_ret == BLE_PROTO_ERR_OK ? 1U : 0U, notify_ret,
                       resp_payload[0] != '\0' ? resp_payload : "bad_payload");
  }
  else
  {
    BleProto_SendFrame(req.seq, 0, BLE_PROTO_ERR_CMD, "unknown_cmd");
  }
}

static void Legacy_HandleCommand(char *cmd_text)
{
  if (!strcmp(cmd_text, "OV"))
  {
    printf("OK\r\n");
  }
  else if (!strcmp(cmd_text, "OV+VERSION"))
  {
    printf("VERSION=V%d.%d.%d\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
  }
  else if (!strcmp(cmd_text, "OV+SEND"))
  {
    HAL_RTC_GetTime(&hrtc, &(BLEMessage.nowtime), RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &BLEMessage.nowdate, RTC_FORMAT_BIN);
    BLEMessage.humi = HWInterface.AHT21.humidity;
    BLEMessage.temp = HWInterface.AHT21.temperature;
    BLEMessage.HR = HWInterface.HR_meter.HrRate;
    BLEMessage.SPO2 = HWInterface.HR_meter.SPO2;
    BLEMessage.stepNum = HWInterface.IMU.Steps;

    printf("data:%2d-%02d\r\n", BLEMessage.nowdate.Month, BLEMessage.nowdate.Date);
    printf("time:%02d:%02d:%02d\r\n", BLEMessage.nowtime.Hours, BLEMessage.nowtime.Minutes, BLEMessage.nowtime.Seconds);
    printf("humidity:%d%%\r\n", BLEMessage.humi);
    printf("temperature:%d\r\n", BLEMessage.temp);
    printf("Heart Rate:%d%%\r\n", BLEMessage.HR);
    printf("SPO2:%d%%\r\n", BLEMessage.SPO2);
    printf("Step today:%d\r\n", BLEMessage.stepNum);
  }
  // set time // OV+ST=20230629125555
  else if (strlen(cmd_text) == 20U)
  {
    uint8_t cmd[10];
    memset(cmd, 0, sizeof(cmd));
    StrCMD_Get((uint8_t *)cmd_text, cmd);
    if (ui_APPSy_EN && !strcmp((char *)cmd, "OV+ST"))
    {
      TimeFormat_Get((uint8_t *)cmd_text);
    }
  }
}

/**
  * @brief  send the message via BLE, use uart
  * @param  argument: Not used
  * @retval None
  */
void MessageSendTask(void *argument)
{
	while(1)
	{
		if(HardInt_uart_flag)
		{
			HardInt_uart_flag = 0;
			uint8_t IdleBreakstr = 0;
			osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1);
			printf("RecStr:%s\r\n",HardInt_receive_str);
			if(HardInt_receive_str[0] == '#')
			{
				BleProto_HandleFrame((char *)HardInt_receive_str);
			}
			else if(!strcmp(HardInt_receive_str,"OV"))
			{
				printf("OK\r\n");
			}
			else if(!strcmp(HardInt_receive_str,"OV+VERSION"))
			{
				printf("VERSION=V%d.%d.%d\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
			}
			else if(!strcmp(HardInt_receive_str,"OV+SEND"))
			{
				HAL_RTC_GetTime(&hrtc,&(BLEMessage.nowtime),RTC_FORMAT_BIN);
				HAL_RTC_GetDate(&hrtc,&BLEMessage.nowdate,RTC_FORMAT_BIN);
				BLEMessage.humi = HWInterface.AHT21.humidity;
				BLEMessage.temp = HWInterface.AHT21.temperature;
				BLEMessage.HR = HWInterface.HR_meter.HrRate;
				BLEMessage.SPO2 = HWInterface.HR_meter.SPO2;
				BLEMessage.stepNum = HWInterface.IMU.Steps;

				printf("data:%2d-%02d\r\n",BLEMessage.nowdate.Month,BLEMessage.nowdate.Date);
				printf("time:%02d:%02d:%02d\r\n",BLEMessage.nowtime.Hours,BLEMessage.nowtime.Minutes,BLEMessage.nowtime.Seconds);
				printf("humidity:%d%%\r\n",BLEMessage.humi);
				printf("temperature:%d\r\n",BLEMessage.temp);
				printf("Heart Rate:%d%%\r\n",BLEMessage.HR);
				if(BLEMessage.SPO2 > 0U)
				{
					printf("SPO2:%d%%\r\n",BLEMessage.SPO2);
				}
				else
				{
					printf("SPO2:N/A\r\n");
				}
				printf("SPO2 State:%s\r\n", user_SPO2StateToString(HWInterface.HR_meter.SPO2State));
				printf("PPG Raw:%u\r\n", HWInterface.HR_meter.RawSample);
				printf("PPG Amp:%u\r\n", HWInterface.HR_meter.SampleAmplitude);
				printf("PPG Quality:%u%%\r\n", HWInterface.HR_meter.SignalQuality);
				printf("Step today:%d\r\n",BLEMessage.stepNum);
			}
			//set time//OV+ST=20230629125555
			else if(strlen(HardInt_receive_str)==20)
			{
				uint8_t cmd[10];
				memset(cmd,0,sizeof(cmd));
				StrCMD_Get(HardInt_receive_str,cmd);
				if(ui_APPSy_EN && !strcmp(cmd,"OV+ST"))
				{
					TimeFormat_Get(HardInt_receive_str);
				}
			}
			memset(HardInt_receive_str,0,sizeof(HardInt_receive_str));
		}
		osDelay(1000);
	}
}
