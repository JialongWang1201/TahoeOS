# OV-Watch Display Debug Report (2026-02-06)

## Scope
- Target issue: the display stayed dark after flashing the firmware.
- Debug repo: `/Users/wangjialong/Software/OV-Watch`
- Debug method: OpenOCD + ST-Link register/flash inspection and board-level forced GPIO tests.

## Baseline Check

### 1) Confirm Boot/App images and boot conditions
- Read startup-related addresses:
  - `0x08000000` (boot vector table)
  - `0x0800C000` (app vector table)
  - `0x08008000` (`APP FLAG`)
- Verification results:
  - Boot vector table is valid (`SP=0x20020000`, `Reset=0x08002b09`)
  - App vector table is valid (`SP=0x20019870`, `Reset=0x0800c271`)
  - `APP FLAG` is `"APP FLAG"` (`0x20505041 0x47414C46`)

### 2) Reflash the complete image set to rule out image mismatch
Executed:
- `Firmware/BootLoader_F411.hex`
- `Firmware/OV_Watch_V2_4_4.bin` programmed at `0x0800C000`
- Rewrote `APP FLAG` at `0x08008000`

Result: OpenOCD reported `Programming Finished` and `Verified OK`.

## Runtime Evidence

### 3) Confirm runtime is in the App image
Multiple `reset run -> sleep -> halt` captures showed:
- `PC` repeatedly landed in `0x0801xxxx/0x0803xxxx`
- `VTOR (0xE000ED08)=0x0000C000`

Conclusion: the device reliably jumps to and executes the App image. This is not a "flash failed" or "stuck in boot while(1)" case.

### 4) Backlight path status
Key registers:
- `TIM3_CCR3 (0x4000043C)`
- `GPIOB_MODER (0x40020400)`
- `GPIOB_AFRL (0x40020420)`

Observed:
- `TIM3_CCR3` reached `0x96` (about 50% duty cycle)
- `PB0` was configured as `AF2(TIM3_CH3)`

This indicates the firmware is enabling the backlight PWM path.

### 5) CPU stall location
- Repeated halts showed `PC` around `0x080181f6~0x0801820e`
- The corresponding disassembly is the SysTick polling loop inside `delay_us()` (waiting for `SysTick->VAL` to change)
- This is runtime behavior, not a HardFault or panic

## Hardware Isolation Test

### 6) Forced backlight GPIO high/low test
Registers were modified while halted to:
- Temporarily switch `PB0` to a normal GPIO output and drive it high for 5 seconds
- Then drive it low for 5 seconds
- Restore `PB0` back to `AF2(TIM3_CH3)`

This test distinguishes between:
- Visible brightness change: backlight hardware path is good, and the fault is more likely in the LCD data path (SPI/DC/RES/panel init)
- No visible change: prioritize backlight hardware path investigation (FPC, backlight power, driver components)

## Final Conclusion
- **The firmware was flashed successfully, the App is running, and VTOR points to the App image.**
- **Backlight PWM can be driven to a valid duty cycle from software.**
- The remaining "display stays dark" symptom is more consistent with a **hardware path issue** (backlight or LCD panel connection/power) than a firmware programming issue.

## Repro Commands (Core Commands)

```bash
# 1) Read key runtime state
openocd -f Software/IAP_F411/openocd.cfg \
  -c "init; reset run; sleep 1200; halt; reg pc; mdw 0xE000ED08 1; mdw 0x08008000 2; mdw 0x0800C000 2; mdw 0x4000043C 1; shutdown"

# 2) Reflash boot + app + APP FLAG
openocd -f Software/IAP_F411/openocd.cfg \
  -c "init; reset halt; program Firmware/BootLoader_F411.hex verify; \
      program Firmware/OV_Watch_V2_4_4.bin 0x0800C000 verify; \
      flash write_image erase /tmp/ov_app_flag.bin 0x08008000 bin; \
      verify_image /tmp/ov_app_flag.bin 0x08008000 bin; reset run; shutdown"
```
