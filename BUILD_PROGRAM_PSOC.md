# Build y programación PSoC 5LP

Flujo actual para `AcondicionamientoAnalogico.cydsn` en Windows con PSoC Creator 4.4.

## Estado del build actual (2026-07-02, politica de eventos determinismo-primero)

- Flash: **40758 bytes** -> filas a programar: **0..159**  
  (`ceil(40758/256) - 1 = 159`)
- SRAM: 49352 bytes
- KitProg observado: `KitProg (CMSIS-DAP/236111)` — **siempre correr `GetPorts` antes de programar** porque el nombre puede variar.

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

- Build: 2026-07-02 17:13:43, `Build Succeeded` (TC de timers sin IRQ ni
  sticky; ver `docs/psoc_supermaquina_handoff.md`, seccion "Politica de
  eventos determinismo-primero").
- Flash/SRAM: 40758 / 49352 bytes.
- Filas: `0..159`, ECC option `0x01`.
- Log: `C:\Users\elias\AppData\Local\Temp\psoc_program_eventpolicy_20260702_171417.log`.
- Script: `C:\Users\elias\AppData\Local\Temp\psoc_program_eventpolicy_20260702_171417.cli`.

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

## Wiring UART PSoC ↔ ESP esclavo

```
PSoC Tx_1 = P1[5]  →  ESP GPIO16 (PSOC_UART_RX)
PSoC Rx_1 = P1[2]  ←  ESP GPIO17 (PSOC_UART_TX)
PSoC SYNC_IN       ←  ESP GPIO23 (SYNC_TO_PSOC_PIN)
GND común
```

Baud: 115200. El arranque del PSoC es siempre por flanco en SYNC_IN, nunca por UART.
