[CmdletBinding()]
param(
    [string]$Port,
    [switch]$SkipBuild,
    [switch]$SelfTest,
    # Graba las 1024 filas como hacia la version que rompio. Solo para probar.
    [switch]$AllRows,
    [ValidateRange(1, 100000)]
    [int]$ExpectedSampleRate = 2604
)

$ErrorActionPreference = 'Stop'

$ProjectDir = if ($SelfTest) {
    Join-Path $PSScriptRoot 'AcondicionamientoAnalogicoTest\AcondicionamientoAnalogico.cydsn'
} else {
    Join-Path $PSScriptRoot 'AcondicionamientoAnalogico.cydsn'
}
$firmwareKind = if ($SelfTest) { 'GEO+AUTOTEST' } else { 'GEO' }
$Workspace = Join-Path $ProjectDir 'AcondicionamientoAnalogico.cywrk'
$Hex = Join-Path $ProjectDir 'CortexM3\ARM_GCC_541\Debug\AcondicionamientoAnalogico.hex'
$BuildLog = Join-Path $ProjectDir 'BUILD.log'
$RebuildLog = Join-Path $ProjectDir 'REBUILD.log'
$Creator = 'C:\Program Files (x86)\Cypress\PSoC Creator\4.4\PSoC Creator\bin\cyprjmgr.exe'
$ProgrammerDir = 'C:\Program Files (x86)\Cypress\Programmer'
$Ppcli = Join-Path $ProgrammerDir 'ppcli.exe'
$GeneratedAdc = Join-Path $ProjectDir 'Generated_Source\PSoC5\ADC.h'
$GeneratedProject = Join-Path $ProjectDir 'Generated_Source\PSoC5\project.h'
$AdcParams = Join-Path $ProjectDir 'adc_2604.params'

foreach ($required in @($Workspace, $Creator, $Ppcli, $AdcParams)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "No se encontró: $required"
    }
}
if ($SelfTest -and -not (Test-Path -LiteralPath (Join-Path $ProjectDir 'psoc_selftest.h'))) {
    throw "El proyecto seleccionado no contiene el protocolo de autotest: $ProjectDir"
}

if (-not $SkipBuild) {
    Write-Host "[PSoC] Compilando proyecto $firmwareKind..."
    Push-Location $ProjectDir
    try {
        # -m vuelve determinista el customizer: no depende del último valor
        # guardado por la GUI y fuerza 2604 Hz en las cuatro configuraciones.
        $buildOutput = & $Creator -wrk (Split-Path -Leaf $Workspace) -rebuild -m (Split-Path -Leaf $AdcParams) 2>&1
        $buildExit = $LASTEXITCODE
        $buildOutput | Write-Host
    }
    finally {
        Pop-Location
    }
    if ($buildExit -ne 0 -or ($buildOutput -join "`n") -notmatch '(?:Build|Rebuild) Succeeded') {
        throw "Falló el build PSoC (exit=$buildExit)."
    }
}

foreach ($required in @($Hex, $GeneratedAdc, $GeneratedProject)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Artefacto de build ausente: $required"
    }
}

# Gate de identidad: nunca grabar por accidente un HEX HAMMER en la placa GEO.
$projectHeader = Get-Content -Raw -LiteralPath $GeneratedProject
if ($projectHeader -notmatch '#include\s+"PGAgain\.h"' -or
    $projectHeader -notmatch '#include\s+"SPI\.h"') {
    throw 'El build no es GEO+SPI (faltan PGAgain o SPI en project.h).'
}

# Las cuatro configuraciones deben compartir exactamente la Fs pedida.
$adcHeader = Get-Content -Raw -LiteralPath $GeneratedAdc
$rates = [regex]::Matches(
    $adcHeader,
    '(?m)^#define\s+ADC_CF_(?:2V5|0V512|1V024|0V625)_SRATE\s+\((\d+)u'
) | ForEach-Object { [int]$_.Groups[1].Value }
if ($rates.Count -ne 4 -or ($rates | Where-Object { $_ -ne $ExpectedSampleRate })) {
    throw "Fs ADC inválida. Esperada 4x$ExpectedSampleRate; encontrada: $($rates -join ',')."
}

$buildText = if (-not $SkipBuild -and $null -ne $buildOutput) {
    $buildOutput -join "`n"
} else {
    # Creator usa BUILD.log o REBUILD.log según la operación. Elegir siempre
    # el más reciente y exigir que corresponda al HEX evita programar solo una
    # parte de un firmware nuevo por haber leído un BUILD.log histórico.
    $logCandidates = @($BuildLog, $RebuildLog) |
        Where-Object { Test-Path -LiteralPath $_ } |
        ForEach-Object { Get-Item -LiteralPath $_ } |
        Sort-Object LastWriteTimeUtc -Descending
    if ($logCandidates.Count -eq 0) {
        throw 'No existe BUILD.log ni REBUILD.log; ejecute sin -SkipBuild.'
    }
    $latestBuildLog = $logCandidates[0]
    $hexInfo = Get-Item -LiteralPath $Hex
    if ($latestBuildLog.LastWriteTimeUtc -lt $hexInfo.LastWriteTimeUtc) {
        throw "Log de build anterior al HEX ($($latestBuildLog.FullName)); ejecute sin -SkipBuild."
    }
    Get-Content -Raw -LiteralPath $latestBuildLog.FullName
}
$flashMatches = [regex]::Matches($buildText, 'Flash used:\s*(\d+)\s+of')
if ($flashMatches.Count -eq 0) {
    throw "No se pudo extraer 'Flash used' de la salida/log de build."
}
$flashUsed = [int]$flashMatches[$flashMatches.Count - 1].Groups[1].Value
if ($flashUsed -lt 1 -or $flashUsed -gt 262144) {
    throw "Uso de flash fuera de rango para CY8C5888LTI-LP097: $flashUsed bytes."
}

# Cuántas filas grabar.
#
# HISTORIA, porque esto ya rompió una vez: la versión que grababa las cuatro
# matrices completas (4x256 = 1024 filas) verificaba todas las filas con 0 OK,
# cerraba con ProtectAll/VerifyProtect en 0 OK... y dejaba el chip SIN ARRANCAR.
# Cero flancos en SCL, silencio total en el I2C de subida, y ni dos ToggleReset
# lo recuperaban; regrabando el mismo proyecto desde PSoC Creator volvía al
# instante. Medido el 2026-09-02.
#
# El flujo que sí está validado contra hardware (2026-07-07, log en
# BUILD_PROGRAM_PSOC.md) grababa solamente las filas que el HEX ocupa. Eso es
# lo que se hace acá: se derivan de 'Flash used' con un margen, y no se tocan
# las filas que el firmware no usa.
#
# En PSoC 5LP la configuración UDB/ruteo vive en el espacio ECC de las filas de
# Flash, así que hay que grabar ECC (opción 0x01) en todas las filas ocupadas;
# de ahí viene la confusión que llevó a grabarlas todas. Grabar ECC de las
# filas ocupadas sí hace falta; grabar las filas vacías no.
#
# -AllRows fuerza el comportamiento viejo, solo para experimentar.
$rowsPerArray = 256
$rowBytes = 256
$rowsNeeded = [Math]::Ceiling($flashUsed / $rowBytes) + 8   # margen de 8 filas
if ($AllRows) {
    $rowsNeeded = 4 * $rowsPerArray
    Write-Warning "-AllRows: se graban las 1024 filas. Esto dejó el chip sin arrancar el 2026-09-02."
}
if ($rowsNeeded -gt (4 * $rowsPerArray)) { $rowsNeeded = 4 * $rowsPerArray }
$lastArray = [Math]::Floor(($rowsNeeded - 1) / $rowsPerArray)
$flashArrays = 0..$lastArray

if ([string]::IsNullOrWhiteSpace($Port)) {
    $portsScript = Join-Path $env:TEMP 'psoc_getports_codex.cli'
    @('GetPorts', 'quit') | Set-Content -LiteralPath $portsScript -Encoding ASCII
    Push-Location $ProgrammerDir
    try {
        $portsOutput = & $Ppcli "--runfile $($portsScript -replace '\\','/')" 2>&1
    }
    finally {
        Pop-Location
    }
    # Programmer 3.x devuelve, segun el firmware del kit, tanto
    # "KitProg/1D1F..." como "KitProg (CMSIS-DAP/236111)". Conservar el
    # nombre literal entre los marcadores de PPCLI porque OpenPort lo exige.
    $ports = @($portsOutput | ForEach-Object {
        $line = "$_".Trim()
        if ($line -match '^<\s*(KitProg[^>]*)>?\s*$') {
            $Matches[1].Trim()
        }
    } | Sort-Object -Unique)
    if ($ports.Count -ne 1) {
        throw "Se esperaba un único KitProg; detectados: $($ports -join ', '). Use -Port."
    }
    $Port = $ports[0]
}

$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$programScript = Join-Path $env:TEMP "psoc_program_geo_2604_$stamp.cli"
$programLog = Join-Path $env:TEMP "psoc_program_geo_2604_$stamp.log"
$hexPosix = $Hex -replace '\\', '/'
$programmerPosix = ($ProgrammerDir + '\') -replace '\\', '/'

$commands = [System.Collections.Generic.List[string]]::new()
$commands.Add(('OpenPort "{0}" "{1}"' -f $Port, $programmerPosix))
$commands.Add('SetAcquireMode Reset')
$commands.Add('SetProtocol 8')
$commands.Add('SetProtocolConnector 1')
$commands.Add('SetProtocolClock 152')
$commands.Add(('HEX_ReadFile "{0}"' -f $hexPosix))
$commands.Add('DAP_Acquire')
$commands.Add('PSoC3_GetJtagID')
$commands.Add('PSoC3_EraseAll')
$rowsWritten = 0
foreach ($array in $flashArrays) {
    $inThisArray = [Math]::Min($rowsPerArray, $rowsNeeded - ($array * $rowsPerArray))
    0..($inThisArray - 1) | ForEach-Object {
        $commands.Add(('PSoC3_ProgramRowFromHex 0x{0:X2} {1} 0x01' -f $array, $_))
        $commands.Add(('PSoC3_VerifyRowFromHex 0x{0:X2} {1} 0x01' -f $array, $_))
    }
    $rowsWritten += $inThisArray
}
$commands.Add('PSoC3_ProtectAll')
$commands.Add('PSoC3_VerifyProtect')
$commands.Add('DAP_ReleaseChip')
$commands.Add('ClosePort')
$commands.Add('quit')
$commands | Set-Content -LiteralPath $programScript -Encoding ASCII

Write-Host "[PSoC] Programando $firmwareKind en ${Port}: $flashUsed bytes usados, $rowsWritten filas con ECC/config (matrices 0..$lastArray), Fs 4x$ExpectedSampleRate Hz..."
Push-Location $ProgrammerDir
try {
    $programOutput = & $Ppcli "--runfile $($programScript -replace '\\','/')" 2>&1
    $programExit = $LASTEXITCODE
    $programOutput | Tee-Object -FilePath $programLog | Write-Host
}
finally {
    Pop-Location
}

$programText = $programOutput -join "`n"
if ($programExit -ne 0 -or $programText -match '(?im)^\s*[1-9]\d*\s+(?:ERROR|FAIL)') {
    throw "Falló PPCLI (exit=$programExit). Log: $programLog"
}
foreach ($requiredSuccess in @('PSoC3_ProtectAll', 'PSoC3_VerifyProtect', 'DAP_ReleaseChip', 'ClosePort')) {
    if ($programText -notmatch [regex]::Escape($requiredSuccess)) {
        throw "PPCLI no ejecutó $requiredSuccess. Log: $programLog"
    }
}

Write-Host "[PSoC] OK: $firmwareKind+SPI, Fs=$ExpectedSampleRate Hz, $rowsWritten filas verificadas."
Write-Host "[PSoC] Log: $programLog"
