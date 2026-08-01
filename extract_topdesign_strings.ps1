[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [string]$Path = (Join-Path $PSScriptRoot 'AcondicionamientoAnalogico.cydsn\TopDesign\TopDesign.cysch'),

    [string]$ContextFor,

    [ValidateRange(1, 100)]
    [int]$Context = 10,

    [switch]$All
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$resolvedPath = (Resolve-Path -LiteralPath $Path).Path
$bytes = [System.IO.File]::ReadAllBytes($resolvedPath)
$strings = [System.Collections.Generic.List[object]]::new()

for ($index = 0; $index -lt ($bytes.Length - 3);) {
    $start = $index
    $builder = [System.Text.StringBuilder]::new()

    while (
        ($index + 1) -lt $bytes.Length -and
        $bytes[$index] -ge 32 -and
        $bytes[$index] -le 126 -and
        $bytes[$index + 1] -eq 0
    ) {
        [void]$builder.Append([char]$bytes[$index])
        $index += 2
    }

    if ($builder.Length -ge 2) {
        $text = $builder.ToString()
        $kind = switch -Regex ($text) {
            '^[RCL]_\d+$' {
                'PassiveReference'
                break
            }
            '^(?:\d+(?:\.\d+)?)(?:[pnumkKM]?)(?:[Ff]|[oO]hm)?$' {
                'CandidateValue'
                break
            }
            '^(?:Resistor|Capacitor|Inductor|Potentiometer)_v\d+_\d+$' {
                'PassiveType'
                break
            }
            '^(?:INp|INn|SEo|BPm|BPo|SUMm|SUMo|LPm|LPo|PGAo|Vref(?:_.+)?|SYNC_IN|Rx|Tx)$' {
                'PhysicalNet'
                break
            }
            '^Shape_\d+$' {
                'Shape'
                break
            }
            default {
                'Text'
            }
        }

        $strings.Add([pscustomobject]@{
            Offset = $start
            HexOffset = ('0x{0:X}' -f $start)
            Kind = $kind
            Text = $text
        })
    }

    if ($index -eq $start) {
        $index++
    }
}

if ($PSBoundParameters.ContainsKey('ContextFor')) {
    $matches = for ($itemIndex = 0; $itemIndex -lt $strings.Count; $itemIndex++) {
        if ($strings[$itemIndex].Text -match $ContextFor) {
            $itemIndex
        }
    }

    foreach ($matchIndex in $matches) {
        $first = [Math]::Max(0, $matchIndex - $Context)
        $last = [Math]::Min($strings.Count - 1, $matchIndex + $Context)

        Write-Output ('--- match {0} at {1} ---' -f
            $strings[$matchIndex].Text,
            $strings[$matchIndex].HexOffset)
        $strings[$first..$last] |
            Select-Object HexOffset, Kind, Text |
            Format-Table -AutoSize
    }
    return
}

if ($All) {
    $strings | Select-Object HexOffset, Kind, Text
    return
}

$strings |
    Where-Object {
        $_.Kind -in @(
            'PassiveReference',
            'CandidateValue',
            'PassiveType',
            'PhysicalNet'
        )
    } |
    Select-Object HexOffset, Kind, Text

