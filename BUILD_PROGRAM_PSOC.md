# Build y programación PSoC 5LP

Flujo actual para `AcondicionamientoAnalogico.cydsn` en Windows con PSoC Creator 4.4.

## Grabado por PPCLI: VERIFICADO 2026-09-05

`program_psoc.ps1` **graba bien y el chip arranca**. Queda cerrado el tema, que
estuvo mal diagnosticado entre el 2026-09-02 y el 2026-09-04.

Evidencia de la corrida del 2026-09-05 (proyecto de autotest, `-SkipBuild`,
mismo HEX que ya estaba en el chip):

- `PSoC3_EraseAll` + 272 filas `ProgramRowFromHex`/`VerifyRowFromHex` con ECC
  (`0x01`), todas `0 OK` y `verResult = 1`;
- `ProtectAll`, `VerifyProtect`, `DAP_ReleaseChip`, `ClosePort` en `0 OK`;
- **después de grabar**, el PSoC contesta I2C: `taps` da los cuatro canales en
  1001,3 / 1001,5 / 1001,5 / 1000,8 mV, y el autotest completo repite el mismo
  veredicto que antes de grabar (22 PASS, 3 FAIL conocidos, 1 WARN, 83,9 s).

Lo que rompía era `-AllRows` (las 1024 filas), no la falta de PSoC Creator.

**Hipótesis descartada — NV latches.** Se sospechaba que al script le faltaba
escribir las NVL. No es eso:

- `PSoC3_EraseAll` no borra las NVL, así que las que dejó Creator siguen ahí;
- `PSoC3_GetEccStatus` sobre el chip devuelve `0`, coherente con el HEX
  (`nvlUserSize=4`, user `00 00 40 05`, WO `bc 90 ac af`) y con
  `CYDEV_ECC_ENABLE=False` del `.cydwr`;
- `PSoC3_ReadNvlArray` falla con `0x80004005` y por eso no se pueden comparar
  directamente, pero `PSoC3_WriteNvlArray` existe si alguna vez hiciera falta.

**Por qué alcanza con las filas ocupadas.** El `.cydwr` tiene
`CYDEV_CONFIGURATION_ECC=True` con `CYDEV_ECC_ENABLE=False`: la configuración
del fabric se guarda en el espacio ECC. Parseando el HEX, esa configuración es
no nula **sólo en las filas 0..150** (region `0x80000000`, 32 B por fila). El
código ocupa 264 filas y el script graba 272, así que cubre las dos cosas.

**Trampas prácticas.**

- El nombre del KitProg cambia entre sesiones: se vieron
  `KitProg (CMSIS-DAP/236111)` y `KitProg (CMSIS-DAP/246475)`. El script lo
  autodetecta; para forzarlo, `-Port`.
- PPCLI escribe ~1100 líneas por corrida: redirigir a archivo.
- `DAP_Acquire` + `DAP_ReleaseChip` sin grabar **no** perturba al PSoC: se
  comprobó que sigue contestando después.

## Flujo vigente (verificado 2026-09-01)

Usar los scripts del repositorio; las recetas manuales más abajo quedan como
historial de diagnóstico:

```powershell
Set-Location 'C:\Github\Tesis\src\firmware\psoc'

# Firmware normal de campo
.\program_psoc.ps1

# Firmware de autotest de placa
.\program_psoc.ps1 -SelfTest

# Reset después de volver a grabar el ESP32
.\reset_psoc.ps1
```

`program_psoc.ps1` compila, valida que sea el proyecto GEO+SPI, comprueba las
cuatro frecuencias ADC, detecta el KitProg y programa las **4 matrices × 256
filas con ECC**. Esto es obligatorio aunque el código use menos flash: la
configuración digital y el ruteo UDB del PSoC 5LP están en el espacio ECC. El
intento de programar solamente `0..lastRow` fue reproducido el 2026-09-01 y dejó
el PSoC sin arranque digital correcto.

El pinout efectivo se toma de `Generated_Source/PSoC5/cyfitter.h`, idéntico en
los proyectos normal y de autotest:

```text
ESP GPIO26 -> PSoC Rx P15[0]
ESP GPIO27 -> PSoC SYNC_IN P0[4]
PSoC I2C maestro -> ESP SDA GPIO21 / SCL GPIO22, dirección 0x42
```

> No copiar el ejemplo histórico que calcula `lastRow`: se conserva sólo para
> entender builds antiguos y no es un procedimiento válido de programación.

## Migración `cambios-hardware` (2026-08-03) — placa nueva

Esta migración se desarrolló en `cambios-hardware` y se integró a `main` tras
la puesta en marcha digital del 2026-09-01.
Build verificado: **Flash 60400 B / SRAM 51928 B**, `Build Succeeded`.

Cambios de TopDesign y lo que hubo que tocar en firmware:

| Cambio de hardware | Impacto en firmware |
|---|---|
| Los cuatro `VDAC_ref_*` pasan de VDAC8 a **IDAC8** (mismo nombre de instancia, por eso compilan) | `psoc_hw.h/.c`: globales `g_psoc_idac_rset_ohm` (30 kΩ), `g_psoc_idac_vref_uv` (2.048 V), `g_psoc_idac_fullscale_na` (31.875 µA) + helpers código↔tensión. `V = Vref + I·R`, LSB = 125 nA → **3.75 mV**, rango 2.048–3.004 V |
| **PGAout** agregado al pipeline GEO | comando `0xA8` (`PSOC_CMD_PGAOUT`), `psoc_hw_set_pgaout()`, y `psoc_hw_start_analog()` toma un tercer parámetro |
| UART del PSoC queda **solo RX** | toda la salida sale por `psoc_link_put_array()` (I2C maestro) en vez de `UART_PutArray()` |
| **I2C** nuevo: PSoC maestro, ESP esclavo en `0x42` | `psoc_link_*` en `psoc_hw.c`. La instancia del TopDesign se llama `I2C` (los `I2C_1.*` de `Generated_Source` son restos viejos; el `.cyprj` todavía los lista pero no se compilan) |
| Los pines de `SPIp` quedaron repartidos en varios puertos | el fitter dejó de emitir los macros agregados (`SPIp__DR`, `SPIp__BYP`, `SPIp__PRTDSI__*`); el CS de la SD ahora usa los del pin 0 |
| El pin **LED desapareció** del TopDesign | se quitó el `#include "LED.h"` de `main.c` para que los guards `CY_PINS_LED_H` hagan efecto; sin eso el link falla con `undefined reference to LED_Write` |

La SD con el ruteo nuevo de `SPIp` quedó validada en hardware el 2026-09-01:
FAT montado, escritura/lectura correctas, R1 `0x00`, los cuatro pads SPI
observaron niveles bajo y alto y los flags de error quedaron en cero.

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
Push-Location ".\AcondicionamientoAnalogico.cydsn"
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
$hex        = (Resolve-Path '.\AcondicionamientoAnalogico.cydsn\CortexM3\ARM_GCC_541\Debug\AcondicionamientoAnalogico.hex').Path.Replace('\', '/')
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
cd firmware/esp32/Nodo\ comunicación/slave
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

## Wiring PSoC ↔ ESP esclavo

En `cambios-hardware` el enlace es **asimétrico**:

```
ESP → PSoC : UART   ESP GPIO26 (PSOC_UART_TX) → PSoC Rx = P15[0]
PSoC → ESP : I2C    PSoC maestro (SCL/SDA) → ESP GPIO22/GPIO21, esclavo 0x42
Sync       :        ESP GPIO27 (SYNC_TO_PSOC_PIN) → PSoC SYNC_IN = P0[4]
GND común
```

`PSOC_I2C_ADDR` (esclavo, `platformio.ini`) tiene que coincidir con
`PSOC_LINK_I2C_ADDR` (`psoc_hw.h`). No hay entrada de sync por cable desde el
maestro: el arranque viaja por ESP-NOW y el esclavo genera el flanco.

Wiring anterior (rama `main`, UART bidireccional):

```
PSoC Tx = P12[7] / J1.9  →  ESP GPIO25 / J2.9  (PSOC_UART_RX)
PSoC Rx = P2[0]  / J1.1  ←  ESP GPIO26 / J2.10 (PSOC_UART_TX)
PSoC SYNC_IN = P1[5] / J1.22 ← ESP GPIO27 / J2.11
GND común
```

Baud: 115200. `Tx` del PSoC está configurado como open-drain y en la carrier
lleva un pull-up de 4.7 kΩ a los 3.3 V del ESP32; no elevar esa línea a 5 V.
El arranque del PSoC es siempre por flanco en `SYNC_IN`, nunca por UART. La
entrada de sincronismo externo del ESP32 quedó separada en GPIO32.
