# Build and Program PSoC 5LP

This note records the working command-line flow for
`AcondicionamientoAnalogico.cydsn`.

## Hardware

- KitProg USB-UART/Programmer: `COM6`
- PPCLI port name observed on this machine: `KitProg/1D1F17F002152400`
- Active PSoC Creator project:
  `C:\Github\Tesis\src\psoc\AcondicionamientoAnalogico.cydsn`

## Build

Run from PowerShell:

```powershell
& "C:\Program Files (x86)\Cypress\PSoC Creator\4.4\PSoC Creator\bin\cyprjmgr.exe" `
  -wrk "C:\Github\Tesis\src\psoc\AcondicionamientoAnalogico.cydsn\DiferencialToSingleEnded_ESP-000.cywrk" `
  -build `
  -prj AcondicionamientoAnalogico `
  -c Debug
```

Expected hex:

```text
C:\Github\Tesis\src\psoc\AcondicionamientoAnalogico.cydsn\CortexM3\ARM_GCC_541\Debug\AcondicionamientoAnalogico.hex
```

## List KitProg Ports

Important: `ppcli.exe` must be run with working directory
`C:\Program Files (x86)\Cypress\Programmer`. If it is run from the repo
directory, `--runfile` can appear to hang instead of consuming the script.

```powershell
$script = Join-Path $env:TEMP "psoc_getports.cli"
@("GetPorts", "quit") | Set-Content -LiteralPath $script -Encoding ASCII
$runfile = $script -replace "\\", "/"

Push-Location "C:\Program Files (x86)\Cypress\Programmer"
try {
    & .\ppcli.exe "--runfile $runfile"
} finally {
    Pop-Location
}
```

Expected port:

```text
KitProg/1D1F17F002152400
```

## Program

This reuses the generated PPCLI example script and patches the first two
lines for this board and hex file.

```powershell
$example = "C:\Github\Tesis\src\psoc\PPCLI_example_scripts\PSoC3_5_PPCLI\example_script.cli"
$out = Join-Path $env:TEMP "psoc_program_acondicionamiento.cli"
$hex = "C:/Github/Tesis/src/psoc/AcondicionamientoAnalogico.cydsn/CortexM3/ARM_GCC_541/Debug/AcondicionamientoAnalogico.hex"

$lines = Get-Content -LiteralPath $example
$lines[0] = 'OpenPort KitProg/1D1F17F002152400 "C:\Program Files (x86)\Cypress\Programmer\"'
$lines[1] = "HEX_ReadFile `"$hex`""
if ($lines[-1].Trim().ToLowerInvariant() -ne "quit") { $lines += "quit" }
Set-Content -LiteralPath $out -Value $lines -Encoding ASCII

$runfile = $out -replace "\\", "/"
Push-Location "C:\Program Files (x86)\Cypress\Programmer"
try {
    & .\ppcli.exe "--runfile $runfile"
} finally {
    Pop-Location
}
```

Successful programming ends with:

```text
PSoC3_ProtectAll
PSoC3_VerifyProtect
DAP_ReleaseChip
ClosePort
quit
```

## Reset Target

If needed after programming:

```powershell
$script = Join-Path $env:TEMP "psoc_reset_target.cli"
@(
'OpenPort KitProg/1D1F17F002152400 "C:\Program Files (x86)\Cypress\Programmer\"',
'SetProtocol 8',
'SetProtocolClock 152',
'SetProtocolConnector 1',
'SetPowerVoltage 5.0',
'PowerOn',
'ToggleReset 0 100',
'ClosePort',
'quit'
) | Set-Content -LiteralPath $script -Encoding ASCII

$runfile = $script -replace "\\", "/"
Push-Location "C:\Program Files (x86)\Cypress\Programmer"
try {
    & .\ppcli.exe "--runfile $runfile"
} finally {
    Pop-Location
}
```

## Debug State Notes

Useful context for resuming this session:

- Normal source defaults are `PSOC_EARLY_UART_TEST=0` and
  `PSOC_TX1_BITBANG_TEST=0` in `main.c`.
- `PSOC_TX1_GPIO_TEST=0` is also normal. It is a temporary diagnostic mode in
  `main.c` that bypasses DSI (`PRTx_BYP`) and toggles `Tx_1=P1[5]` as GPIO.
- During the UART-link diagnosis, a temporary PSoC bitbang firmware was
  programmed. That test toggles the LED and tries to drive `Tx_1` manually.
  If the target LED keeps blinking forever, the board may still be running
  that diagnostic firmware. Rebuild and program the normal hex above to leave
  the diagnostic mode.
- Breakpoints were reached both at the `Tx_1` pin setup and inside the
  bitbang transmit function. That confirms the PSoC firmware is executing.
- On the ESP slave, the raw edge counter on `PSOC_UART_RX` reported
  `edge=0/1` with `uartBytes=0` while the PSoC diagnostic firmware was
  blinking. That means ESP `GPIO16` stayed high and saw no transitions.
- A slave-side GPIO scanner was added behind `PSOC_RX_SCAN_ENABLE`. On COM12,
  with `slave1` built with `-DPSOC_RX_SCAN_ENABLE=1`, the scanner also reported
  zero edges on the checked candidate GPIOs while normal PSoC firmware and the
  `PSOC_TX1_GPIO_TEST` firmware were programmed.
- This means the current failure is earlier/lower than autocalibration:
  no PSoC BOOT/PING bytes and no observed PSoC TX edges reach the monitored
  ESP image. Next debug should distinguish PSoC code execution, P1[5] register
  state, P1[5] pin state, and COM12/KitProg target pairing.

Regression checks on 2026-06-14:

- `0d2f8778` (`Ya funciona bien`) PSoC hex was programmed from a separate
  worktree. The old PSoC project did not rebuild from scratch because the
  worktree lacks complete `BNC`/`BoostConverter` component implementations, so
  the committed `CortexM3/ARM_GCC_541/Debug/AcondicionamientoAnalogico.hex`
  was used. Result with current scanner slave on COM12: `uartBytes=0`,
  `ping=0`, `edge=0/1`, and all scanned GPIOs reported zero edges.
- The matching `0d2f8778` slave firmware was then built and uploaded to COM12
  against the same old PSoC hex. Result: `uartBytes=0`, `ping=0`, `rx=1`.
- `ade58604` (`Previo a cambios`) PSoC hex was also programmed and tested with
  the current scanner slave. Result: `uartBytes=0`, `ping=0`, `edge=0/1`, and
  all scanned GPIOs reported zero edges.
- After those checks, the current PSoC Debug hex from `C:\Github\Tesis` was
  rebuilt/programmed again, and the current scanner slave was left on COM12.
  Final state remained `uartBytes=0`, `ping=0`, `edge=0/1`.

## CORRECTION 2026-06-14 (later the same day): UART link is fine

A fresh capture on COM12, same scanner slave + same current PSoC hex left
from the regression checks above, shows full normal operation from boot:

- `[SLAVE 1] boot` -> ESPNOW ready -> `RX_SCAN enabled` -> `PSoC: DETECTADO`
  -> `listo, esperando ARM`.
- After `ARM`: `uartBytes` climbs steadily (21 -> 9598+ over the session),
  `edge` counters increase into the tens of thousands, `ping=1-2`.
- A full 100-batch VIEW capture completed end-to-end (`START_OK` ->
  `VIEW_UART` progressing -> `FULL -> STOPPED (100 batches)` ->
  `DUMP_DONE bOK=100 fill=100/100`).
- A `0xB5` (Calibrar) command was processed: `CAL_START` ->
  `CAL begin stage=0/GEO_PGA` -> per-iteration `CAL_STAGE_DAC`/`CAL_STAGE_MEAS`
  telemetry -> `CAL_STAGE_OK val=1` (GEO_PGA converged, `dac=228 meas=-85`,
  within `CAL_TOL_COUNTS=250`) -> `CAL begin stage=1/GEO_BP`.

**Conclusion**: the "no PSoC BOOT/PING, no Tx edges" result from the
regression checks above was a transient/timing artifact of that specific
test run (e.g. monitor opened mid-boot, or board not re-reset after
programming) — **not** a pin-mapping/UART-routing/target-pairing problem.
`Rx_1=P1[2]`/`Tx_1=P1[5]` <-> ESP `GPIO17`/`GPIO16` wiring is confirmed
correct by this capture. Do not re-litigate the UART link unless a *new*
fresh capture from a clean reset also shows `uartBytes=0` for the full
`PSOC_STARTUP_CAL_DELAY_MS=5000` boot window and beyond.

The open question is now purely about calibration **convergence**, not
communication — see `HANDOFF_CALIBRATION.md` §14.

## Current PSoC Breakpoints / Watches

For the normal firmware, set breakpoints at:

```text
main.c:798  UART_Start();
main.c:811  uart_send_diag(PSOC_EVT_BOOT, PSOC_HW_CLASS);
main.c:681  uart_send_diag(PSOC_EVT_WAIT_ESP, 0u);
Generated_Source/PSoC5/UART.c:1075  UART_TXDATA_REG = txDataByte;
Generated_Source/PSoC5/UART_INT.c   CY_ISR(UART_TXISR)
```

For the GPIO diagnostic (`PSOC_TX1_GPIO_TEST=1`), set breakpoints at:

```text
main.c:772  tx1_gpio_detach_dsi();
main.c:777  tx1_gpio_write(0u);
main.c:779  tx1_gpio_write(1u);
```

Watch these expressions while stopped in the GPIO diagnostic loop:

```text
*(reg8 *)Tx_1__DR
*(reg8 *)Tx_1__PS
*(reg8 *)Tx_1__BYP
*(reg8 *)Tx_1__DM0
*(reg8 *)Tx_1__DM1
*(reg8 *)Tx_1__DM2
*(reg8 *)Tx_1__PRTDSI__OUT_SEL0
*(reg8 *)Tx_1__PRTDSI__OUT_SEL1
*(reg8 *)Tx_1__PRTDSI__OE_SEL0
*(reg8 *)Tx_1__PRTDSI__OE_SEL1
```

Interpretation:

- If `Tx_1__DR` changes but `Tx_1__PS` does not, debug PSoC pin drive/DSI
  override.
- If `Tx_1__PS` changes but the ESP scanner still reports `edge=0`, debug
  target pairing or ESP-side GPIO mapping.
- If execution never reaches `UART_TXDATA_REG` in normal firmware, debug the
  UART component/clock path before looking at analog/autocalibration.

Current UART wiring assumptions:

```text
PSoC Rx_1 = P1[2]  <- ESP GPIO17 / PSOC_UART_TX
PSoC Tx_1 = P1[5]  -> ESP GPIO16 / PSOC_UART_RX
GND common
```
