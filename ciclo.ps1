<#
    ciclo.ps1 - compila y/o graba el PSoC sin vomitar el log.

    El build de PSoC Creator y ppcli escupen miles de lineas ("ProgramRow...
    VerifyRow..." por cada una de las 223 filas). Esto manda todo a un archivo y
    devuelve UNA linea de veredicto, que es lo unico que se mira.

    Uso:  .\ciclo.ps1            compila y graba
          .\ciclo.ps1 -SoloBuild compila nada mas
          .\ciclo.ps1 -SoloGrabar graba el hex que ya esta
#>
[CmdletBinding()]
param([switch]$SoloBuild, [switch]$SoloGrabar)

$ErrorActionPreference = 'Continue'
$raiz = $PSScriptRoot
$proy = Join-Path $raiz 'AcondicionamientoAnalogico.cydsn'
$creator = 'C:\Program Files (x86)\Cypress\PSoC Creator\4.4\PSoC Creator\bin\cyprjmgr.exe'
$log = Join-Path $env:TEMP 'psoc_ciclo.log'

if (-not $SoloGrabar) {
    Push-Location $proy
    try { & $creator -wrk 'AcondicionamientoAnalogico.cywrk' -build *> $log }
    finally { Pop-Location }
    $ok = Select-String -Path $log -Pattern 'Build Succeeded' -Quiet
    if (-not $ok) {
        Write-Host '[BUILD] FALLO:'
        Select-String -Path $log -Pattern 'error' | Select-Object -First 5 |
            ForEach-Object { Write-Host "  $($_.Line.Trim())" }
        exit 1
    }
    Write-Host '[BUILD] ok'
    if ($SoloBuild) { exit 0 }
}

& (Join-Path $raiz '..\..\..\src\firmware\psoc\program_psoc.ps1') -SkipBuild *> $log 2>&1
$linea = Select-String -Path $log -Pattern '^\[PSoC\] OK' | Select-Object -Last 1
if ($linea) { Write-Host "[GRABAR] $($linea.Line.Trim())" }
else {
    Write-Host '[GRABAR] FALLO:'
    Select-String -Path $log -Pattern 'E$|error|Error' | Select-Object -First 5 |
        ForEach-Object { Write-Host "  $($_.Line.Trim())" }
    exit 1
}
