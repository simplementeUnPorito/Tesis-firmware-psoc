<#
    reanudar_sesion.ps1 - Reanuda la sesion de Claude Code para que el trabajo
    siga solo cuando se reponga la cuota de tokens.

    Lo dispara una tarea programada cada hora durante la noche. Si la sesion ya
    declaro el trabajo terminado (existe el archivo TERMINADO), no hace nada y
    se va.

    NO apaga la computadora: la consigna de esta noche es avanzar sostenido
    hasta la manana.

    Uso manual:  .\reanudar_sesion.ps1
#>

$ErrorActionPreference = 'Stop'

$Sesion    = '17391455-01f5-43a8-8b39-2717439e180c'
$Repo      = 'C:\Github\Tesis'
$LogDir    = Join-Path $env:LOCALAPPDATA 'claude_reanudar'
$Terminado = Join-Path $LogDir 'TERMINADO'
$Lock      = Join-Path $LogDir 'CORRIENDO'

New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

if (Test-Path $Terminado) {
    "[$(Get-Date -Format s)] ya estaba terminado; no hago nada" |
        Add-Content (Join-Path $LogDir 'reanudar.log')
    exit 0
}

# Candado: si una corrida anterior sigue viva, no encimar dos sesiones sobre el
# mismo repo ni sobre los mismos puertos serie. Se considera muerto un candado
# de mas de 3 horas.
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
Segui el trabajo de la noche. NO preguntes nada, no hay nadie mirando: el
usuario se fue a dormir y vuelve a las 8 de la manana.

ANTES DE TOCAR NADA, leer C:\Github\Tesis\docs\HANDOFF_NOCHE_2026-09-03.md.
Ahi esta el estado, el plan ordenado, las trampas que cuestan horas, y tres
conclusiones viejas que resultaron falsas. Ese archivo es la fuente de verdad;
si algo de lo que recordas lo contradice, gana el archivo. Actualizalo a medida
que avanzas: es lo que el usuario va a leer a las 8am.

REGLA DE TOKENS, es la mas importante: delegar a codex TODO lo que sea pesado y
de poco juicio. Compilar, grabar, correr baterias largas, barridos, repeticiones,
buscar en archivos grandes. Se invoca asi:

  codex exec --dangerously-bypass-approvals-and-sandbox "<tarea>" < /dev/null

Pedirle SIEMPRE un informe corto y acotado ("maximo 10 lineas, en espanol, solo
errores textuales, no pegues la salida completa"), porque lo que devuelve entra
en el contexto. Vos analizas los casos y decidis; codex ejecuta. Avance suave y
sostenido, sin apuro: tiene que durar toda la noche.

HARDWARE CONECTADO (estado al 2026-09-03 17:10):
  COM8  ESP32 esclavo nodo 2 con firmware de CAMPO slave2
  COM6  ESP32 maestro con firmware y LittleFS nuevos
  PSoC  firmware GEO de campo con polarity_reg; el KitProg dejo de enumerar
        despues de las cargas, pero el PSoC esta vivo y enlazado con COM8

YA ESTA HECHO; NO REPETIR:
- Port PSoC de campo, TopDesign polarity_reg, build y programacion.
- Port 0xAA firmado completo: USB master, ESP-NOW, ESP slave, UART PSoC, web y
  helper Python. Commits y evidencia exacta estan en el HANDOFF.
- Builds master, slave1/2/3 y slaveTest; cargas fisicas master/slave2/LittleFS.

QUE FALTA, en orden (el detalle y los limites de evidencia estan en HANDOFF):
1. Continuar el pipeline de CAPTURA/INGESTA, no el de configuracion 0xAA:
   PSoC -> slave2 -> master/pagina y luego inyeccion con el server de Python.
   Los datos ingeridos van a una carpeta lab/ para no mezclarlos con datos
   buenos. Restaurar cualquier ajuste manual de banco al terminar.
2. Confirmar con una medida electrica independiente el sentido de polarity_reg
   sobre LPo. El ACK 0xAA ya esta probado, pero el snapshot ADC de campo no dio
   una comparacion estable y no alcanza para afirmar el sentido fisico.
3. Revisar la captura/asentamiento de la configuracion ADC 2 y re-caracterizar
   topes sin el limitador de +-127. No tocar retries a ciegas: el transporte de
   configuracion dio 15/15 ACK en la prueba documentada.
4. Si vuelve el KitProg, registrar que reaparecio; no reprogramar sin necesidad.

Commitear seguido, con mensajes que expliquen POR QUE. Al final de cada ronda,
dejar el HANDOFF actualizado. NO apagues la computadora y NO crees el archivo
TERMINADO salvo que de verdad no quede nada por hacer. Si una prueba fisica
requiere DMM/osciloscopio o intervencion humana, documentarla como pendiente y
seguir con todo lo que si pueda hacerse sin usuario.
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
