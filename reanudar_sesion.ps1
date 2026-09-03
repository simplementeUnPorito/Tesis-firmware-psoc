<#
    reanudar_sesion.ps1 - Reanuda la sesion de Claude Code que quedo a mitad de
    camino, para que el trabajo siga solo cuando se reponga la cuota.

    Lo dispara una tarea programada cada hora. Si la sesion ya termino el
    trabajo (existe el archivo TERMINADO), no hace nada y se va.

    Uso manual:  .\reanudar_sesion.ps1
#>

$ErrorActionPreference = 'Stop'

$Sesion   = '17391455-01f5-43a8-8b39-2717439e180c'
$Repo     = 'C:\Github\Tesis'
$LogDir   = Join-Path $env:LOCALAPPDATA 'claude_reanudar'
$Terminado = Join-Path $LogDir 'TERMINADO'
$Lock     = Join-Path $LogDir 'CORRIENDO'

New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

if (Test-Path $Terminado) {
    "[$(Get-Date -Format s)] ya estaba terminado; no hago nada" |
        Add-Content (Join-Path $LogDir 'reanudar.log')
    exit 0
}

# Candado: si una corrida anterior sigue viva, no encimar dos sesiones sobre el
# mismo repo. Se considera muerto un candado de mas de 3 horas.
if (Test-Path $Lock) {
    $edad = (Get-Date) - (Get-Item $Lock).LastWriteTime
    if ($edad.TotalHours -lt 3) {
        "[$(Get-Date -Format s)] hay otra corrida activa ($([int]$edad.TotalMinutes) min); salgo" |
            Add-Content (Join-Path $LogDir 'reanudar.log')
        exit 0
    }
    Remove-Item $Lock -Force
}

$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$log   = Join-Path $LogDir "reanudar_$stamp.log"
Set-Content -LiteralPath $Lock -Value $stamp

$instrucciones = @'
Reanudá el trabajo que quedó a medias. Contexto: estabas portando la
autocalibración del PSoC a IDAC8 con signo usando polarity_reg, sacando el
limitador de un paso por muestra, y borrando todo el código legacy (servo y
búsqueda binaria); el PI se queda.

Qué falta, en orden:
1. Terminar los parches en src/firmware/psoc/AcondicionamientoAnalogicoTest y
   que el proyecto COMPILE con program_psoc.ps1 -SelfTest (o cyprjmgr -build).
   Los scripts de parche están en el scratchpad de la sesión.
2. Arreglar program_psoc.ps1: la versión que graba las 4x256 filas deja el chip
   sin arrancar. La versión históricamente validada grababa sólo las filas que
   el HEX realmente ocupa.
3. Incorporar los Kp/Ki que haya dejado el simulador en
   src/calculos_modelados/python/calibracion_pi/RESULTADOS.md, si existe.
4. Si el KitProg (COM3) volvió a aparecer, grabar el PSoC y probar contra la
   placa con python -m testbench desde src/interfaces/python. Si no volvió,
   alcanza con que compile.
5. Validar varias veces, commitear todo, y actualizar el registro en
   slave/artifacts/registro_pruebas_analogicas_2026-09-02.md.

Cuando esté TODO terminado y validado, creá el archivo
%LOCALAPPDATA%\claude_reanudar\TERMINADO con un resumen adentro, y recién ahí
apagá la computadora con shutdown /s /t 60.

Si te queda trabajo sin terminar, NO crees ese archivo: la tarea programada te
va a volver a levantar dentro de una hora.
'@

Push-Location $Repo
try {
    & claude --resume $Sesion --dangerously-skip-permissions -p $instrucciones *>&1 |
        Tee-Object -FilePath $log
}
finally {
    Pop-Location
    Remove-Item $Lock -Force -ErrorAction SilentlyContinue
}

"[$(Get-Date -Format s)] corrida terminada, log en $log" |
    Add-Content (Join-Path $LogDir 'reanudar.log')
