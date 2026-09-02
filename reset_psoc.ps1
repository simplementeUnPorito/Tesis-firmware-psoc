<#
    reset_psoc.ps1 - ToggleReset al PSoC por el KitProg.

    Hace falta SIEMPRE despues de reflashear el ESP32: el reflasheo del ESP
    cuelga al PSoC, y sin este reset el PSoC no vuelve a contestar.

    Detecta el KitProg solo con GetPorts, asi que no hay que andar copiando el
    numero de serie a mano (cambia entre kits, y el formato tambien: puede ser
    "KitProg/1D1F..." o "KitProg (CMSIS-DAP/...)").

    Uso:   .\reset_psoc.ps1
#>

$ErrorActionPreference = 'Stop'
$prog = 'C:\Program Files (x86)\Cypress\Programmer'

if (-not (Test-Path $prog)) {
    Write-Error "No esta PSoC Programmer en $prog"
}

# --- 1) Averiguar el nombre exacto del puerto ------------------------------
# GetPorts directo entra en modo interactivo y se queda esperando; hay que
# pasarle un runfile que termine en 'quit'.
$lsScript = Join-Path $env:TEMP 'psoc_getports.cli'
@('GetPorts', 'quit') | Set-Content -LiteralPath $lsScript -Encoding ASCII

Push-Location $prog
try {
    $salida = & .\ppcli.exe "--runfile $($lsScript -replace '\\','/')" 2>&1
} finally {
    Pop-Location
}

$puerto = $null
foreach ($linea in $salida) {
    $t = "$linea".Trim().TrimStart('<').TrimEnd('>').Trim()
    if ($t -like 'KitProg*') { $puerto = $t; break }
}

if (-not $puerto) {
    Write-Host "Salida de GetPorts:" -ForegroundColor Yellow
    $salida | ForEach-Object { Write-Host "  $_" }
    Write-Error "No aparece ningun KitProg. Revisar que el CY8CKIT-059 este enchufado por USB."
}

Write-Host "KitProg: $puerto" -ForegroundColor Cyan

# --- 2) ToggleReset --------------------------------------------------------
$rstScript = Join-Path $env:TEMP 'psoc_reset.cli'
@(
    ('OpenPort "{0}" "{1}\"' -f $puerto, $prog),
    'SetProtocol 8',
    'SetProtocolClock 152',
    'SetProtocolConnector 1',
    'ToggleReset 0 100',
    'ClosePort',
    'quit'
) | Set-Content -LiteralPath $rstScript -Encoding ASCII

Push-Location $prog
try {
    $r = & .\ppcli.exe "--runfile $($rstScript -replace '\\','/')" 2>&1
} finally {
    Pop-Location
}

$r | ForEach-Object { Write-Host "  $_" }

if (($r -join "`n") -match '(?ms)ToggleReset.*?\b0 OK\b') {
    Write-Host ""
    Write-Host "PSoC reseteado." -ForegroundColor Green
    Write-Host "Esperar ~20 s si el firmware corre auto-calibracion al boot." -ForegroundColor Green
} else {
    Write-Warning "No se vio un '0 OK' claro. Revisar la salida de arriba."
}
