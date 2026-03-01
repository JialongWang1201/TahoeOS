
## ~~HAL Portability Documentation~~ ✓ DONE
**Completed:** `docs/sensor-extension.md` — four-file sensor extension pattern plus
Hardware Portability section covering iic_hal.h coupling, HW_USE_HARDWARE=0 simulator
mode, and MCU porting guidance.

## ~~cppcheck Blocking Mode~~ ✓ DONE
**Completed:** Fixed `strbuf` type/size in `ui_TimerPage.c`, initialised `temp.number`
in `StrCalculate.c`, suppressed false positives in generated font files and `ui.c`,
changed `--error-exitcode=0` → `--error-exitcode=1` in CI. Branch: `fix/cppcheck-gate`.
