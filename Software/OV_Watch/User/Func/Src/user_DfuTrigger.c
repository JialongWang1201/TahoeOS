/**
 * @file    user_DfuTrigger.c
 * @brief   Jump to the STM32F411 ROM USB-DFU bootloader (ST AN2606).
 *
 * ROM bootloader address: 0x1FFF0000 (STM32F411 system memory).
 *
 * Sequence
 * ────────
 *  1. Disable the external GPIO watchdog (prevents WD reset during DFU).
 *  2. Arm the internal IWDG at maximum timeout (~32 s).
 *     → If USB is not connected, IWDG fires and the MCU reboots to the app.
 *  3. Disable all interrupts + SysTick.
 *  4. Load MSP from ROM vector table [0x1FFF0000].
 *  5. Jump to ROM reset handler at [0x1FFF0004].
 */

#include "user_DfuTrigger.h"
#include "main.h"      /* SCB, IWDG_TypeDef, __disable_irq, __set_MSP */
#include "WDOG.h"      /* WDOG_Disnable()                              */

/* ST AN2606: STM32F411 ROM bootloader base address */
#define ROM_DFU_BASE_ADDR   0x1FFF0000U

/* IWDG key register magic values */
#define IWDG_KEY_ACCESS     0x5555U   /* Unlock PR / RLR                 */
#define IWDG_KEY_START      0xCCCCU   /* Start IWDG (cannot be stopped)  */

typedef void (*pFunction)(void);

void user_DfuTrigger_JumpToRom(void)
{
    uint32_t   rom_sp;
    pFunction  rom_reset;

    /* Step 1: Disable external GPIO watchdog so it does not reset the
     *         device while the ROM bootloader is enumerating USB. */
    WDOG_Disnable();

    /* Step 2: Arm internal IWDG — prescaler /256, reload 4095 → ~32 s.
     *
     *   Timeout = (RLR + 1) * prescaler / f_LSI
     *           = 4096 * 256 / 32000 Hz ≈ 32.8 s
     *
     * The IWDG cannot be stopped once started.  If USB DFU does not
     * complete within ~32 s the IWDG resets the MCU; IAP_F411 will then
     * boot the application normally. */
    IWDG->KR  = IWDG_KEY_ACCESS;  /* Unlock PR and RLR registers    */
    IWDG->PR  = IWDG_PR_PR;       /* Prescaler = /256               */
    IWDG->RLR = IWDG_RLR_RL;      /* Reload = 0x0FFF (4095, max)    */
    while (IWDG->SR != 0U) {}     /* Wait for register update       */
    IWDG->KR  = IWDG_KEY_START;   /* Start IWDG                     */

    /* Step 3: Disable all interrupts and stop SysTick.
     *         The ROM bootloader re-initialises everything from scratch. */
    __disable_irq();
    SysTick->CTRL = 0U;

    /* Step 4+5: Read ROM vector table and jump. */
    rom_sp    = *(volatile uint32_t *)ROM_DFU_BASE_ADDR;
    rom_reset = (pFunction)(*(volatile uint32_t *)(ROM_DFU_BASE_ADDR + 4U));

    __set_MSP(rom_sp);
    rom_reset();

    /* Should never reach here. */
    while (1) {}
}
