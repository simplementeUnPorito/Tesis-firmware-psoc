# Build y programación PSoC 5LP

Flujo actual para `AcondicionamientoAnalogico.cydsn` en Windows con PSoC Creator 4.4.

## Estado del build actual (2026-07-07, 4 configs de ADC)

- Flash: **41982 bytes** -> filas a programar: **0..163**
  (`ceil(41982/256) - 1 = 163`)
- SRAM: 49352 bytes
- KitProg observado: `KitProg (CMSIS-DAP/236111)` — **siempre correr `GetPorts` antes de programar** porque el nombre puede variar.

### ADC: 4 configs de rango expuestas (antes eran 2)

El customizer del componente `ADC` (TopDesign) ahora tiene las 4 configs
activas, las 4 a **1020 SPS / 18 bits** (misma Fs de sistema, ver
`psoc_adc.h`):

| # | Nombre        | Rango de entrada | Alineación | Referencia         |
|---|---------------|-------------------|------------|---------------------|
| 1 | `CF_2V5`      | ±2.5 V            | Left (Bit-23 OVF Protected), `DEC_DIV=32` | Internal Vdda/4 |
| 2 | `CF_0V512`    | ±0.512 V          | Right, `DEC_DIV=0` | Internal 1.024 V |
| 3 | `CF_1V024`    | ±1.024 V          | Right, `DEC_DIV=0` | Internal 1.024 V |
| 4 | `CF_0V625`    | ±0.625 V          | Right, `DEC_DIV=0` | Internal Vdda/4 |

Firmware tocado para soportar las 4 (antes solo 1 y 2 estaban habilitadas
en `psoc_adc_set_config`):
- `psoc_adc.h` / `psoc_adc.c`: `psoc_adc_set_config` acepta 1..4.
- `main.c` (`dma_buf_to_adc_counts`): la extensión de signo right-aligned
  ahora se aplica a "cualquier config que no sea `CF_2V5`" (antes solo
  comparaba contra `CF_0V512`), porque las 3 configs 2/3/4 comparten
  `RESOLUTION=18` y `ALIGNMENT=0`.
- `psoc_hw.h`: comentario de `PSOC_CMD_ADC_CONFIG` (0xBA) actualizado:
  `1=CF_2V5, 2=CF_0V512, 3=CF_1V024, 4=CF_0V625`.
- ESP esclavo (`slave/src/main.cpp`): `isAdcConfigCode()` aceptaba solo
  1/2, ahora 1..4. Se agregó el comando USB de banco de pruebas `range N`
  (ver más abajo) para seleccionar la config sin pasar por el maestro.
- Web (`master/data/js/config.js`): `ADC_CONFIGS` tiene las 4 entradas
  (el dropdown de la UI ya era genérico — itera el array, no hacía falta
  tocar `slave_panel.js`). Se corrigió además un clamp viejo en `app.js`
  (`loadSlavePanelState`) que limitaba el valor restaurado de
  `localStorage` a `[1,2]`.

**Validado en hardware 2026-07-07** (PSoC recién programado con este build,
ESP esclavo2 en `COM12`, comando `range N` + secuencia de captura): las 4
configs completan captura limpia (`bBad=0`, `fill=1/1`, ack `ok=1`).
Ver sección "Prueba rápida por ESP USB" más abajo.

## Build (PowerShell)

```powershell
Push-Location "C:\Github\Tesis\src\psoc\AcondicionamientoAnalogico.cydsn"
try {
  & "C:\Program Files (x86)\Cypress\PSoC Creator\4.4\PSoC Creator\bin\cyprjmgr.exe" `
    -wrk "AcondicionamientoAnalogico.cywrk" `
    -build
} finally {
  Pop-Location
}
```

Hex generado:
```
CortexM3\ARM_GCC_541\Debug\AcondicionamientoAnalogico.hex
```

Calcular `$lastRow` tras cualquier rebuild:
```powershell
$lastRow = [int][Math]::Ceiling($flashUsedBytes / 256.0) - 1
```

## Listar puertos KitProg

Ejecutar desde `C:\Program Files (x86)\Cypress\Programmer`:

```powershell
$script = Join-Path $env:TEMP "psoc_getports.cli"
@("GetPorts", "quit") | Set-Content -LiteralPath $script -Encoding ASCII
Push-Location "C:\Program Files (x86)\Cypress\Programmer"
try { & .\ppcli.exe "--runfile $($script -replace '\\','/')" }
finally { Pop-Location }
```

Evitar `ppcli.exe GetPorts` directo: entra en modo interactivo y puede quedar
esperando prompt. Usar siempre runfile con `quit`.

## Programar (PPCLI)

```powershell
$timestamp  = Get-Date -Format 'yyyyMMdd_HHmmss'
$log        = Join-Path $env:TEMP "psoc_program_acondicionamiento_$timestamp.log"
$hex        = 'C:/Github/Tesis/src/psoc/AcondicionamientoAnalogico.cydsn/CortexM3/ARM_GCC_541/Debug/AcondicionamientoAnalogico.hex'
$port       = 'KitProg (CMSIS-DAP/236111)'   # reemplazar con resultado exacto de GetPorts
$programmer = 'C:/Program Files (x86)/Cypress/Programmer/'
$lastRow    = 159                             # actualizar según el build actual

$cmds = New-Object System.Collections.Generic.List[string]
$cmds.Add(('OpenPort "{0}" "{1}"' -f $port, $programmer))
$cmds.Add('SetAcquireMode Reset')
$cmds.Add('SetProtocol 8')
$cmds.Add('SetProtocolConnector 1')
$cmds.Add('SetProtocolClock 152')
$cmds.Add(('HEX_ReadFile "{0}"' -f $hex))
$cmds.Add('DAP_Acquire')
$cmds.Add('PSoC3_GetJtagID')
$cmds.Add('PSoC3_EraseAll')
0..$lastRow | ForEach-Object {
    $cmds.Add(('PSoC3_ProgramRowFromHex 0x00 {0} 0x01' -f $_))
    $cmds.Add(('PSoC3_VerifyRowFromHex 0x00 {0} 0x01' -f $_))
}
$cmds.Add('PSoC3_ProtectAll')
$cmds.Add('PSoC3_VerifyProtect')
$cmds.Add('DAP_ReleaseChip')
$cmds.Add('ClosePort')
$cmds.Add('quit')

$script = Join-Path $env:TEMP "psoc_program_acondicionamiento_$timestamp.cli"
$cmds | Set-Content -LiteralPath $script -Encoding ASCII
Push-Location "C:\Program Files (x86)\Cypress\Programmer"
try { & .\ppcli.exe "--runfile $($script -replace '\\','/')" | Tee-Object -FilePath $log }
finally { Pop-Location }
```

El programado exitoso termina con `0 OK` en cada fila y después:
```
PSoC3_ProtectAll → 0 OK
PSoC3_VerifyProtect → 0 OK
DAP_ReleaseChip → 0 OK
ClosePort → 0 OK
```

Ultimo programado validado:

- Build: 2026-07-07 12:37:50, `Build Succeeded` (4 configs de ADC — ver
  seccion de arriba).
- Flash/SRAM: 41982 / 49352 bytes.
- Filas: `0..163`, ECC option `0x01`.
- Log: `C:\Users\elias\AppData\Local\Temp\psoc_program_acondicionamiento_20260707_124954.log`.
- Programado y verificado con hardware real (KitProg CMSIS-DAP/236111,
  todas las filas `0 OK`) — capturas confirmadas en las 4 configs de ADC
  vía ESP esclavo `COM12` (ver "Prueba rápida por ESP USB").

Build previo (referencia historica):

- 2026-07-02 17:13:43, TC de timers sin IRQ ni sticky; ver
  `docs/psoc_supermaquina_handoff.md`, seccion "Politica de eventos
  determinismo-primero". Flash/SRAM: 40758 / 49352 bytes, filas `0..159`.

Notas del build actual:

- `Timer_3` sigue presente en TopDesign pero el firmware no lo usa ni registra
  `isr_Timer_3`; se puede eliminar cuando se permita tocar componentes.
- El intento de mover el descarte FIR de 63 muestras a `superMaquina.v` no cabe
  en recursos PLD/UDB con el diseno actual. Mantener el descarte en C hasta
  liberar recursos.

## Reset de target

Si hace falta resetear después de programar:

```powershell
$script = Join-Path $env:TEMP "psoc_reset.cli"
@(
  'OpenPort "KitProg (CMSIS-DAP/236111)" "C:\Program Files (x86)\Cypress\Programmer\"',
  'SetProtocol 8',
  'SetProtocolClock 152',
  'SetProtocolConnector 1',
  'ToggleReset 0 100',
  'ClosePort',
  'quit'
) | Set-Content -LiteralPath $script -Encoding ASCII
Push-Location "C:\Program Files (x86)\Cypress\Programmer"
try { & .\ppcli.exe "--runfile $($script -replace '\\','/')" }
finally { Pop-Location }
```

## Titilar LED desde la web

"Titilar LED" en la UI web titila el **LED del PSoC** (no del ESP).
Cadena: web → maestro → ESP-NOW `CMD_BLINK_LED` → esclavo → UART `0xB9` → PSoC.
El PSoC parpadea ~8 s a 2.5 Hz (no bloqueante, servido por `Timer_2` fixed ILO 100 kHz)
y vuelve a encendido fijo.

## Prueba rápida por ESP USB

Puertos observados 2026-07-06: ESP esclavo `COM12`, maestro `COM8`. Para esta
prueba usar el esclavo (`COM12`, 115200).

Secuencia validada tras el ultimo flash:

```text
clear
probe
stream 0
debugpsoc 0
cap 1
status      # bBad=0, fill=1/1
clear
stream 1
cap 2
status      # bBad=0, fill=2/2
clear
```

Pruebas adicionales del mismo firmware: `blink` no bloqueo pings; `cal` no
colgo el sistema y una captura posterior `cap 1` volvio a completar.

### Comando `range N` — banco de pruebas para las 4 configs de ADC (2026-07-07)

Firmware de esclavo compilado con flags de banco de pruebas (no son las de
`platformio.ini` por defecto, que traen todo apagado para campo):

```bash
cd src/esp/Nodo\ comunicación/slave
PLATFORMIO_BUILD_FLAGS="-DSLAVE_USB_CMD_ENABLE=1 -DSLAVE_LAB_TOOLS_ENABLE=1 -DSLAVE_LOGS_ENABLE=1 -DDBG_HUMAN=1" \
  pio run -e slave2 -t upload
```

Se agregó el comando USB `range N` (1..4) que llama directo a
`psoc.setAdcConfig(N)` sin pasar por el maestro/ESP-NOW — útil para probar
el ADC standalone. Secuencia por config:

```text
range 1        # o 2, 3, 4
clear
stream 0
debugpsoc 0
cap 1
status         # bBad=0, fill=1/1
stop
clear
```

**Importante**: el PSoC corre auto-calibración al boot/reset de config
(`PSOC_AUTO_CAL_ON_READY=1`). Si se manda `cap` mientras hay `[CAL] busy`/
`[CAL] pi stage=...` en el log, la captura queda colgada en
`START_SYNC_STALL` y el estado no vuelve a IDLE solo con `clear` — hace
falta `stop` explícito. Esperar a `[CAL] done ok=1` (o unos ~8 s tras el
boot) antes de capturar evita el problema; no es un bug de las configs
nuevas, pasa igual con las viejas si se apura la primera captura.

Resultado validado (PSoC recién programado, calibración asentada):

| Config | `psoc cfg ack` | Captura | Status |
|--------|-----------------|---------|--------|
| 1 `CF_2V5`   | ok=1 | FULL -> STOPPED | bOK++, bBad=0, fill=1/1 |
| 2 `CF_0V512` | ok=1 | FULL -> STOPPED | bOK++, bBad=0, fill=1/1 |
| 3 `CF_1V024` | ok=1 | FULL -> STOPPED | bOK++, bBad=0, fill=1/1 |
| 4 `CF_0V625` | ok=1 | FULL -> STOPPED | bOK++, bBad=0, fill=1/1 |

## Wiring UART PSoC ↔ ESP esclavo

```
PSoC Tx_1 = P1[5]  →  ESP GPIO16 (PSOC_UART_RX)
PSoC Rx_1 = P1[2]  ←  ESP GPIO17 (PSOC_UART_TX)
PSoC SYNC_IN       ←  ESP GPIO23 (SYNC_TO_PSOC_PIN)
GND común
```

Baud: 115200. El arranque del PSoC es siempre por flanco en SYNC_IN, nunca por UART.
