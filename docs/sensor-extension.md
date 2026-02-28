# Sensor Extension Walkthrough

This document explains how to add a new sensor to TahoeOS and documents the hardware
portability boundary you must understand before porting to a non-STM32F411 board.

---

## Overview

TahoeOS reads sensors through a three-layer stack:

```
Application layer (Pages, HealthTrack)
        │
        ▼
HWDataAccess  (User/Func/Src/HWDataAccess.c)
  HWInterface singleton — function-pointer facade over all hardware
        │
        ▼
BSP drivers   (Software/OV_Watch/BSP/)
  AHT21, MPU6050, QMC5883L, BMP280, MAX30102, …
        │
        ▼
iic_hal / STM32 HAL  (BSP/IIC/iic_hal.h → HAL_GPIO_*)
  Bit-bang I2C over GPIO; STM32F411 specific
```

The dispatch loop in `SensorDataUpdateTask` polls every sensor on a tick-based
schedule. Adding a new sensor touches exactly four files.

---

## Step 1 — Write the BSP driver

Create `Software/OV_Watch/BSP/<SENSOR>/` with at minimum:

```
<SENSOR>.h   — data struct + public API
<SENSOR>.c   — I2C read/write logic using iic_hal
```

The driver should expose at least:

```c
typedef struct {
    uint8_t  ConnectionError;   /* set 1 when comms fail */
    /* sensor-specific readings */
    float    value;
} MyDevice_t;

void MyDevice_Init(iic_bus_t *bus);
void MyDevice_GetValue(MyDevice_t *dev);
```

Use `iic_read()` / `iic_write()` from `iic_hal.h` for I2C transactions — do not call
`HAL_I2C_*` directly in the driver header. See `BSP/AHT21/AHT21.c` for a reference
implementation.

---

## Step 2 — Register in HWDataAccess

**`Software/OV_Watch/User/Func/Inc/HWDataAccess.h`**

Add a member to `HW_DevInterface_t`:

```c
typedef struct {
    /* existing members … */
    MyDevice_t  MyDevice;
} HW_DevInterface_t;
```

**`Software/OV_Watch/User/Func/Src/HWDataAccess.c`**

Add an init call inside `HWDataAccess_Init()` and a stub for simulator builds:

```c
#if HW_USE_HARDWARE
    MyDevice_Init(&i2c_bus_1);
#else
    /* simulator stub — hard-code a plausible value */
    HWInterface.MyDevice.value = 42.0f;
#endif
```

---

## Step 3 — Add a poll function and dispatch entry

**`Software/OV_Watch/User/Tasks/Src/user_SensUpdateTask.c`**

Add a static poll function:

```c
static void sensor_poll_mydevice(void)
{
    MyDevice_GetValue(&HWInterface.MyDevice);
    /* optionally push reading into health-track */
}
```

Add an entry to `sensor_table[]`:

```c
static sensor_dispatch_entry_t sensor_table[] = {
    /* existing entries … */
    { 1000U, sensor_poll_mydevice, &HWInterface.MyDevice.ConnectionError, 0U },
};
```

Fields:
| Field | Meaning |
|---|---|
| `poll_interval_ms` | How often to call the poll function (milliseconds) |
| `poll_fn` | Pointer to the static poll function |
| `error_flag` | Points to the `ConnectionError` byte; poll is skipped while non-zero |
| `last_tick` | Initialise to `0U`; updated automatically by the scheduler |

The scheduler runs every `SENSOR_TASK_TICK_MS` (50 ms). Choose a `poll_interval_ms`
that is a multiple of 50 ms. Minimum useful value: 50 ms.

---

## Step 4 — Expose the reading to pages (optional)

If a UI page needs the value:

1. Declare an extern in `HWDataAccess.h` or read directly through `HWInterface.MyDevice`.
2. Call `lv_label_set_text_fmt()` in the page's LVGL timer callback.

No changes to the scheduler are needed — the dispatch table drives everything.

---

## Hardware Portability

> **TL;DR:** The simulator (`HW_USE_HARDWARE=0`) works on any host. Running on
> real silicon currently requires an STM32F411 because the BSP drivers call STM32 HAL
> GPIO functions directly. Porting to a different MCU means reimplementing the BSP
> layer.

### Where the coupling lives

`BSP/IIC/iic_hal.h` implements bit-bang I2C using STM32 HAL GPIO calls:

```c
/* iic_hal.h — simplified */
typedef struct {
    GPIO_TypeDef *SDA_Port;
    uint16_t      SDA_Pin;
    GPIO_TypeDef *SCL_Port;
    uint16_t      SCL_Pin;
} iic_bus_t;

static inline void iic_sda_high(iic_bus_t *bus) {
    HAL_GPIO_WritePin(bus->SDA_Port, bus->SDA_Pin, GPIO_PIN_SET);
}
/* … */
```

`GPIO_TypeDef`, `HAL_GPIO_WritePin`, and `HAL_GPIO_ReadPin` are all
STM32 HAL primitives defined in `stm32f4xx_hal_gpio.h`. Every BSP driver that
includes `iic_hal.h` therefore carries this dependency transitively.

### Simulator mode (`HW_USE_HARDWARE=0`)

`HWDataAccess.c` wraps every hardware init and read call in `#if HW_USE_HARDWARE`:

```c
#if HW_USE_HARDWARE
    AHT21_GetHumiTemp(&humi_value, &temp_value);
    HWInterface.AHT21.temperature = (int8_t)temp_value;
#else
    HWInterface.AHT21.temperature = 25;   /* stub */
#endif
```

Set `HW_USE_HARDWARE=0` in the build (e.g. the LVGL simulator CMake target) to
compile and run the full application stack — including all pages and the health-track
logic — on a desktop host without any hardware attached.

### Porting to a different MCU

To run TahoeOS on a non-STM32F411 board:

1. **Reimplement `iic_hal.h`** using your MCU's GPIO API. The struct fields
   (`SDA_Port`, `SCL_Port`, etc.) can be replaced with whatever your HAL uses to
   identify pins. All bit-bang timing logic lives in `iic_hal.h` and does not need
   to change.

2. **Replace the BSP driver includes.** Each driver in `BSP/*/` includes
   `stm32f4xx_hal.h` and `iic_hal.h`. After Step 1, only the `stm32f4xx_hal.h`
   include needs to be swapped for your MCU's HAL header; the driver logic itself
   is portable.

3. **Remap peripherals.** Some sensors use hardware I2C or SPI instead of bit-bang.
   If your MCU has a hardware I2C peripheral, replace the `iic_*` calls in the driver
   with the appropriate HAL calls for that peripheral.

4. **Update `HWDataAccess.c` init calls.** Pass the correct bus descriptor for your
   board's pin assignment.

The `HW_USE_HARDWARE` flag and the `HWInterface` singleton are fully portable —
they are pure C and carry no MCU-specific types.

### Summary of portability boundaries

```
PORTABLE (pure C, no MCU types)
  HWDataAccess.h / HWDataAccess.c  ← HWInterface singleton & stubs
  user_SensUpdateTask.c             ← dispatch table scheduler
  All User/ application code

NOT PORTABLE without modification
  BSP/IIC/iic_hal.h                ← GPIO_TypeDef, HAL_GPIO_*
  BSP/<SENSOR>/<SENSOR>.h/.c       ← includes iic_hal.h
  Core/ (STM32CubeMX generated)    ← clock, peripheral init
```

For a client board engagement, budget time to reimplement `iic_hal.h` and audit
each BSP driver for any direct `stm32f4xx_hal.h` references before committing to
a porting timeline.
