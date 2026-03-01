#ifndef __USER_DFUTRIGGER_H__
#define __USER_DFUTRIGGER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file    user_DfuTrigger.h
 * @brief   Jump to the STM32F411 ROM USB-DFU bootloader (AN2606).
 *
 * Usage:
 *   Call user_DfuTrigger_JumpToRom() from any FreeRTOS task or ISR-free
 *   context when you want the device to enumerate as a DFU device on USB.
 *
 * Safety:
 *   The internal IWDG is armed with a ~32 s timeout before the jump.
 *   If USB is not connected the IWDG fires and the MCU reboots to the
 *   application normally (IAP_F411 → app at 0x0800C000).
 *
 * This function does NOT return.
 */
void user_DfuTrigger_JumpToRom(void);

#ifdef __cplusplus
}
#endif

#endif /* __USER_DFUTRIGGER_H__ */
