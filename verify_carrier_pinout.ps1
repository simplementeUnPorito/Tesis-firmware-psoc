[CmdletBinding()]
param(
    [string]$ProjectDirectory = (
        Join-Path $PSScriptRoot 'AcondicionamientoAnalogico.cydsn'
    ),
    [switch]$SkipBuildStatus
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$fitterHeader = Join-Path $ProjectDirectory 'Generated_Source\PSoC5\cyfitter.h'
$dwrPath = Join-Path $ProjectDirectory 'AcondicionamientoAnalogico.cydwr'
$buildLog = Join-Path $ProjectDirectory 'BUILD.log'

foreach ($requiredPath in @($fitterHeader, $dwrPath)) {
    if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
        throw "No se encontró el archivo requerido: $requiredPath"
    }
}

# Guid is the active PSoC Creator pin-instance GUID. Some names have stale
# duplicate GUIDs in DWRInstGuidMapping; the active GUIDs below are the ones
# that the fitter reports as physical pins.
$expected = @(
    [pscustomobject]@{ Name = 'AMuxCapacitor'; Macro = 'AMuxCapacitor'; Pin = 'P1[4]'; Header = 'J1.21'; Guid = '6607d90a-4585-4268-9b0e-8e04a1da165e'; Bit = 0 },
    [pscustomobject]@{ Name = 'BPm';             Macro = 'BPm';             Pin = 'P3[3]'; Header = 'J2.22'; Guid = '11268be6-e82b-4a3c-a773-58d27b1e7e5e'; Bit = 0 },
    [pscustomobject]@{ Name = 'BPo';             Macro = 'BPo';             Pin = 'P3[7]'; Header = 'J2.18'; Guid = '4d3ad0a1-0ec3-4326-981d-69cc2ae37a01'; Bit = 0 },
    [pscustomobject]@{ Name = 'Button';          Macro = 'Button';          Pin = 'P12[4]'; Header = 'J1.12'; Guid = 'a490133f-45d0-4064-afd2-f9f8f39d9fc8'; Bit = 0 },
    [pscustomobject]@{ Name = 'INn';             Macro = 'INn';             Pin = 'P1[6]'; Header = 'J1.23'; Guid = 'd43cd46e-28b0-4f4f-8ddb-1abc5d80acdb'; Bit = 0 },
    [pscustomobject]@{ Name = 'INp';             Macro = 'INp';             Pin = 'P1[7]'; Header = 'J1.24'; Guid = 'e3b927a2-b845-409d-aa52-8c1fc1a669ec'; Bit = 0 },
    [pscustomobject]@{ Name = 'LED';             Macro = 'LED';             Pin = 'P1[2]'; Header = 'J1.19'; Guid = 'e851a3b9-efb8-48be-bbb8-b303b216c393'; Bit = 0 },
    [pscustomobject]@{ Name = 'LPm';             Macro = 'LPm';             Pin = 'P0[3]'; Header = 'J2.8';  Guid = 'a8e741cb-52da-4eb1-8013-3a9f7793e90a'; Bit = 0 },
    [pscustomobject]@{ Name = 'LPo';             Macro = 'LPo';             Pin = 'P0[1]'; Header = 'J2.10'; Guid = 'efdea9d7-dbde-4222-a75a-6960856938ef'; Bit = 0 },
    [pscustomobject]@{ Name = 'PGAo';            Macro = 'PGAo';            Pin = 'P3[4]'; Header = 'J2.21'; Guid = '5750fc06-1929-4f0b-8a77-2b5ceb6e0978'; Bit = 0 },
    [pscustomobject]@{ Name = 'Rx';              Macro = 'Rx';              Pin = 'P2[0]'; Header = 'J1.1';  Guid = '1425177d-0d0e-4468-8bcc-e638e5509a9b'; Bit = 0 },
    [pscustomobject]@{ Name = 'SEo';             Macro = 'SEo';             Pin = 'P2[7]'; Header = 'J1.8';  Guid = 'cc3bcd7e-5dc0-48ea-9bf6-6aa082be1ada'; Bit = 0 },
    [pscustomobject]@{ Name = 'SPIp_CS';         Macro = 'SPIp__0';         Pin = 'P2[3]'; Header = 'J1.4';  Guid = '89df988e-255f-4972-92c4-b9773fdbd3f6'; Bit = 0 },
    [pscustomobject]@{ Name = 'SPIp_SCK';        Macro = 'SPIp__1';         Pin = 'P2[4]'; Header = 'J1.5';  Guid = '89df988e-255f-4972-92c4-b9773fdbd3f6'; Bit = 1 },
    [pscustomobject]@{ Name = 'SPIp_MOSI';       Macro = 'SPIp__2';         Pin = 'P2[5]'; Header = 'J1.6';  Guid = '89df988e-255f-4972-92c4-b9773fdbd3f6'; Bit = 2 },
    [pscustomobject]@{ Name = 'SPIp_MISO';       Macro = 'SPIp__3';         Pin = 'P2[6]'; Header = 'J1.7';  Guid = '89df988e-255f-4972-92c4-b9773fdbd3f6'; Bit = 3 },
    [pscustomobject]@{ Name = 'SUMm';            Macro = 'SUMm';            Pin = 'P0[5]'; Header = 'J2.6';  Guid = '3fde1f9b-8262-4d24-a0cd-27c32cc9ef2a'; Bit = 0 },
    [pscustomobject]@{ Name = 'SUMo';            Macro = 'SUMo';            Pin = 'P0[0]'; Header = 'J2.11'; Guid = '0b740478-9e54-4120-9bf2-d02741c863a9'; Bit = 0 },
    [pscustomobject]@{ Name = 'SYNC_IN';         Macro = 'SYNC_IN';         Pin = 'P1[5]'; Header = 'J1.22'; Guid = '8d318d8b-cf7b-4b6b-b02c-ab1c5c49d0ba'; Bit = 0 },
    [pscustomobject]@{ Name = 'Tx';              Macro = 'Tx';              Pin = 'P12[7]'; Header = 'J1.9'; Guid = 'ed092b9b-d398-4703-be89-cebf998501f6'; Bit = 0 },
    [pscustomobject]@{ Name = 'Vref';            Macro = 'Vref';            Pin = 'P3[6]'; Header = 'J2.19'; Guid = '9a3f4733-40d7-4c3b-8187-c7cd10160c7a'; Bit = 0 },
    [pscustomobject]@{ Name = 'Vref_ADDER';      Macro = 'Vref_ADDER';      Pin = 'P0[7]'; Header = 'J2.4';  Guid = 'b7698a5a-9f16-4afe-84f5-368c95424baf'; Bit = 0 },
    [pscustomobject]@{ Name = 'Vref_BP';         Macro = 'Vref_BP';         Pin = 'P3[1]'; Header = 'J2.24'; Guid = '21660380-cb52-4656-a735-42dfba73e25f'; Bit = 0 },
    [pscustomobject]@{ Name = 'Vref_LP';         Macro = 'Vref_LP';         Pin = 'P0[6]'; Header = 'J2.5';  Guid = 'c8eed0ad-f4a0-4055-b31d-7dda03abcdb0'; Bit = 0 },
    [pscustomobject]@{ Name = 'Vref_PGA';        Macro = 'Vref_PGA';        Pin = 'P3[0]'; Header = 'J2.25'; Guid = '6dca8981-f62c-4f5a-a751-5b73bdd201af'; Bit = 0 }
)

$headerText = [System.IO.File]::ReadAllText($fitterHeader)

function Get-FittedPin {
    param([string]$Macro)

    $escapedMacro = [regex]::Escape($Macro)
    $portMatch = [regex]::Match(
        $headerText,
        "(?m)^#define\s+$escapedMacro`__PORT\s+(\d+)u\s*$"
    )
    $shiftMatch = [regex]::Match(
        $headerText,
        "(?m)^#define\s+$escapedMacro`__SHIFT\s+(\d+)u\s*$"
    )
    if (-not $portMatch.Success -or -not $shiftMatch.Success) {
        throw "No se encontraron PORT y SHIFT para $Macro en cyfitter.h"
    }
    return 'P{0}[{1}]' -f
        $portMatch.Groups[1].Value,
        $shiftMatch.Groups[1].Value
}

[xml]$dwr = [System.IO.File]::ReadAllText($dwrPath)
$namespace = [System.Xml.XmlNamespaceManager]::new($dwr.NameTable)
$namespace.AddNamespace('c', 'http://cypress.com/xsd/cydwr')
$pinGroup = $dwr.SelectSingleNode(
    '/c:DesignWideResources/c:Group[@key="Pin2"]',
    $namespace
)
if ($null -eq $pinGroup) {
    throw 'No se encontró el grupo Pin2 en el DWR.'
}

$results = foreach ($item in $expected) {
    $actualPin = Get-FittedPin -Macro $item.Macro
    $portFormat = $item.Pin.Replace('P', '').Replace('[', ',').Replace(']', '')
    $lockedNode = $pinGroup.SelectSingleNode(
        "c:Group[@key='$($item.Guid)']/c:Group[@key='$($item.Bit)']/c:Data[@key='Port Format']",
        $namespace
    )
    $unlockedNode = $pinGroup.SelectSingleNode(
        "c:Group[@key='Unlocked Pins']/c:Group[@key='$($item.Guid)']",
        $namespace
    )
    $unassignedNode = $pinGroup.SelectSingleNode(
        "c:Group[@key='UnAssigned Pins']/c:Group[@key='$($item.Guid)']",
        $namespace
    )

    $dwrLocked = (
        $null -ne $lockedNode -and
        $lockedNode.Value -eq $portFormat -and
        $null -eq $unlockedNode -and
        $null -eq $unassignedNode
    )

    [pscustomobject]@{
        Signal = $item.Name
        Expected = $item.Pin
        Actual = $actualPin
        Header = $item.Header
        FitterOK = ($actualPin -eq $item.Pin)
        DwrLocked = $dwrLocked
    }
}

$results | Format-Table -AutoSize

$failed = @($results | Where-Object {
        -not $_.FitterOK -or -not $_.DwrLocked
    })

if (-not $SkipBuildStatus) {
    if (-not (Test-Path -LiteralPath $buildLog -PathType Leaf)) {
        throw "No se encontró el log de compilación: $buildLog"
    }
    $buildText = [System.IO.File]::ReadAllText($buildLog)
    if ($buildText -notmatch 'Build Succeeded:') {
        $failed += [pscustomobject]@{
            Signal = 'BUILD'
            Expected = 'Succeeded'
            Actual = 'No success marker'
            Header = '-'
            FitterOK = $false
            DwrLocked = $false
        }
    }
}

if ($failed.Count -gt 0) {
    Write-Error "El pinout carrier no coincide o no está totalmente bloqueado ($($failed.Count) fallo(s))."
    exit 1
}

Write-Host 'OK: pinout carrier coincide con cyfitter.h, está bloqueado en el DWR y la última compilación fue exitosa.'
