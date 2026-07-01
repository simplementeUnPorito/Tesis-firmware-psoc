# Build y programación PSoC 5LP

Flujo actual para `AcondicionamientoAnalogico.cydsn` en Windows con PSoC Creator 4.4.

## Estado del build actual (2026-07-01)

- Flash: **31086 bytes** → filas a programar: **0..121**  
  (`ceil(31086/256) - 1 = 121`)
- SRAM: 49312 bytes
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

## Programar (PPCLI)

```powershell
$timestamp  = Get-Date -Format 'yyyyMMdd_HHmmss'
$log        = Join-Path $env:TEMP "psoc_program_acondicionamiento_$timestamp.log"
$hex        = 'C:/Github/Tesis/src/psoc/AcondicionamientoAnalogico.cydsn/CortexM3/ARM_GCC_541/Debug/AcondicionamientoAnalogico.hex'
$port       = 'KitProg (CMSIS-DAP/236111)'   # reemplazar con resultado exacto de GetPorts
$programmer = 'C:\Program Files (x86)\Cypress\Programmer\'
$lastRow    = 121                             # actualizar según el build actual

$cmds = New-Object System.Collections.Generic.List[string]
$cmds.Add(('OpenPort "{0}" "{1}"' -f $port, $programmer))
$cmds.Add('SetAcquireMode Reset')
$cmds.Add('SetProtocol 8')
$cmds.Add('SetProtocolConnector 1')
$cmds.Add('SetProtocolClock 192')
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

$inputText = ($cmds -join [Environment]::NewLine) + [Environment]::NewLine
$inputText | & 'C:\Program Files (x86)\Cypress\Programmer\PPCLI.exe' |
  Tee-Object -FilePath $log
```

El programado exitoso termina con `0 OK` en cada fila y después:
```
PSoC3_ProtectAll → 0 OK
PSoC3_VerifyProtect → 0 OK
DAP_ReleaseChip → 0 OK
ClosePort → 0 OK
```

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
El PSoC parpadea ~8 s a 2.5 Hz (no bloqueante, servido por Timer ILO 100 kHz)
y vuelve a encendido fijo.

## Wiring UART PSoC ↔ ESP esclavo

```
PSoC Tx_1 = P1[5]  →  ESP GPIO16 (PSOC_UART_RX)
PSoC Rx_1 = P1[2]  ←  ESP GPIO17 (PSOC_UART_TX)
PSoC SYNC_IN       ←  ESP GPIO23 (SYNC_TO_PSOC_PIN)
GND común
```

Baud: 115200. El arranque del PSoC es siempre por flanco en SYNC_IN, nunca por UART.
