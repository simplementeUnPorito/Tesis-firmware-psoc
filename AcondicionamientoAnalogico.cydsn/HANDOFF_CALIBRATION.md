# HANDOFF: Auto-calibración analógica PSoC (PID + media móvil) — AcondicionamientoAnalogico.cydsn

Sesión nocturna de revisión de código **sin hardware** (no se sube nada a la placa).
Objetivo: dejar todo lo verificable por software listo y documentado para la sesión
de bring-up con placa real. Escrito para que un agente fresco (Codex, Claude, o vos
mañana) retome sin releer el hilo completo.

Branch: `feature/master-web-ui`. Archivos nuevos sin trackear (`??` en git status):
`calibration.c/.h`, `calibration_tables.h`, `psoc_hw.c/.h`, `psoc_adc.c/.h`.
`main.c` modificado (M). Memoria persistente del agente: `[[project_psoc_calibration]]`
(índice de alto nivel; este doc es el detalle vivo).

---

> ## ⚠️ LEER PRIMERO (2026-06-14): reescritura completa del motor de calibración
>
> Las secciones §0-§12 de este documento describen una arquitectura que **ya no
> existe en el código**: ADC con dos configs (CFG1 continuo / CFG2 single-sample,
> con `psoc_adc_select_calibration_config()`/`_select_capture_config()` para
> alternar entre ellas), y un algoritmo PID + media móvil (`CAL_PID_KP/KI/KD_Q12`,
> `CAL_DAC_INIT=128`, `CAL_MAX_ITER=90`, `tools/cal_sim.py`). Todo eso fue removido
> en dos sesiones consecutivas (2026-06-13 y 2026-06-14). §0-§12 quedan como
> **registro histórico** de la investigación (por si algo de esa arquitectura
> vuelve a discutirse), pero **no describen el firmware actual**.
>
> **La arquitectura actual (un solo ADC config continuo + calibración por
> búsqueda binaria desde 2.5V) está en §13, al final de este documento — empezar
> por ahí.**

---

## 0. Objetivo de la migración (contexto)

Los operacionales internos del PSoC tienen offsets/derivas. En vez de que el usuario
ajuste VDAC/PGAvdac a mano desde la web, el PSoC se auto-calibra al boot (y on-demand
con el botón "Calibrar"): pone la entrada a tierra (`AMux_IN=1`), y para cada etapa
analógica (de la entrada hacia la salida) usa un PID (Q12) + media móvil sobre el ADC
para ajustar el VDAC de referencia de esa etapa hasta un target en cuentas del ADC.

---

## 1. Arquitectura — archivos y tablas (verificado línea por línea)

- `calibration.h` / `calibration_tables.h` / `calibration.c` — motor genérico +
  tablas por etapa (`PsocCalStage[]`).
- `psoc_hw.h/.c` — detección Geo/Hammer (`PSOC_HW_CLASS`) + arranque analógico +
  opcodes UART (incluye `PSOC_CMD_CALIBRATE = 0xB5u`).
- `psoc_adc.h/.c` — switch ADC Config1 (captura continua) ↔ Config2 (single-sample
  para calibración).
- `main.c` — boot: `psoc_hw_start_analog()` → `psoc_calibration_start_references()`
  → `psoc_adc_select_capture_config()` → arranca ISRs (`isr_DelSig_StartEx`,
  `isr_Timer_StartEx`, `isr_SyncIn_StartEx`) → `Timer_Start()` →
  `CyDelay(PSOC_STARTUP_CAL_DELAY_MS=5000)` → `psoc_run_calibration_if_idle()`
  (main.c:564-588). `g_state` arranca en `PSOC_IDLE` (main.c:92), así que la
  calibración de boot **siempre corre** (nada la deja en otro estado antes).
- Comando on-demand: `0xB5` → `psoc_run_calibration_if_idle()` (main.c:333-339,
  455-458) → `psoc_calibration_run_blocking()` → `CFG_ACK`.
- Stages Geo (4, capture channel final=3): GEO_PGA→GEO_BP→GEO_ADDER→GEO_LP,
  `VDAC_ref_PGA/BP/Adder/LP` (calibration_tables.h:90-95).
- Stages Hammer (3, capture channel final=2): HAMMER_IN→HAMMER_PGA→HAMMER_LP,
  `VDAC_ref_IN/PGA/LP` (calibration_tables.h:122-126).
- PID Q12: `KP=4, KI=1, KD=0`, `CAL_AVG_N=32`, `CAL_MAX_ITER=90`,
  `CAL_TOL_COUNTS=250`, `CAL_DAC_INIT=128`, `CAL_INTEGRAL_LIMIT=±400000`.
- Salvaguardas: corre solo si `g_state==PSOC_IDLE`; `isr_DelSig`/`isr_SyncIn` se
  deshabilitan durante calibración (ver §6); `best_dac` se conserva si no converge.

Esto coincide con lo documentado en memoria — sin cambios respecto a lo que ya se
sabía. Las secciones siguientes son **hallazgos nuevos de esta sesión**.

---

## 2. HALLAZGO #1 (arquitectural, importante): `PSOC_HW_CLASS` resuelve SIEMPRE a GEO en este `Generated_Source`

Evidencia (grep directo a `Generated_Source/PSoC5/`):

```
PGAgain.h:101:  #define PGAgain_DEFAULT_GAIN  (8u)   <- presente
PGA.h:101:      #define PGA_DEFAULT_GAIN      (3u)   <- TAMBIÉN presente
```

`psoc_hw.h:9-26`:
```c
#if defined(PGAgain_DEFAULT_GAIN)
    #define PSOC_HW_CLASS PSOC_HW_GEO
#elif defined(PGA_DEFAULT_GAIN)
    #define PSOC_HW_CLASS PSOC_HW_HAMMER
#else
    #error "..."
#endif
```

Como `PGAgain_DEFAULT_GAIN` está definido, el `#if` matchea primero →
**`PSOC_HW_CLASS == PSOC_HW_GEO` siempre**, para este `Generated_Source`. El `#elif`
HAMMER nunca se evalúa.

Además, el TopDesign que generó este árbol tiene **ambos juegos de componentes
colocados simultáneamente**:
- Geo: `PGAgain`, `VDAC_ref_PGA/BP/Adder/LP`, `OPAref`, `PGAp`, `PGAn`, `OPAbp`,
  `OPAadder`, `OPAlp`, `LPF_1`, `LPF_2`
- Hammer: `PGA`, `VDAC_ref_IN/PGA/LP`, `LPF_ref`, `LPF_ADC`, `Opa_ref_IN`,
  `Opa_ref_PGA`, `Opa_LP`

Solo existe **un** `.cydsn` (`AcondicionamientoAnalogico.cydsn`) — no hay un proyecto
"AnalogHammer" separado en el repo (`src/psoc/*.cydsn` listado completo, sin nada
parecido).

**Implicación**: tal como está hoy, compilar este proyecto en PSoC Creator produce
**siempre el firmware "Geo"**. La rama `#else`/HAMMER de `psoc_hw.c` y
`calibration_tables.h` es sintácticamente válida y sus símbolos VDAC existen (§3),
pero es **código muerto** en este `Generated_Source` — nunca se compila activamente
porque el `#elif` no se alcanza.

Esto **no es un bug de C** (semántica `#if/#elif` correcta). Es una pregunta de
**TopDesign/hardware** que no se puede resolver leyendo `.c`:

- ¿El plan es tener DOS `.cydsn` (uno por variante, cada uno con su TopDesign
  recortado a sus componentes)?
- ¿O UN TopDesign con ambas cadenas analógicas dibujadas y la idea es que cada placa
  física solo "puebla" (suelda) los componentes de su variante, dejando los otros
  sin conexión — y entonces `PSOC_HW_CLASS` debería decidirse por OTRO medio (ej. un
  pin de selección leído en runtime) en vez de por `#if defined(...)` en
  compile-time?

**Acción sugerida para la sesión de hardware**: abrir `AcondicionamientoAnalogico.cydsn`
en PSoC Creator y mirar el TopDesign. Si confirman que hay dos cadenas completas
dibujadas, esto define cómo construir el firmware "Hammer" — probablemente haga falta
quitar/placear componentes y regenerar antes de poder compilar esa variante.

### 2.1 NUEVO esta sesión: el fitter report (`.rpt`) muestra recursos analógicos al 100% — y son TODOS Geo

`AcondicionamientoAnalogico.rpt` (5260 líneas; termina en "Routing successful" +
Bitstream Generation/Verification + Static Timing OK → build exitoso, consistente con
"0 Errors" de la memoria — las ~1500 líneas "overuses wire" en `Analog Routing`
son iteraciones normales de rip-up-and-reroute, no errores finales).

Tabla de utilización (líneas 2405-2438):

| Resource Type | Used | Free | Max | % Used |
|---|---|---|---|---|
| Opamp | 4 | 0 | 4 | 100% |
| Delta-Sigma ADC | 1 | 0 | 1 | 100% |
| LPF | 2 | 0 | 2 | 100% |
| Analog (SC/CT) Blocks | 3 | 1 | 4 | 75% |
| **VIDAC** | **4** | **0** | **4** | **100%** |

"Analog Placement Results" — inicial (líneas 2444-2474) y final (2495-2509, mismos 4
componentes con índices VIDAC reasignados) listan, para los 4 VIDAC, **únicamente**:
`VDAC_Ref_Adder`, `VDAC_ref_BP`, `VDAC_ref_LP`, `VDAC_ref_PGA` (los 4 **Geo**). Los 4
Opamp son `OPAref`/`OPAlp`/`OPAbp`/`OPAadder` (Geo). El "Port Configuration report"
final (~líneas 5201-5226, fin del archivo) confirma el pinout físico: `Vref, BPo, BPm,
INp, INn, SEo, LPm, ADDERm, ADDERo, LPo, AMux_0, AMux_1` + UART + `SYNC_IN` — otra vez
solo nombres **Geo**.

**Ningún componente Hammer** (`PGA`, `VDAC_ref_IN`, `VDAC_PGA`, `VDAC_LP`, `LPF_ref`,
`LPF_ADC`, `Opa_ref_IN`, `Opa_ref_PGA`, `Opa_LP`) **aparece en ninguna de las dos listas
de placement ni en el pinout final** — a pesar de que `PGA.h`, `VDAC_ref_IN.h`,
`VDAC_PGA.h`, `VDAC_LP.h` SÍ existen en `Generated_Source/PSoC5/` con
`_DEFAULT_GAIN`/`_DEFAULT_DATA`/`_SetValue` válidos (§3): se generó código para
componentes que el fitter de este `.rpt` NO colocó/ruteó. Mismo patrón que las "ISRs
huérfanas" de §3, pero a nivel de bloques analógicos completos.

**Hallazgo extra, mismo patrón**: `VDAC_refIn` (sin guión bajo, distinto de
`VDAC_ref_IN`) es un 9° componente tipo-VDAC — listado como item top-level en
`.cyprj` (línea 10078, junto a `VDAC_ref_PGA`@10342 y `VDAC_ref_IN`@10717, mismo
formato de entrada hermana) y con `.lst` compilado en
`CortexM3/ARM_GCC_541/Debug/VDAC_refIn{,_PM}.lst` — pero CERO referencias en `.c`/`.h`
fuera de `Generated_Source`, y tampoco aparece en el placement del `.rpt`. Generado y
compilado alguna vez, ni cableado en el TopDesign actual ni usado en firmware.

**Por qué importa para §2 (no es solo curiosidad)**: VIDAC, Opamp, Delta-Sigma ADC y
LPF están los 4 al **100% usado / 0 libre — solo con la cadena Geo**. Para que Hammer
(`VDAC_ref_IN/PGA/LP` = 3 VIDAC más, su propio Opamp/LPF/ADC) coexista en el MISMO
TopDesign que Geo haría falta ~el doble de VIDAC/Opamp/DSM-ADC/LPF de los que el
CY8C58LP tiene físicamente (Max=4/4/1/2 — ya al tope). Esto **descarta por recursos**
la opción "¿UN TopDesign con ambas cadenas dibujadas y selección en runtime?" del §2 —
no alcanza con un pin de selección, las dos cadenas no pueden estar simultáneamente
routeadas. La opción "**dos `.cydsn` separados**", cada uno saturando el 100% de
VIDAC/Opamp/ADC/LPF con su propia cadena, queda como la única arquitectura viable
confirmada por hardware. (Ya sea que Hammer hoy NO esté dibujado en el TopDesign —los
Generated_Source serían sobras de una iteración anterior— o que SÍ lo esté pero este
`.rpt` sea previo a esa adición, la conclusión de recursos es la misma: Geo solo ya
satura los 4 bloques analógicos clave). Sigue siendo decisión humana — nada se tocó —
pero ahora con una razón cuantitativa, no solo de `#if/#elif`.

---

## 3. Auditoría de API — Generated_Source/PSoC5 (variante GEO, la activa hoy)

Todas las funciones que `calibration.c` / `calibration_tables.h` / `psoc_hw.c` /
`psoc_adc.c` llaman, verificadas contra los headers generados:

| Función | Declarada en | Estado |
|---|---|---|
| `VDAC_ref_PGA_Start/_SetValue` | VDAC_ref_PGA.h:51,55 | ✅ |
| `VDAC_ref_BP_Start/_SetValue` | VDAC_ref_BP.h:51,55 | ✅ |
| `VDAC_Ref_Adder_Start/_SetValue` | VDAC_Ref_Adder.h:51,55 | ✅ |
| `VDAC_ref_LP_Start/_SetValue` | VDAC_ref_LP.h:51,55 | ✅ |
| `VDAC_ref_IN_Start/_SetValue` (Hammer) | VDAC_ref_IN.h:51,55 | ✅ existe (rama no compila hoy, §2) |
| `VDAC_PGA_Start/_SetValue` (Hammer) | VDAC_PGA.h:51,55 | ✅ existe |
| `VDAC_LP_Start/_SetValue` (Hammer) | VDAC_LP.h:51,55 | ✅ existe |
| `PGAvdac_Start/_SetGain` | PGAvdac.h:62,65 | ✅ existe, `PGAvdac_DEFAULT_GAIN=(0u)`@101 — usado por `psoc_hw.c` bajo `#if defined(PGAvdac_DEFAULT_GAIN)` (psoc_hw.c:15-16,43-44) |
| `AMux_ADC_Start/_Select` | AMux_ADC.h:37,87 | ✅ |
| `AMux_IN_Start/_Select` | AMux_IN.h:37,87 | ✅ |
| `isr_DelSig_Disable/_Enable/_ClearPending` | isr_DelSig.h:35,37,40 | ✅ y es el correcto (ver abajo) |
| `isr_SyncIn_Disable/_Enable/_ClearPending` | isr_SyncIn.h:35,37,40 | ✅ y es el correcto |
| `ADC_SelectConfiguration/_Start/_StopConvert/_StartConvert/_GetResult32/_IsEndConversion` | ADC.h | ✅ API estándar Delta-Sigma |
| Guards `#error` `VDAC_*_DEFAULT_DATA` (calibration_tables.h:72-83,105-113) | todos definidos en Generated_Source | ✅ ninguno dispara |

**Confirmación isr_DelSig / isr_SyncIn correctos**: `main.c:576-578`
```c
isr_DelSig_StartEx(isr_DelSigReady);
isr_Timer_StartEx(isr_Timer);
isr_SyncIn_StartEx(isr_SyncIn);
```
`isr_DelSig` (componente) está atado al handler `isr_DelSigReady` (la ISR de
"muestra lista" que llena el buffer de captura, main.c:198-221). Por lo tanto
`isr_DelSig_Disable()` en `calibration.c:132` apaga la ISR correcta.

**Componentes ISR "huérfanos"** (existen en Generated_Source, ningún `_StartEx` en
`main.c`/`calibration.c`): `isr_DelSig_1`, `isr_adc`, `isr_newSampleSaved`,
`isr_Filter`, `isr_DMA_Filter`, `isr_SAR`, `isr_boton`. Inertes (nunca se habilitan),
pero confirman que el TopDesign acumula componentes de iteraciones previas — mismo
patrón que `[[feedback_psoc_isr_patterns]]` (StartEx copy-paste). No bloquean nada,
solo ruido.

**Mismo patrón en analógico** (NUEVO esta sesión, ver §2.1): `VDAC_refIn` (sin guión,
9° componente tipo-VDAC) y todo el set Hammer (`PGA`, `VDAC_ref_IN/PGA/LP`,
`LPF_ref/ADC`, `Opa_ref_IN/PGA`, `Opa_LP`) tienen Generated_Source válido (filas de
arriba ✅) pero **no aparecen en el placement del fitter** (`.rpt`) — generados pero
no routeados/usados, igual que las ISRs huérfanas.

---

## 4. Targets de calibración 52429 / 78644 — VERIFICADOS, no son placeholders arbitrarios

`ADC.h` documenta ambas configs:

```
ADC_CFG1: Sample Rate 3000 Hz, Resolution 18 bits, Conv Mode 2 (continuo)
  DEC_SHIFT1=4, DEC_SHIFT2=4  → shift HW = 2^(4+4) = 2^8 = 256
  DEC_DIV=32 = 2^5 (división por software en psoc_adc_counts_right_aligned)
  → normalización total = 256 × 32 = 2^13 = 8192
  ADC_CFG1_COUNTS_PER_VOLT = 52429
  ADC_CFG1_REFERENCE_VOLTAGE = 1.25 V

ADC_CFG2: Sample Rate 2475 Hz, Resolution 18 bits, Conv Mode 0 (single-sample)
  DEC_SHIFT1=4, DEC_SHIFT2=9  → shift HW = 2^(4+9) = 2^13 = 8192
  DEC_DIV=0 → en psoc_adc_counts_right_aligned, el bloque
              `#if (ADC_CFG2_DEC_DIV != 0)` es FALSO en compile-time → NO divide
  → normalización total = 8192 × 1 = 2^13 = 8192   (¡IGUAL que CFG1!)
```

Ambas configs normalizan al **mismo factor 2^13 total** (HW shifts × DEC_DIV
software), combinando de forma distinta pero llegando al mismo lugar — así es como
PSoC Creator hace que "cuentas" signifique lo mismo en ambas configs aunque tengan
sample rates distintos. Por lo tanto:

- `ADC_CFG1_COUNTS_PER_VOLT = 52429` aplica IGUAL a las lecturas en CFG2 (la que usa
  `psoc_adc_read_single_counts()` durante calibración).
- `CAL_TARGET_1V_COUNTS = 52429` = exactamente `ADC_CFG1_COUNTS_PER_VOLT` → **1.00 V**.
- `CAL_TARGET_1V5_COUNTS = 78644` ≈ `52429 × 1.5 = 78643.5` (redondeado) → **1.50 V**.

**Conclusión**: las unidades/escala de los targets son correctas tal como está el
código (no hay que tocar `psoc_adc_counts_right_aligned` ni los `#define`). Lo que
SIGUE pendiente (y es de hardware/circuito, no de software) es: **¿1.00 V y 1.50 V
son los voltajes de salida deseados para cada etapa?** Si no, recalcular con
`target_counts = target_volts × 52429`.

También: `ADC_CFG2_CONV_MODE = 0` (Single Sample) — confirma que CFG2 fue
customizado específicamente para el patrón `StartConvert→IsEndConversion(WAIT)→
GetResult32→StopConvert` que usa `psoc_adc_read_single_counts()`
(`psoc_adc.c:45-53`). Coherente, buen diseño.

---

## 5. Timeout `0xB5` (Calibrar) — riesgo CUANTIFICADO, falta medir en hardware

ESP slave: `slave/src/main.cpp:129  #define PSOC_CAL_ACK_TIMEOUT_MS 15000u` — el
esclavo espera hasta 15 s el `CFG_ACK` del PSoC tras mandar `0xB5`
(`psoc_uart.cpp:187 calibrate() → _sendCmd1(PSOC_CMD_CALIBRATE, 1)`, mismo framing
`_sendCmd1` que ya usan 0xA6/0xA9/0xAA — riesgo bajo de framing).

Estimación de tiempo de calibración en el PSoC (worst case, sin convergencia,
`CAL_MAX_ITER=90` en todas las etapas):

```
por iteración = CAL_AVG_N(32) × T_read_CFG2  +  CAL_DAC_SETTLE_MS(5ms)

T_read_CFG2 (Single Sample @ 2475 Hz):
  optimista  ≈ 1/2475 s ≈ 0.40 ms   → iter ≈ 12.9+5  = 17.9 ms
  pesimista  ≈ 4×(1/2475) ≈ 1.6 ms  → iter ≈ 51.2+5  = 56.2 ms
  (el "pesimista" asume que el pipeline del decimador necesita ~4 periodos de
  muestra para asentar en modo single-sample, patrón típico de Delta-Sigma)

por etapa (90 iters):     optimista ≈ 1.61 s   |  pesimista ≈ 5.06 s
GEO    (4 etapas):         optimista ≈ 6.45 s   |  pesimista ≈ 20.2 s   ⚠ supera 15s
HAMMER (3 etapas):          optimista ≈ 4.84 s   |  pesimista ≈ 15.2 s   ⚠ al límite
```

`PSOC_CAL_ACK_TIMEOUT_MS=15000` da margen 2.3x en el caso optimista, pero el caso
pesimista para GEO **lo supera**. El caso "todas las etapas usan los 90 iters sin
converger" sería en sí mismo una señal de mal funcionamiento (con PID KP=4 debería
converger mucho antes si el target es alcanzable) — pero conviene no depender de
"debería converger" para el timeout del protocolo.

### ✅ APLICADO esta sesión

`slave/src/main.cpp:129`: `PSOC_CAL_ACK_TIMEOUT_MS` 15000 → **25000** (25s).
`pio run` (slave1/slave2/slave3) → SUCCESS, mismo footprint RAM/Flash (13.5% /
55.9%). Cambio de una sola constante, sin tradeoffs de RF.

La simulación PID (§11.D/E, NO disponible cuando se escribió la estimación de
arriba) **confirma cuantitativamente** que esto era necesario: con el lazo PID real
(no solo el modelo "90 iters worst case"), escenarios de "varias etapas en FAIL" —
que §11.A muestra que NO son raros para un rango amplio de `gain` físicos — dan
20438ms (GEO) / 15329ms (HAMMER) / 15504ms (GEO realista 3-de-4-FAIL) en el caso
pesimista, los tres por ENCIMA de 15000ms y por DEBAJO de 25000ms (margen ≥4500ms).
Ver §11 para el detalle y para cómo re-chequear esto una vez que se midan las
ganancias reales de cada etapa.

---

## 6. ISRs durante calibración — verificado correcto

- `isr_DelSig_Disable()` (calibration.c:132): apaga la ISR de muestra lista — sin
  esto, durante `psoc_adc_read_single_counts()` (que hace su propio
  `StartConvert/StopConvert` en CFG2) la ISR de CFG1 podría interferir. Correcta.
- `isr_SyncIn_Disable()` (calibration.c:133): **necesaria** porque `isr_SyncIn`
  (main.c:241-264) tiene una rama `else` (SYNC_IN en bajo) que hace
  `ADC_StopConvert(); timer_start_runtime(); ... g_state=PSOC_IDLE`. Si SYNC_IN
  está en bajo (su estado de reposo probable) durante la calibración de boot, esa
  ISR — si NO estuviera deshabilitada — llamaría `ADC_StopConvert()` en medio de
  `psoc_adc_read_single_counts()`, rompiendo la secuencia
  `StartConvert→IsEndConversion→GetResult32→StopConvert`. Buen safeguard, ya
  presente.
- `isr_Timer` (10ms tick, RX watchdog) **NO se deshabilita** durante calibración —
  y no hace falta: durante el boot `rx_state==0`, así que
  `g_rx_watch_ticks`/`watchdog_rx` no se disparan (main.c:223-239). Confirmado
  inofensivo.
- `g_state` arranca en `PSOC_IDLE` (main.c:92) y nada lo cambia antes de
  `psoc_run_calibration_if_idle()` en boot (main.c:585) → la calibración de boot
  siempre corre.

**ACTUALIZACIÓN 2026-06-13 (§12)**: el análisis de arriba sigue siendo correcto
para lo que evalúa (SyncIn/Timer), pero está INCOMPLETO — no consideró que
`isr_DelSig_Disable()` también enmascara la interrupción que CFG2 necesita para
señalar `ADC_convDone`. Eso produce un cuelgue permanente en la primera lectura
de la primera etapa. Ver §12 para el mecanismo completo y el fix candidato.

---

## 7. Punch-list

1. **✅ ARREGLADO esta sesión** — `psoc_hw.c:36`:
   `PGAn_SetGain(PGAp_GAIN_02)` → `PGAn_SetGain(PGAn_GAIN_02)`.
   Mismo valor numérico (`0x01u` en ambos: PGAp.h:86 / PGAn.h:86), cero riesgo
   funcional — solo usa la constante de la instancia correcta para claridad.
2. **[Arquitectural, necesita tu decisión]** §2 — `PSOC_HW_CLASS` siempre GEO en
   este `Generated_Source`; "Hammer" no compila desde este árbol tal cual está.
   §2.1 (nuevo): el fitter report confirma VIDAC/Opamp/DSM-ADC/LPF al 100% solo con
   Geo → "1 TopDesign con selección runtime" no entra por recursos; "2 `.cydsn`
   separados" es la única opción viable confirmada por hardware (decisión sigue
   pendiente).
3. **✅ ARREGLADO esta sesión** — §5: `PSOC_CAL_ACK_TIMEOUT_MS` 15000→25000,
   `pio run` slave1/2/3 verificado, justificado cuantitativamente por §11.D/E.
4. **[De la sesión anterior, sigue abierto]** — signo `direction` por etapa,
   `CAL_TOL_COUNTS=250` vs counts/LSB real, voltajes target 1.0V/1.5V por etapa
   (§4 ya confirma que las UNIDADES están bien; falta el VALOR físico correcto).
   §11 da una receta concreta para chequear esto con `cal_sim.py` apenas se midan
   los `gain` reales — incluyendo una banda de "resolución insuficiente" (§11.A,
   gains ~800-1200 para target=52429, `gain>2·tol`) donde la etapa daría `ok=0` de
   forma reproducible sin que sea bug — ahí el remedio es subir `CAL_TOL_COUNTS`,
   NO retunear el PID (ver punto 7).
5. **[Nuevo, necesita tu decisión]** §10 — el bit `ok` del `CFG_ACK` de CALIBRATE
   conflactúa "busy/rehusado", "convergió casi todo salvo 1 etapa con error chico"
   y "1 etapa con `direction`/target mal" en el mismo `ok=0` / mismo string de UI
   ("Calibracion pendiente o fallida"). No bloquea la sesión de hardware (los datos
   viajan bien), pero limita el diagnóstico. §10 propone 3 opciones (debug-print,
   extender el frame, o solo cambiar el texto de la UI).
6. **[Nuevo, informativo]** §11.B — si una etapa en hardware da `ok=0` de forma
   sistemática mientras las demás convergen, sospechar primero signo de
   `direction` mal puesto en `calibration_tables.h` (falla "segura": el VDAC queda
   como estaba) antes que HW roto.
7. **✅ CORREGIDO esta sesión** — §11.A clasificaba la banda FAIL 800/1000/1200
   como "limit-cycle del PID, retunear KP/KI"; **era incorrecto**. Es un problema
   de RESOLUCIÓN (`gain>2·tol`, ningún entero cae en tol — el PID ya está en el
   óptimo). Nuevo helper `true_optimum()` en `cal_sim.py` + experimento (G)
   verifican esto empíricamente y dan el algoritmo de diagnóstico correcto
   (RESOLUCIÓN→subir tol; DINÁMICA `gain≥~2000`→bajar `KP_Q12<2·4096/gain`).
8. **[Nuevo, informativo, no bloquea]** §2.1/§3 — `Generated_Source/PSoC5` contiene
   un 9° componente VDAC (`VDAC_refIn`, sin guión) y todo el set Hammer (`PGA`,
   `VDAC_ref_IN/PGA/LP`, `LPF_ref/ADC`, `Opa_ref_*`) con símbolos válidos pero
   AUSENTES del placement del fitter (`.rpt`) — generados, nunca routeados/usados.
   Si se decide ir por "2 `.cydsn` separados" (item 2), esto da pistas de qué
   componentes ya tienen Generated_Source listo para el `.cydsn` Hammer (aunque
   habría que re-colocarlos/re-rutear, ya que no están en el `.rpt` actual).
9. **[BLOQUEANTE — sesión 2026-06-13, FIX APLICADO]** §12 — la calibración de
   boot CUELGA el PSoC para siempre en su primera lectura ADC en CFG2
   (`psoc_adc.c:49`), porque `isr_DelSig_Disable()` (calibration.c:132)
   enmascara la interrupción de la que `ADC_convDone` depende en CFG2, y nada la
   re-habilitaba antes de usarla. Esto explica "sin señal de vida del PSoC desde
   que agregamos autocalibración" — confirmado con captura de 90s
   (`uartBytes=0` todo el tiempo, vs ~20s peor caso esperado de §11.D/E).
   Fix de 1 línea (`isr_DelSig_Enable()` en psoc_adc.c:33) APLICADO, pendiente
   compilar+programar+verificar en hardware (ver §12).

---

## 8. Pendiente para esta sesión nocturna (próximas iteraciones del loop)

- [x] Leer ESP slave/master + JS modificados (`app.js`, `config.js`, `plot.js`,
      `slave_panel.js`, `matlab_transport.h`, `sync_protocol.h` ×2,
      `slave/src/main.cpp` completo, `psoc_uart.cpp/.h`) — confirmar framing 0xB5
      end-to-end y el botón "Calibrar" en la UI. → §10 (camino completo + hallazgo
      de sobrecarga semántica del bit `ok`).
- [x] `pio run` baseline (master + slave1/2/3) — establecer build limpio antes/
      después de cualquier edit ESP. → §10 (resultados RAM/Flash).
- [x] `node --check` sobre los `.js` modificados. → §10 (Node no disponible en
      este entorno; sustituido por checker de balance de brackets en Python,
      "balanced OK" para los 4 archivos — pendiente correr `node --check` real).
- [x] Aplicar (si se confirma) el bump de `PSOC_CAL_ACK_TIMEOUT_MS` 15000→25000 y
      re-correr `pio run`. → §5 (aplicado y verificado).
- [x] Guardar la simulación Python del PID/media móvil como `tools/cal_sim.py`
      (parametrizable con los valores reales de `calibration_tables.h`), para que
      sirva de referencia cuando lleguen datos de hardware. → §11.
- [x] Revisar el fix de persistencia Raw/Filtered en `plot.js` (localStorage)
      mencionado en `[[project_psoc_calibration]]`. → §10 (releído completo,
      sin issues).
- [x] Corregir atribución de la banda FAIL 800/1000/1200 (era "limit-cycle del
      PID/retunear KP-KI", en realidad RESOLUCIÓN `gain>2·tol`) y verificar
      empíricamente con `true_optimum()` + experimento (G). → §11.A (corregido),
      §11.G (nuevo), §7 punto 7.
- [x] Re-verificar TODA la API de `calibration.c`/`calibration_tables.h`/`psoc_hw.c`/
      `psoc_adc.c` contra `Generated_Source/PSoC5` (VDACs Geo+Hammer, `PGAvdac`,
      `AMux_ADC/IN`, `isr_DelSig/SyncIn`, `ADC_SelectConfiguration`+CFG1/CFG2) — TODO
      ✅, sin símbolos inventados. → §3 (tabla ampliada).
- [x] Trazar mecanismo 0xA9/0xAA "legacy ACK" end-to-end (ESP slave → UART → PSoC
      `main.c`) — confirma con código que son no-op de HW post-boot. → §10
      (Confirmaciones adicionales).
- [x] Leer `AcondicionamientoAnalogico.rpt` (fitter report, 5260 líneas) — VIDAC/
      Opamp/DSM-ADC/LPF al 100% solo con componentes Geo; set Hammer + `VDAC_refIn`
      ausentes del placement pese a tener Generated_Source válido. → §2.1 (NUEVO),
      §3, §7 puntos 2/8.
- [x] Extender `tools/cal_sim.py` con modelo de ruido gaussiano del ADC
      (`make_noisy_plant`, opt-in, promedia `CAL_AVG_N=32` lecturas como
      `avg_counts()`) y barrer sigma para gain=300/1000/2100 → §11.H (nuevo):
      `best_dac` robusto; nuevo riesgo de falso `ok=1` en RESOLUCIÓN a sigma alto.

---

## 9. Checklist de bring-up con hardware (nada de esto se puede resolver sin placa)

- [ ] Abrir el TopDesign en PSoC Creator y resolver §2 (Geo+Hammer combinados o
      solo Geo realmente activo).
- [ ] Medir `T_read_CFG2` real (osciloscopio/GPIO toggle alrededor de
      `psoc_adc_read_single_counts()`) → validar contra el timeout (§5).
- [ ] Por etapa: confirmar signo `direction`, medir counts/LSB de DAC (`gain`), y
      correr `tools/cal_sim.py` con ese `gain` para predecir si converge dentro de
      `CAL_MAX_ITER=90` y dentro de `CAL_TOL_COUNTS=250` — ver receta en §11. Si da
      FAIL, comparar `r.best_dac` con `true_optimum(target, plant)`: si coinciden
      (RESOLUCIÓN, §11.A.3) subir `CAL_TOL_COUNTS` de esa etapa; si NO coinciden
      (DINÁMICA, §11.A.4) bajar `KP_Q12` (`<2·4096/gain`). No asumir HW roto.
- [ ] Confirmar/recalcular voltajes target por etapa (§4: `counts = volts × 52429`).
- [ ] Probar botón "Calibrar" end-to-end (web→master→slave→PSoC→CFG_ACK) una vez;
      si falla, leer `g_psoc_cal_stages[i].name` por debug UART para ver en qué
      etapa. Recordar §10: `ok=0` puede significar 3 cosas distintas — no asumir
      automáticamente que es un fallo grave.

---

## 10. Camino ESP/Web 0xB5 (botón "Calibrar") — verificado end-to-end

Trazado completo, opcode por opcode, sin tocar hardware:

```
[Web] slave_panel.js:96-100  botón "Calibrar" -> evento 'calibrate-requested'
   -> app.js: onCalibrateRequested (~470-535) -> sendDirected(..., SUBCMD_CALIBRATE)
   -> config.js:33  SUBCMD_CALIBRATE = 0xB5
[Master ESPNOW] reenvia el frame dirigido al slave correspondiente
[Slave] main.cpp handleSetConfig (340-399) -> case 0xB5 -> psoc.calibrate()
   -> psoc_uart.cpp: calibrate() -> _sendCmd1(PSOC_CMD_CALIBRATE=0xB5, 1)
       (mismo framing que 0xA6/0xA9/0xAA, ya validado en sesion anterior)
[PSoC] main.c dispatcher -> case PSOC_CMD_CALIBRATE:
   -> uart_send_cfg_ack(0xB5, psoc_run_calibration_if_idle())   (main.c:456)
   -> uart_send_cfg_ack arma frame: [0xAB, CFG_ACK, cmd=0xB5, val, xor-checksum]
                                                                  (main.c:122-130)
[Slave] psoc_uart.cpp: _parseConfigAck/takeConfigAck (132-165)
   -> cmd=_buf[2]=0xB5, val=_buf[3]=resultado 0/1
   -> main.cpp: servicePsocConfigAck (242) -> ok=(val==g_cfg_param)?1:0
   -> sendCfgAck(...) hacia el master
[Master] _handleCfgAck -> app.js: handleAck (745-800), rama CALIBRATE (775-779)
   -> slave_panel.js: setCalibrationLock(state) (233, 274-276)
```

**Conclusión**: opcodes (`0xB5`), framing (`_sendCmd1`, frame de 5 bytes con XOR
checksum) y eventos JS están todos consistentes end-to-end. No hay nada roto en este
camino.

### HALLAZGO: sobrecarga semántica del bit `ok` en CALIBRATE

`servicePsocConfigAck` (main.cpp:242) es genérico:

```cpp
ok = (ackVal == g_cfg_param) ? 1 : 0;
```

Para `0xA6`/`0xA9`/`0xAA`, `val` es un ECO del valor aplicado → `ok=0` significa "el
PSoC aplicó algo distinto a lo pedido" (mismatch real, raro).

Para CALIBRATE, **ambos lados hardcodean `g_cfg_param=1`** (`_sendCmd1(0xB5,1)` y el
`g_cfg_param` que arma `waitForPsocConfigAck`), y `val = psoc_run_calibration_if_idle()`
devuelve:

- `0` si `g_state != PSOC_IDLE` → **rehusado, ni siquiera corrió** (busy)
- `0` si corrió `psoc_calibration_run_blocking()` pero **alguna etapa** salió por
  `calibrate_stage()==0` (best_dac fallback, no convergió dentro de `tol`)
- `1` solo si **todas** las etapas convergieron (`abs_error<=tol`)

`ok=(val==1)?1:0` da el resultado correcto para "todo perfecto" por coincidencia
(1==1), pero **`ok=0` conflactúa situaciones muy distintas**:

| Caso | `val` | UI muestra |
|---|---|---|
| (a) Busy/rehusado, calibración NO corrió | 0 | "Calibracion pendiente o fallida" |
| (b) Corrió, 1 etapa con error residual chico (ej. 71-429 counts ~ 0.0014-0.0082V, ver §11.A) | 0 | "Calibracion pendiente o fallida" |
| (c) Corrió, 1 etapa con `direction`/gain tal que el target es inalcanzable para esa etapa (ver §11.B/C) | 0 | "Calibracion pendiente o fallida" |

(slave_panel.js:275, app.js:777-778 — mismo string para los tres casos)

**Esto NO es un bug de protocolo** (los bits viajan correctos). Es un problema de
**diagnosticabilidad** para la sesión de bring-up: si "Calibrar" da "fallida", hoy NO
hay forma de saber, sin instrumentación adicional, si conviene reintentar (caso a),
si el resultado YA quedó "suficientemente bien" aplicado vía `best_dac` (caso b —
`calibration.c:97` SIEMPRE escribe `best_dac` al VDAC aunque falle), o si una etapa
específica tiene `direction`/`target`/`tolerance` mal configurados en
`calibration_tables.h` (caso c).

**Sugerencias (NO aplicadas, requieren tu decisión)** — ver punch-list §7 item 5:
1. Más barato: debug-print por UART en `psoc_calibration_run_blocking()` con
   `(name, ok, best_dac, best_abs_error)` por etapa — sin tocar el framing `CFG_ACK`.
2. Más invasivo: extender el frame `CFG_ACK` de CALIBRATE con bytes adicionales
   (bitmask por etapa, o índice+`best_abs_error` de la peor) — toca PSoC+slave+
   master+UI.
3. Mínimo: cambiar el string de UI para CALIBRATE a algo más neutro como
   "Calibracion no confirmada (ver consola/serial)".

### Confirmaciones adicionales (sin cambios de código)

- **Mecanismo 0xA9/0xAA "legacy ACK" (NUEVO esta sesión, confirma comentario
  `main.c:29-30`)**: en PSoC `main.c`, `0xA9`→`PGAvdac_Set(rx_p1)` solo hace
  `g_pgavdac_code=rx_p1` (NO llama HW, a diferencia de `PGAgain_Set`/`0xA6` que sí
  llama `psoc_hw_set_pga`, main.c:303-316); `0xAA`→`g_vdac_val=rx_p1` directo. Ambas
  variables son inertes: `g_pgavdac_code` se consume UNA sola vez, en
  `psoc_hw_start_analog(g_pga_code, g_pgavdac_code)` (main.c:572), llamado en el
  arranque ANTES de que pueda llegar cualquier comando UART; `g_vdac_val` no se lee
  en ningún otro lado. Resultado: post-boot, `0xA9`/`0xAA` **ACKean (CFG_ACK con eco
  del valor recibido) pero no tienen efecto en hardware** — el ESP (`slave/main.cpp`)
  los despacha con `waitAck=true` como si fueran "vivos" a nivel mensaje, pero son
  no-op a nivel HW. Confirma textualmente memoria ("0xA9/0xAA quedaron legacy/no-op")
  con mecanismo verificado.
- **localStorage Raw/Filtered** (`app.js:29-100`, `1015-1035`, `plot.js`): patrón
  `loadBoolSetting`/`saveSetting` con `try/catch` (browsers en modo privado) —
  releído completo, correcto, sin issues.
- **`pio run` baseline**:
  - `master` (`esp32dev`): RAM 14.9% (48672/327680), Flash 66.1% (866433/1310720),
    SUCCESS 4.56s.
  - `slave1/2/3`: RAM 13.5% (44168/327680) los tres; Flash 55.9%
    (732909 slave1, 732941 slave2/slave3 — diferencia de ~32 bytes, esperable por
    `#define` de identidad por env). SUCCESS ~3s c/u.
- **`node --check` no disponible** en este entorno (sin `node`/`npm` en PATH ni en
  rutas comunes — verificado con `Get-Command`/`Get-ChildItem`). Sustituto: checker
  Python ad-hoc de balance de `(){}[]`/strings/template-literals/comentarios
  (`C:\Users\elias\AppData\Local\Temp\js_balance_check.py`, fuera del repo) sobre
  `app.js`, `config.js`, `plot.js`, `slave_panel.js` -> **"balanced OK"** los 4. Esto
  NO reemplaza `node --check` (no detecta errores de sintaxis que no rompan el
  balance de brackets) — **correr `node --check` real cuando haya Node disponible**.

---

## 11. Simulación Python del lazo PID (`tools/cal_sim.py`) — resultados

Nuevo archivo: `AcondicionamientoAnalogico.cydsn/tools/cal_sim.py` — puerto 1:1 de
`calibrate_stage()` (calibration.c:53-100) a Python, con un modelo de planta lineal
`measured(dac) = clip(offset + gain*dac, 0, 131071)`. Constantes copiadas de
`calibration_tables.h` (KP=4, KI=1, KD=0, AVG_N=32, MAX_ITER=90, TOL=250,
DAC_INIT=128, INTEGRAL_LIMIT=±400000). El operador `>>` de Python sobre enteros con
signo es shift aritmético = floor-division, igual que ARM GCC para `int32` con
signo — el puerto es fiel sin reimplementar la división a mano.

Correr: `python tools/cal_sim.py` (Python 3.14, sin dependencias extra, ~6
experimentos A-F impresos por stdout).

### (A) Barrido de ganancia counts/LSB (target=52429, dac arranca en 128, offset=0)

`true_dac`/`true_|err|` = óptimo global por búsqueda exhaustiva 0..255 (helper
`true_optimum()`, nuevo esta sesión). `óptimo?` = `best_dac==true_dac` (¿el PID
encontró lo mejor posible?).

| gain | resultado | best_dac | best\|err\| | true_dac | true\|err\| | óptimo? |
|---|---|---|---|---|---|---|
| <205 | FAIL | 255 | 1429-52174 | 255 | = best | sí |
| 205-206 | OK | 255 | 101-154 | 255 | = best | sí |
| 250 | OK | 209 | 179 | 210 | 71 | NO (pero OK igual, ver nota) |
| 300-600 | OK | — | 29-229 | — | = best | sí |
| **800** | **FAIL** | 66 | **371** | 66 | 371 | **sí** |
| **1000** | **FAIL** | 52 | **429** | 52 | 429 | **sí** |
| 1024 | OK | 51 | 205 | 51 | 205 | sí |
| **1200** | **FAIL** | 44 | **371** | 44 | 371 | **sí** |
| 1500 | OK | 35 | 71 | 35 | 71 | sí |
| **2000-10000** | **FAIL** | 7-32 | 11571-47571 | 5-26 | 429-2429 | **NO** |

Nota fila 250: converge (`best_|err|=179≤tol`) pero el PID no encontró el óptimo
absoluto (`true_dac=210` da `71`); como ya estaba dentro de tol no importa. Es la
única fila "óptimo=NO" que da OK — en todas las filas FAIL, "óptimo" es la columna
que decide el régimen (hallazgos 3/4).

**Hallazgos**:

1. **Reachability**: `gain < target/255 ≈ 205.6` → target físicamente inalcanzable
   (ni con `dac=255`). FAIL "limpio", `best_dac=255=true_dac` (óptimo=sí), error
   residual = cuánto le falta a `gain×255` para llegar a `target`.
2. **Banda "buena" 205-1500 (salvo 800/1000/1200)**: converge en pocas iteraciones
   (3-20), bien dentro de `max_iter=90`.
3. **CORREGIDO esta sesión — banda FAIL 800/1000/1200 es de RESOLUCIÓN, no
   "limit-cycle"/dinámica**: para los tres, `óptimo?=sí` — **el PID ya encontró el
   mejor código DAC posible en 0..255**. Lo que pasa es que `gain > 2·tol = 500`
   significa que **no hay garantía de que algún entero caiga dentro de `±tol` del
   target**; depende de `target mod gain`. Para estos tres gains el entero más
   cercano queda a 371-429 counts (>250=tol). (La sesión anterior describió esto
   como "limit-cycle del PID Q12" y sugería retunear `KP_Q12`/`KI_Q12` — **esa
   atribución era incorrecta**, corregida acá tras revisión.)
   **Verificación empírica (experimento G)**: para `gain=1000`, barrer
   `kp∈{1,2,3,4,8,16}` da SIEMPRE `best_dac=52, best_|err|=429` (kp≤4) o peor
   (`kp≥8`) — nunca `≤250`. Ningún `kp` cambia el resultado porque `52` YA es el
   óptimo. En cambio, subir `CAL_TOL_COUNTS` de 250→450 (>429) hace que **converja
   en 11 iters con el mismo `best_dac=52`** — la única palanca real para este
   régimen es `CAL_TOL_COUNTS` (o cambiar la ganancia física de la etapa).
4. **Régimen DISTINTO para `gain≥2000`**: acá `óptimo?=NO` — el PID **no encuentra**
   el mejor código posible (`best_|err|` 11571-47571 vs `true_|err|` 429-2429, muy
   lejos). Este SÍ es un problema de dinámica/sintonía. **Verificación empírica
   (experimento G)**: para `gain=2100` (`true_|err|=71≤tol`, o sea el óptimo SÍ
   convergería si el PID lo alcanzara), bajar `kp` de 4→{1,2,3} hace que
   **converja** (10-12 iters, `best_dac=25=true_dac=25`, `err=71`); `kp=4` (actual)
   sigue en FAIL. El límite de estabilidad lineal `gain·kp/4096<2` predice
   `kp<3.90` para `gain=2100` — **kp=3 converge, kp=4 no: calza exactamente**.

**Implicación para hardware** — algoritmo de diagnóstico con datos medidos:
1. Medir `gain` real de la etapa (ver "Cómo reusar" más abajo) y correr `cal_sim.py`
   / `true_optimum()` con ese `gain`.
2. Si `best_dac==true_dac` (**RESOLUCIÓN**, ej. banda 800-1200 para target=52429):
   subir `CAL_TOL_COUNTS` de esa etapa por encima de `true_|err|`, o aceptar que con
   DAC de 8 bits esa etapa no calibra más fino que `±gain/2` counts. **Retunear PID
   NO sirve** (verificado, exp. G).
3. Si `best_dac!=true_dac` (**DINÁMICA**, ej. `gain≳2000` para este target): bajar
   `KP_Q12` de esa etapa (criterio `KP_Q12 < 2·4096/gain`); la tabla
   `calibration_tables.h` ya soporta `kp/ki/kd` independientes por etapa.
4. Si además `dac_code` satura en 0/255 al final de `r.history`: sospechar signo de
   `direction` (§11.B) antes que cualquiera de los dos regímenes anteriores.

### (B) Signo `direction` vs signo real de la planta (gain=300, target=52429)

- **Planta normal** (subir DAC -> sube ADC), `direction=+1` (hardcodeado, coincide):
  converge en 18 iters, `dac=175`, `err=71`. OK.
- **Planta invertida** (subir DAC -> BAJA ADC), `direction=+1` (NO coincide):
  realimentación positiva -> `dac_code` satura en 255 buscando, pero
  `calibration.c:97 stage->write(best_dac)` al salir por FAIL deja el VDAC en
  `best_dac=128 == initial_dac` (el primer punto evaluado fue, por construcción, el
  mejor de todo el barrido, porque a partir de ahí el error solo crece). FAIL
  (`ok=0`), pero **el VDAC queda EXACTAMENTE como antes de calibrar — no rompe nada**.

**Implicación**: si una etapa tiene el signo de `direction` mal puesto en
`calibration_tables.h` respecto al signo real de su lazo analógico, el síntoma es
"esa etapa siempre da fallida" pero **de forma segura** (no deja el VDAC en un
extremo arbitrario) — luce idéntico a los casos (a)/(c) de §10 en el ACK. Si en
hardware una etapa específica SIEMPRE falla y las demás no, **sospechar signo de
`direction` antes que HW roto**.

### (C) Target fuera de rango (gain=100, target=78644, plant(255)=25500)

FAIL limpio, `best_dac=255`, `best_|err|=53144`. Mismo patrón que el caso "gain bajo"
de (A) — comportamiento consistente, sin sorpresas, sin oscilaciones raras cuando el
target es simplemente inalcanzable por rango (a diferencia del limit-cycle de (A),
que ocurre con target SÍ alcanzable pero granularidad/dinámica desfavorable).

### (D)/(E) Tiempos totales — contrastado contra el timeout (§5)

Con `T_iter` optimista (17.93ms) y pesimista (56.72ms) de §5:

| Escenario | iters totales | tiempo optimista | tiempo pesimista |
|---|---|---|---|
| GEO, las 4 etapas convergen rápido (gain=500) | 42 | 773 ms | 2402 ms |
| GEO, las 4 etapas FAIL (gain=50/100/200/1000) | 360 | 6475 ms | **20438 ms** |
| HAMMER, las 3 etapas FAIL | 270 | 4856 ms | **15329 ms** |
| GEO "realista" (3 de 4 etapas FAIL, gains mixtos 80/300/600/150) | 273 | 4915 ms | **15504 ms** |

**Las tres filas en negrita SUPERAN el timeout viejo de 15000ms** — y "varias etapas
en FAIL" no es un escenario exótico: según (A), una fracción amplia y nada
infrecuente del espacio de `gain` posibles cae en FAIL (`gain<206`, banda 800-1200,
`gain≥2000`). Es decir, **un timeout de 15s podía cortar una calibración real a mitad
de camino** dependiendo de las ganancias físicas reales de cada etapa (aún
desconocidas).

**Con el nuevo timeout de 25000ms** (aplicado, §5): el peor caso observado (20438ms,
GEO-pesimista-todo-FAIL) queda con **~4500ms de margen**. 25000ms sigue siendo
razonable mientras `T_read_CFG2 < ~2.06ms` (25000 = 360×(32×T_read+5)+20 →
T_read≈2.06ms ≈ 5.1× el período nominal de 2475Hz).

### (G) Verificación empírica de los remedios de §11.A.3/4 (`tools/cal_sim.py`)

| caso | gain | true_dac (true\|err\|) | kp=1 | kp=2 | kp=3 | kp=4 (actual) | kp=8 | kp=16 |
|---|---|---|---|---|---|---|---|---|
| DINÁMICA | 2100 | 25 (71≤tol) | OK(12) | OK(10) | OK(10) | **FAIL** (best_dac=32) | — | — |
| RESOLUCIÓN | 1000 | 52 (429>tol) | FAIL(52) | FAIL(52) | FAIL(52) | FAIL(52) | FAIL(96,peor) | FAIL(0,peor) |

(`OK(n)`/`FAIL(dac)` = iters para converger / `best_dac` alcanzado.)

- **DINÁMICA (gain=2100)**: `kp∈{1,2,3}` converge con `best_dac=25=true_dac`;
  `kp=4` (default) FAIL. Confirma `KP_Q12 < 2·4096/gain ≈ 3.90` al counts.
- **RESOLUCIÓN (gain=1000)**: `kp∈{1,2,3,4}` da exactamente `best_dac=52,
  best_|err|=429` siempre (el óptimo no cambia con `kp`); `kp≥8` además empeora
  `best_dac` (96, luego 0) — más razón para no usar `kp` como palanca acá. Con
  `CAL_TOL_COUNTS=450` (mismo `kp=4`): converge en 11 iters, `best_dac=52` (igual
  que antes) — confirma que **subir tol, no tocar PID**, es el remedio correcto.
- Nota al margen (no investigado más): `kp=8` para `gain=1000` da
  `gain·kp/4096=1.95<2` (formalmente "estable" por el criterio lineal) pero
  `best_dac=96` (lejísimos de 52) — el criterio de estabilidad puramente
  proporcional no captura el término integral (`KI_Q12=1`); no bloquea nada, queda
  como curiosidad para quien quiera afinar el modelo.

### Cómo reusar `cal_sim.py` en la sesión de hardware

1. Medir, para una etapa, `measured(dac)` en al menos 2 códigos DAC distintos (ej.
   `dac=64` y `dac=192`) -> `gain≈(measured(192)-measured(64))/(192-64)`,
   `offset≈measured(64)-gain*64`.
2. `from cal_sim import calibrate_stage, make_linear_plant, true_optimum` y correr
   `r = calibrate_stage(target_counts, direction, make_linear_plant(gain, offset))`
   con el `target_counts`/`direction`/`tol` reales de esa etapa.
3. Si da FAIL, llamar `true_dac, true_err = true_optimum(target_counts, plant)` y
   comparar con `r.best_dac` (algoritmo completo en "Implicación para hardware" de
   §11.A):
   - `r.best_dac == true_dac` → **RESOLUCIÓN**: subir `CAL_TOL_COUNTS` de esa etapa
     por encima de `true_err`, o cambiar la ganancia física. Retunear PID no sirve.
   - `r.best_dac != true_dac` → **DINÁMICA**: bajar `KP_Q12` de esa etapa
     (`KP_Q12 < 2·4096/gain`) y volver a correr `calibrate_stage(..., kp=nuevo)`.
   - Si `r.history[-1]` tiene `dac_code` en 0 o 255 → revisar signo de `direction`
     (§11.B) primero.

### (H) Sensibilidad a ruido del ADC con `CAL_AVG_N=32` (`tools/cal_sim.py`, experimento (H))

`make_noisy_plant()` (nuevo, opt-in) reproduce `avg_counts()` (calibration.c:42-51):
cada una de las `avg_n=32` "lecturas" individuales = `plant(dac)` + ruido gaussiano
`N(0,sigma)` counts, clippeada a `[0, ADC_FULL_SCALE]`, promediada con `//` (igual a
`acc / (int32)n` de C para acumulador no-negativo). `sigma=0` reproduce el baseline
determinista byte-a-byte (verificado como sanity check). (A)-(G) NO cambian (siguen
usando `make_linear_plant` sin ruido, deterministas).

Barrido `sigma ∈ {5,15,30,60,125,250,500,1000}` counts (desv. estándar POR LECTURA
INDIVIDUAL antes de promediar — el promedio de 32 reduce el ruido efectivo del
`measured` en `√32≈5.66×`), 5 semillas por punto, para las 3 ganancias
representativas:

| gain | caso | true_dac (true\|err\|) | baseline sin ruido | best_dac con ruido (40 corridas) | converged con ruido |
|---|---|---|---|---|---|
| 300  | (B) direction, reachable | 175 (71≤tol) | OK iters=18, best_dac=175 | **175** en 36/40; **174/176** (±1) solo en sigma∈{500,1000} | **siempre OK**, iters 17-19 |
| 1000 | RESOLUCIÓN (A)/(G) | 52 (429>tol) | FAIL iters=90, best_dac=52 | **52** en 39/40; **53** (±1) solo 1 caso, sigma=1000 | FAIL en sigma≤250 (30/30); **2/5 OK a sigma=500; 5/5 OK a sigma=1000** (ver hallazgo 3) |
| 2100 | DINÁMICA (G), kp=4 | 25 (71≤tol) | FAIL iters=90, best_dac=32 | **32** en 40/40, sin excepción | **siempre FAIL**, sin excepción |

**Hallazgos:**

1. **`best_dac` es robusto al ruido en los 3 casos.** Para gain=300 y gain=2100,
   `best_dac` no se mueve más de ±1 LSB (gain=300, solo a sigma≥500) o no se mueve
   en absoluto (gain=2100) en 40 corridas con sigma hasta 1000 counts/lectura
   (~0.76% de `ADC_FULL_SCALE=131071`). Para gain=1000, `best_dac=52=true_dac` en
   39/40 corridas. Conclusión: **`CAL_AVG_N=32` ya alcanza para que la elección de
   `best_dac` sea robusta** frente a ruido de ADC en rangos realistas (sigma de
   pocas a pocas decenas de counts para un ADC de ~17 bits); recién a sigma≥500
   (~0.4-0.8% FS) aparecen desvíos ocasionales de ±1 LSB.
   - Nota aparte: `best_abs_error` (el residuo reportado) SÍ fluctúa bastante con
     sigma incluso cuando `best_dac` no cambia (p.ej. gain=300: 12-249 vs baseline
     71) — es una medición ruidosa de un único `avg_counts()`, no el error
     "verdadero". Relevante si se implementa la sugerencia §10.1 (debug-print de
     `best_abs_error` por etapa): un solo valor puede no ser representativo.

2. **DINÁMICA (gain=2100) totalmente insensible al ruido**: `best_dac=32` y
   `best_|err|∈[14551,14939]` (vs baseline 14771, ~58-60×`tol`) en las 40 corridas,
   sin una sola excepción. El remedio de §11.G (bajar `KP_Q12<2·4096/gain≈3.90`)
   sigue siendo el ÚNICO factor relevante — la oscilación del PID es ~2 órdenes de
   magnitud mayor que cualquier ruido de ADC plausible, así que el ruido es
   estadísticamente invisible frente a la dinámica.

3. **NUEVO RIESGO para RESOLUCIÓN (gain=1000) — falso "OK" por ruido, amplía §10**:
   a partir de sigma≈500 (y con certeza a sigma=1000), el bit `ok` de CALIBRATE
   puede dar `1` ("Calibracion OK") para una etapa cuyo `best_dac` real sigue siendo
   el mismo `52` con `true_|err|=429>tol=250` de siempre — simplemente porque, en
   ALGUNA de las 90 iteraciones, el promedio de 32 lecturas ruidosas cayó por azar
   dentro de `tol` del target (`abs_error<=tol` dispara el `return` anticipado,
   calibration.c:71-72).
   - Orden de magnitud: el ruido post-promedio es `sigma/√32`. Para que un promedio
     entre dentro de `tol` hace falta un salto de `(true_|err|-tol)=179` counts en
     la dirección favorable — un evento de `179/(sigma/√32)` desviaciones estándar
     (one-sided). Para sigma=1000 (`σ/√32≈177`): ~1.01σ → P≈16%/iteración →
     P(≥1 en 90 iters)≈100% (observado: 5/5). Para sigma=500 (`σ/√32≈88`): ~2.03σ →
     P≈2%/iteración → P(≥1 en 90)≈85% (observado: 2/5 — mismo orden, pero n=5 tiene
     mucha varianza).
   - **Esto AMPLÍA §10**: no solo `ok=0` conflactúa 3 causas distintas — además
     **`ok=1` tampoco garantiza que la etapa esté dentro de `tol`** si el ruido de
     ADC de esa etapa es alto (umbral aprox. `sigma≳(true_|err|-tol)·√avg_n`, para
     RESOLUCIÓN típico `true_|err|≈71-429` eso es del orden de cientos a ~mil
     counts — solo plausible con un frontend muy ruidoso). No bloquea nada, pero en
     bring-up: si "Calibrar" da OK en una etapa pero los valores no son repetibles
     entre corridas, repetir el botón y comparar `best_dac`/`best_abs_error` (si
     hay instrumentación, §10.1) es más confiable que un único bit `ok`.

---

## 12. HALLAZGO CRÍTICO (2026-06-13): la calibración de boot cuelga el PSoC para siempre — `ADC_convDone` nunca se setea en CFG2 porque `isr_DelSig_Disable()` enmascara también la interrupción de CFG2

**Síntoma reportado**: "no tengo señal de vida del PSoC desde que pusimos lo de
autocalibrar" — el ESP esclavo1 nunca recibe ni un byte por UART del PSoC
(`uartBytes=0`).

**Confirmado empíricamente esta sesión**: captura de 90s de COM12 (slave1) desde
el reset (cada apertura de `pio device monitor` resetea el ESP por DTR/RTS) →
`uartBytes=0` durante TODO el intervalo, con `HELLO` repitiéndose cada 2s y
`STATUS` cada 10s, todos mostrando `uartBytes=0`. La calibración, en el peor caso
documentado en §11.D/E, tarda ~20.4s (con margen hasta el timeout `0xB5` de 25s,
§5). 90 segundos sin un solo byte es incompatible con "calibración lenta": es un
**cuelgue permanente**, no una demora.

**Mecanismo — verificado línea por línea contra el código actual (no memoria),
ambos "hechos clave" confirmados antes de escribir esto**:

1. `main.c:584-585` — `CyDelay(PSOC_STARTUP_CAL_DELAY_MS)` (5000ms) seguido de
   `psoc_run_calibration_if_idle()` → `psoc_calibration_run_blocking()`, ANTES de
   `wait_for_esp()` (main.c:588). Si esto cuelga, el PSoC nunca llega a
   `uart_send_ping()` (main.c:548) — coincide con `uartBytes=0`.
2. `calibration.c:132` — `isr_DelSig_Disable()`: enmascara (`CyIntDisable`) el
   ÚNICO vector de interrupción que comparte el bloque ADC (`ADC_INTC_NUMBER`).
   §6 documentó esto como correcto para que la ISR de CFG1 no interfiera con el
   `StartConvert/StopConvert` propio de CFG2 — pero §6 no consideró que ese mismo
   enmascarado también desactiva la señal que CFG2 necesita para sí mismo.
3. `calibration.c:139` → `psoc_adc_select_calibration_config()`
   (psoc_adc.c:28-43, rama `ADC_DEFAULT_NUM_CONFIGS>=2` activa, confirmado):
   `ADC_SelectConfiguration(ADC_CFG2, 0u); ADC_Start();` (psoc_adc.c:32-33).
   - `ADC_SelectConfiguration(ADC_CFG2,...)` (Generated_Source/PSoC5/ADC.c:
     1355-1394) repone el vector a `ADC_ISR2` — pero NO toca el enable/mask del
     NVIC (`CyIntSetVector` y `CyIntEnable`/`CyIntDisable` son independientes).
   - `ADC_Start()` llama `ADC_Init()` solo la primera vez (`ADC_initVar`, ya
     consumido en el boot por `psoc_adc_select_capture_config()`,
     psoc_adc.c:20-26 → CFG1). `CyIntEnable(ADC_INTC_NUMBER)` (ADC.c:299) está
     DENTRO de `ADC_Init()` — es el **ÚNICO** `CyIntEnable` en todo `ADC.c`
     (grep confirmado, 1 sola ocurrencia). En esta segunda llamada `ADC_Start()`
     solo corre `ADC_Enable()`, que NO re-habilita la interrupción.
   - **Resultado: tras este paso, la interrupción del ADC queda enmascarada y
     nada la vuelve a habilitar antes de usar CFG2.**
4. **Fact A confirmada** (ADC.h:1500-1501,955,973): `ADC_CFG2_RESOLUTION=18u`
   (>16) y `ADC_CFG2_CONV_MODE=0u` (`ADC_MODE_SINGLE_SAMPLE`) → en
   `ADC_IsEndConversion()` se activa, para CFG2, la rama `ADC_stopConversion!=0`
   (polling por SOFTWARE de `ADC_convDone`), NO la rama que lee directo el
   registro de hardware `ADC_DEC_SR_REG` (esa rama sí funcionaría sin importar el
   enmascarado del NVIC).
5. `ADC_convDone` (ADC.c:36, `volatile uint8 ADC_convDone = 0u`) se resetea a
   `0u` dentro de `ADC_StartConvert()` (ADC.c, una de las líneas 720/783/870
   según config) y `ADC_DEC_CONV_DONE = 0x01u` (ADC.h:973). El único código que
   puede escribir `ADC_convDone = ADC_DEC_CONV_DONE` es el handler de
   interrupción del ADC (`ADC_ISR2` en CFG2) — que está enmascarado por el
   paso 3.
6. `calibration.c:67` — primera iteración de `calibrate_stage()` (etapa 0,
   iter 0) → `avg_counts()` → `psoc_adc_read_single_counts()`
   (psoc_adc.c:45-53):
   - `psoc_adc.c:48` `ADC_StartConvert()` → `ADC_convDone=0u`.
   - `psoc_adc.c:49` `(void)ADC_IsEndConversion(ADC_WAIT_FOR_RESULT)` →
     `do { status = ADC_convDone; } while (status != ADC_DEC_CONV_DONE && ...)`.
     Como `ADC_ISR2` nunca puede ejecutarse (paso 3), `ADC_convDone` queda en
     `0u` para siempre → `0u != 0x01u` → **bucle infinito, aquí mismo**, ~5s
     después del reset, en la primera lectura de la primera etapa.

`calibration.c:155` (`isr_DelSig_Enable()`), `main.c:588` (`wait_for_esp()`) y
`main.c:548` (primer `uart_send_ping()`) nunca se alcanzan.

**Para confirmar en hardware (sin tocar lógica, breakpoints en PSoC Creator)**:
requiere build en Debug + programador/debugger conectado (KitProg/MiniProg3).
Un solo breakpoint alcanza:
- **`psoc_adc.c:49`** (`ADC_IsEndConversion(ADC_WAIT_FOR_RESULT)`). A los ~5s del
  reset debería quedar detenido DENTRO del `do/while` de
  `Generated_Source/PSoC5/ADC.c` (~líneas 653-666), con `ADC_convDone==0u` y
  `ADC_stopConversion!=0` en la ventana de watch — y `calibration.c:155` /
  `main.c:588` nunca se alcanzan (poner breakpoints ahí también deja confirmarlo
  por ausencia de hit).
- Alternativa de cero-instrumentación: el patrón de parpadeo de LED de
  `wait_for_esp()` (main.c, tras línea 588) nunca debería arrancar si esto está
  confirmado — si el LED de boot se queda fijo/apagado tras los primeros ~5s, es
  consistente con este hallazgo.

**✅ FIX APLICADO 2026-06-13 (pendiente compilar+programar+verificar en
hardware)**: se agregó `isr_DelSig_Enable();` inmediatamente después de
`ADC_Start();` en `psoc_adc_select_calibration_config()` (psoc_adc.c:32-36,
rama `ADC_DEFAULT_NUM_CONFIGS>=2`), para re-armar la interrupción antes de que
CFG2 la necesite. Esa interrupción queda habilitada durante TODA la secuencia
de calibración (todas las etapas/iteraciones de `calibrate_stage()`), ya que
nada vuelve a deshabilitarla hasta `calibration.c:155`
(`isr_DelSig_Enable()`, ahora redundante pero inofensivo) seguido de
`psoc_adc_select_capture_config()` (restaura CFG1, que ya funcionaba con la
interrupción habilitada desde el boot original). No se tocó la rama `#else`
(CFG1 fallback, NO activa — `ADC_DEFAULT_NUM_CONFIGS=2u` confirmado) ni ningún
otro safeguard de §6 (`isr_SyncIn_Disable()` etc.) ni la lógica del PID/tablas.

**Verificación pendiente con hardware**: compilar, programar el PSoC, y
re-capturar el log de COM12 (slave1) por ~30-40s desde el reset. Esperado si el
fix es correcto: `uartBytes>0` dentro de los primeros ~5-25s (el PSoC manda
`uart_send_ping()` cada 700ms una vez que llega a `wait_for_esp()`,
main.c:548/588). Si `uartBytes` sigue en 0 tras 40s, el cuelgue no era (solo)
este — repetir el breakpoint en `psoc_adc.c:49` para ver en qué línea quedó
parado ahora.

**Conclusión práctica**: no hace falta aumentar `CAL_AVG_N=32` salvo que el ruido
real medido en hardware (`psoc_adc_read_single_counts()` repetido, ver §9) resulte
ser ≳cientos de counts por lectura individual. Si se mide un sigma así de alto, la
prioridad pasa a ser reducir el ruido analógico (blindaje, referencias, layout), no
a tunear más el PID o subir `CAL_AVG_N`.

---

← `[[project_psoc_calibration]]` (memoria persistente, índice de alto nivel)
→ próxima entrada: §9 (checklist de bring-up con hardware) sigue siendo lo único que
  REQUIERE placa — todo el resto (§§0-8, 10-11, incluyendo §2.1 y el nuevo §11.H)
  está cerrado para esta noche. Si hay más tiempo de loop antes del bring-up:
  1. ✅ HECHO (§11.H): extendido `cal_sim.py` con media móvil + ruido gaussiano
     simulado en el ADC (`make_noisy_plant`, opt-in, (A)-(G) sin cambios). Resultado:
     `best_dac` robusto en los 3 casos representativos (gain=300/1000/2100) hasta
     sigma=1000 counts/lectura; nuevo riesgo identificado de falso `ok=1` en el caso
     RESOLUCIÓN (gain=1000) a sigma≥500 — amplía el hallazgo de §10 sobre el bit `ok`.
  2. Si se confirma "2 `.cydsn` separados" (§2.1/§7.2/§7.8) como dirección: redactar
     (solo documentación, sin tocar PSoC Creator) un borrador de checklist de
     migración — qué archivos `.c/.h` de `calibration_tables.h`/`psoc_hw.*` cambian
     vs quedan igual entre variantes Geo/Hammer, y qué Generated_Source de Hammer
     (§2.1: `PGA`, `VDAC_ref_IN/PGA/LP`, `VDAC_refIn`, etc.) ya existe como
     referencia aunque haya que re-colocarlo.
  3. Repasar `main.c` completo línea por línea (boot sequence, `g_state` machine)
     buscando más hallazgos tipo §2.1/§9 (mismo método: cruzar contra
     `Generated_Source/PSoC5` y el `.rpt`).

---

## 13. Reescritura 2026-06-14: arquitectura final (CFG1 único continuo + calibración por búsqueda binaria)

Dos cambios de arquitectura, hechos en sesiones consecutivas, reemplazan TODO lo
descrito en §0-§12:

1. **Un solo ADC config (CFG1, continuo)** — se eliminó CFG2 y todo el switch de
   configs (sesión 2026-06-13, ver §13.1).
2. **Calibración por búsqueda binaria desde 2.5V**, reemplazando el PID (sesión
   2026-06-14, ver §13.3).

### 13.1 ADC: un solo config continuo, ISR compartida captura/calibración

- `psoc_adc.h/.c` ya NO tienen `psoc_adc_select_calibration_config()` ni
  `psoc_adc_read_single_counts()`. La única función de lectura es
  `psoc_adc_wait_fresh_sample(int32 *out_counts, uint16 timeout_us)`
  (`psoc_adc.c`): espera a que la ISR deposite una muestra nueva vía
  `psoc_adc_note_isr_sample()` — la MISMA ruta que usa la captura normal — con
  timeout en µs (`CAL_ADC_READ_TIMEOUT_US=20000`).
- `psoc_adc_select_capture_config()` ahora solo hace
  `ADC_StopConvert(); ADC_Start(); ADC_StopConvert();` — ya no "selecciona" nada
  (no hay nada que seleccionar), solo asegura que el componente ADC esté
  inicializado.
- `isr_DelSig` queda **habilitado todo el tiempo**, incluso durante calibración —
  `calibration.c` ya NO llama `isr_DelSig_Disable()`/`_Enable()` en ningún punto.
- `psoc_calibration_run_blocking()` (`calibration.c`): `isr_SyncIn_Disable()` se
  mantiene durante TODA la calibración (se reactiva al final, junto con
  `isr_SyncIn_ClearPending()`). El ADC (`ADC_StopConvert`/`ADC_StartConvert`) se
  toca solo en los bordes — se detiene, se cambia el AMux de entrada a GND y el de
  salida al canal de captura, se limpia el estado de la ISR
  (`psoc_adc_clear_isr_sample()` + `isr_DelSig_ClearPending()`) y se arranca UNA
  sola vez (`ADC_StartConvert()`) antes del bucle de 4 etapas; durante las etapas
  el ADC corre en continuo sin pararse. Al final, `ADC_StopConvert()` +
  `psoc_calibration_restore_capture_path()` (vuelve a poner el AMux en el canal de
  captura normal).

> **⚠️ CRÍTICO para tu rebuild "una sola config" en PSoC Creator**: la config CFG1
> que sobreviva DEBE quedar en **Conversion Mode = Continuous** (la misma que ya
> usa la captura hoy). Si por accidente queda en Single Sample, el
> `ADC_StartConvert()` único de `psoc_calibration_run_blocking()` dispara como
> mucho una conversión y no más — `psoc_adc_wait_fresh_sample()` nunca vuelve a
> ver una muestra nueva. Ver §13.5 para la firma exacta de este fallo en la
> telemetría (es FACIL de distinguir: la calibración entera dura un puñado de
> ms en vez de segundos).

### 13.2 Targets GEO = 0 (sin cambios desde 2026-06-13, repetido por contexto)

`ADC_CFG1` es diferencial signed de 18 bits → **0 counts = 0V diferencial**. En
modo GEO cada etapa debe quedar con su salida DC igualada al canal de
referencia/tierra del `AMux_IN` (`CAL_INPUT_GROUND_CHANNEL`), es decir lectura
diferencial = 0. Por eso `CAL_TARGET_GEO_*_COUNTS = 0L` para las 4 etapas GEO
(`calibration_tables.h`). HAMMER conserva sus targets en counts (`52429`≈1V,
`78644`≈1.5V) sin cambios — esa rama sigue siendo código muerto (§2: `PSOC_HW_CLASS`
resuelve siempre a GEO en este `Generated_Source`).

### 13.3 Algoritmo nuevo: búsqueda binaria desde 2.5V (`calibrate_stage()`, `calibration.c`)

Reemplaza por completo el PID. Para cada etapa (`PsocCalStage`, una de
GEO_PGA/GEO_BP/GEO_ADDER/GEO_LP):

1. **Arranca SIEMPRE en `CAL_DAC_INIT = 0x9C` (156 = 2.5V)**, sin importar el
   estado previo (`156 × 16mV` paso del VDAC8 1x `≈ 2.496V ≈ 2.5V`) — el punto
   medio del rango del VDAC8, asumido "tierra virtual" del front-end (consistente
   con `target_counts=0`).
2. Mide con `stable_avg()` (§13.3.1). Si ya está dentro de `tolerance_counts`
   (`CAL_TOL_COUNTS=250`), termina OK ahí mismo.
3. **Primer paso ("probe")**: se mueve `CAL_PROBE_STEP=32` códigos DAC en la
   dirección de `stage->direction` (con guarda de borde para no salir de
   `[0,255]`), mide de nuevo, y compara ambas medidas para determinar
   **empíricamente** si la lectura del ADC **crece o decrece** con el código DAC
   (`increasing`). `stage->direction` ahora es SOLO la semilla de "hacia dónde
   probar primero" — ya no es un signo de PID (hoy todas las filas, GEO y HAMMER,
   tienen `direction=1`). Si esta 2ª medida ya entra en tolerancia, termina OK.
4. **Búsqueda binaria sobre `[0,255]`** usando `increasing`: en cada iteración se
   decide `go_up` según `increasing` y si `measured` está arriba/abajo de
   `target_counts`, se ajusta `lo`/`hi`, se recalcula
   `next_dac = lo + (hi-lo)/2` y se mide de nuevo. Termina por: (a)
   `abs_error <= tolerance_counts` → OK; (b) `lo>hi`, `next_dac==dac`, o tocar el
   borde 0/255 sin poder seguir → FAIL (se aplica `best_dac`, el código con menor
   `|error|` visto hasta ahora en TODA la corrida); (c) `max_iter` agotado
   (`CAL_MAX_ITER=12` — `log2(256)=8` bisecciones bastan para `[0,255]`, 12 da
   margen).
5. Cualquier falla de lectura (`stable_avg`→`avg_counts`→
   `psoc_adc_wait_fresh_sample` timeout) aborta la etapa de inmediato, escribe
   `best_dac` y marca `ok=0` con `final_measured=0x7FFF` (sentinel, ver §13.5).

Nota de diseño (revisada): la búsqueda "olvida" los dos puntos ya medidos
(`CAL_DAC_INIT` y el probe) al arrancar `[lo,hi]=[0,255]` — cuesta como máximo
1-2 iteraciones extra, no afecta corrección, y simplifica mucho la lógica de
bordes. `best_dac`/`best_measured`/`best_abs_error` se actualizan en CADA medida
de toda la corrida (probe incluido), así que el fallback en caso de FAIL siempre
es "el mejor punto visto", no necesariamente el último.

#### 13.3.1 `stable_avg()` — "esperar a que estabilice, luego promediar"

`avg_counts(n, ...)` (sin cambios) promedia `n=CAL_AVG_N=32` muestras frescas vía
`psoc_adc_wait_fresh_sample` — si UNA sola muestra da timeout, `avg_counts`
devuelve 0 de inmediato (no completa la ventana). `stable_avg` llama a
`avg_counts` una primera vez (`prev_avg`) y luego repite hasta
`CAL_SETTLE_MAX_WINDOWS=10` ventanas más, comparando ventana actual vs anterior;
si la diferencia cae a `<=CAL_SETTLE_TOL_COUNTS=100` ("solo ruido de
cuantización"), corta ahí y usa esa ventana. Si nunca estabiliza, usa la última
ventana medida (no se cuelga). Si la primera `avg_counts` falla, `stable_avg`
falla de inmediato (sin reintentar) — esto es lo que da la "salida rápida" del
§13.5 cuando el ADC no entrega muestras.

#### 13.3.2 Constantes nuevas — TODAS son estimaciones iniciales, esperar telemetría real

| Constante | Valor | Significado | Si hay que ajustar... |
|---|---|---|---|
| `CAL_DAC_INIT` | `0x9C` (156) | Punto de partida obligatorio (2.5V) | Fijo por requerimiento, no tocar |
| `CAL_PROBE_STEP` | `32` | Paso del primer movimiento (detección de pendiente) | Si una etapa de alta ganancia ya satura en el probe, bajarlo da una pendiente más representativa (el algoritmo sigue siendo correcto en el riel: si ambas medidas del probe son iguales, cae a `stage->direction`) |
| `CAL_SETTLE_MAX_WINDOWS` | `10` | Tope de ventanas de 32 muestras esperando estabilización | Si la calibración tarda demasiado, bajarlo (degrada a "usar última ventana", no se rompe) |
| `CAL_SETTLE_TOL_COUNTS` | `100` | Diferencia máx. entre ventanas consecutivas para considerar "estable" | Si repetir "Calibrar" da `final_dac`/`final_measured` muy distintos entre corridas (nunca estabiliza de verdad), subir |
| `CAL_MAX_ITER` | `12` | Tope de iteraciones de bisección (antes 90, era para el PID) | 8 bisecciones bastan para `[0,255]`; 12 da margen. No debería necesitar subirse |
| `CAL_TOL_COUNTS` | `250` | Tolerancia de convergencia (sin cambios) | — |
| `CAL_AVG_N` | `32` | Muestras por ventana (sin cambios) | — |

### 13.4 Cambios de estructura

- **`PsocCalStage`** (`calibration.h`) pasó de 12 campos a 8: se eliminaron
  `kp_q12`, `ki_q12`, `kd_q12` (PID) e `initial_dac` (reemplazado por la constante
  global `CAL_DAC_INIT`, igual para todas las etapas). Orden actual: `{ name,
  adc_channel, target_counts, avg_n, max_iter, tolerance_counts, direction, write
  }`. `direction` cambió de "signo del PID" a "semilla de dirección del probe"
  (§13.3 punto 3).
- **`g_psoc_cal_stages[]`** (GEO y HAMMER, `calibration_tables.h`) reformateadas a
  8 columnas.
- **Nuevo `PsocCalResult`** (`calibration.h`): `{ uint8 final_dac; int32
  final_measured; uint8 ok; }`. Array fijo
  `g_psoc_cal_results[PSOC_CAL_MAX_STAGES=4]` + `g_psoc_cal_result_count`
  (definidos en `calibration.c`, llenados por `psoc_calibration_run_blocking()`).
  `PSOC_CAL_MAX_STAGES=4` ≥ `PSOC_CAL_STAGE_COUNT` para GEO (4) y HAMMER (3) —
  evita incluir `calibration_tables.h` desde `calibration.h` (circular).

### 13.5 Telemetría nueva: `PSOC_EVT_CAL_STAGE_DAC` (0x13) / `PSOC_EVT_CAL_STAGE_MEAS` (0x14)

Motivación: esta es la PRIMERA vez que corre este algoritmo en hardware real —
target=0, punto de arranque 0x9C y `stable_avg` nunca se probaron. Si falla,
necesitamos ver NÚMEROS, no solo `ok/fail`.

`psoc_run_calibration_if_idle()` (`main.c`), después de
`psoc_calibration_run_blocking()` y ANTES de `PSOC_EVT_CAL_DONE`, emite por cada
etapa (`g_psoc_cal_result_count`, en orden GEO_PGA→GEO_BP→GEO_ADDER→GEO_LP):

1. `uart_send_diag(PSOC_EVT_CAL_STAGE_DAC, final_dac)` — 1 byte, código DAC final
   (0-255).
2. `uart_send_cal_stage_meas(final_measured)` (helper nuevo en `main.c`) — 2
   eventos `PSOC_EVT_CAL_STAGE_MEAS` consecutivos: HI luego LO de `final_measured`
   saturado a `int16` (`[-32768,32767]`) y reinterpretado como `uint16` para los
   dos bytes.

Secuencia completa por calibración: `CAL_START, (DAC,MEAS_hi,MEAS_lo)×4,
CAL_DONE`. El slave ESP imprime estos eventos sin cambios adicionales más allá de
agregar los nombres `CAL_STAGE_DAC`/`CAL_STAGE_MEAS` a `psocDiagName()`
(`slave/src/main.cpp`) y las constantes correspondientes en `slave/src/psoc_uart.h`
— `onPsocDiag()` ya logueaba `event.value` genéricamente, así que los nuevos
eventos aparecen solos en el log de COM10.

**Cómo leer `final_measured` (target=0 para GEO ⇒ `final_measured` ES el error con
signo, en counts):**

- **`≈0` (dentro de ±250)** → etapa convergió, `ok=1` (consistente con
  `CAL_DONE val=1` solo si las 4 etapas dieron esto).
- **Valor moderado (cientos/miles), no saturado** → no convergió pero el
  front-end responde y el formato de datos es razonable. Mirar `final_dac`: si
  quedó en 0 o 255 (riel) → revisar `direction`/ganancia de esa etapa
  específica; si quedó en un valor intermedio → puede ser que
  `CAL_TOL_COUNTS`/`CAL_SETTLE_TOL_COUNTS` estén ajustados de forma muy estricta
  para el ruido real — subirlos.
- **`0x7FFF` (32767) exacto en TODAS las etapas, con `final_dac=0x9C` en las 4,
  Y la secuencia `CAL_START..CAL_DONE` casi instantánea (cientos de ms, NO
  segundos)** → la primera muestra de cada etapa ya dio timeout
  (`psoc_adc_wait_fresh_sample`, ~20ms) y la etapa abortó antes de moverse del
  punto de partida. El algoritmo nunca llegó a ejecutarse de verdad — **sospechar
  primero CFG1 en Single Sample** (§13.1), no un bug del algoritmo.
- **`±32767`/`0x7FFF` en UNA sola etapa, con `final_dac` distinto de `0x9C` y
  timing normal (segundos)** → saturación analógica real de esa etapa (su salida
  está en un riel físico fuera de rango del ADC) — investigar el front-end de esa
  etapa puntual, no el algoritmo de calibración ni la config del ADC.

### 13.6 §12 retractado

El hallazgo de §12 (`isr_DelSig_Disable()` enmascara la interrupción que CFG2
necesita → cuelgue permanente) **ya no aplica**: no existe CFG2, no existe
`psoc_adc_select_calibration_config()`, y `calibration.c` ya no llama
`isr_DelSig_Disable()`/`isr_DelSig_Enable()` en ningún punto. El fix de 1 línea
que §12 proponía (`isr_DelSig_Enable()` en `psoc_adc.c`) quedó obsoleto junto con
el código que parcheaba — no se aplicó (ni hace falta) en esta arquitectura.

### 13.7 Archivos tocados (resumen para codex)

| Archivo | Cambio |
|---|---|
| `calibration.h` | `PsocCalStage` de 12→8 campos (sin PID/`initial_dac`); nuevo `PsocCalResult` + `PSOC_CAL_MAX_STAGES=4` + externs `g_psoc_cal_results[]`/`g_psoc_cal_result_count` |
| `calibration_tables.h` | `CAL_DAC_INIT` 128→`0x9C`; quitadas `CAL_PID_KP/KI/KD_Q12`; nuevas `CAL_PROBE_STEP=32`, `CAL_SETTLE_MAX_WINDOWS=10`, `CAL_SETTLE_TOL_COUNTS=100`; `CAL_MAX_ITER` 90→12; tablas GEO/HAMMER reformateadas a 8 columnas |
| `calibration.c` | quitados `clamp_dac_code`/`clamp_integral`/`CAL_INTEGRAL_LIMIT`; nuevo `stable_avg()`; `calibrate_stage()` reescrito completo (búsqueda binaria, firma `(stage, result)`); `psoc_calibration_start_references()` usa `CAL_DAC_INIT` en vez de `initial_dac`; `psoc_calibration_run_blocking()` llena `g_psoc_cal_results[]` |
| `psoc_hw.h` | nuevos `PSOC_EVT_CAL_STAGE_DAC=0x13`, `PSOC_EVT_CAL_STAGE_MEAS=0x14` |
| `main.c` | nuevo helper `uart_send_cal_stage_meas()`; `psoc_run_calibration_if_idle()` emite telemetría por etapa antes de `CAL_DONE` |
| `slave/src/psoc_uart.h` | mismas 2 constantes `PSOC_EVT_CAL_STAGE_DAC/MEAS` espejadas |
| `slave/src/main.cpp` | `psocDiagName()`: nombres para los 2 eventos nuevos |
| `tools/cal_sim.py` | **STALE** — modela el PID viejo (`calibrate_stage` con la firma/lógica anterior). No reusar para el algoritmo nuevo sin reescribirlo de cero. |

### 13.8 Procedimiento de prueba sugerido (primer flash)

1. Confirmar en PSoC Creator que el único ADC config quede en **Continuous**
   (§13.1) antes de programar.
2. Programar, abrir monitor serie del slave (COM10) durante el boot
   (`PSOC_STARTUP_CAL_DELAY_MS=5000` antes de que arranque la calibración de
   boot).
3. Buscar la secuencia `CAL_START ... CAL_STAGE_DAC ... CAL_STAGE_MEAS ×2 ...
   (×4 etapas) ... CAL_DONE`, y tomar nota de cuánto tardó esa secuencia
   (instantánea vs. segundos — ver §13.5).
4. Reconstruir por etapa: `final_dac` = valor de `CAL_STAGE_DAC`; `final_measured`
   = `(MEAS_hi<<8 | MEAS_lo)` reinterpretado como `int16`.
5. Interpretar con la tabla de §13.5. Si `CAL_DONE val=1`, las 4 etapas
   convergieron — repetir el botón "Calibrar" (`0xB5`) una o dos veces más y
   comparar `final_dac`/`final_measured` entre corridas para chequear
   repetibilidad (si varía mucho, `CAL_SETTLE_TOL_COUNTS` puede ser muy laxo).

### 13.9 Pendientes (no tocados en esta reescritura)

- Issues de sesiones previas no relacionados con el algoritmo de calibración:
  confirmación de ACK del PGA, `fs=?Hz` reportado, líneas `PSOC_EVT` faltantes,
  fallback `VIEW_SYNC_STALL` — quedan como estaban, sin tocar acá.
- `tools/cal_sim.py` y todo §11 (simulación PID) — archivados, no migrados al
  algoritmo nuevo. Si se quiere una simulación equivalente para búsqueda binaria,
  habría que escribirla de cero (no es una migración 1:1 trivial como la del PID).
- Compilación del firmware PSoC (`AcondicionamientoAnalogico.cydsn`) — no se puede
  hacer desde este entorno (requiere PSoC Creator). El repo solo tiene
  `.lst`/`.o` de un build anterior (arquitectura PID vieja) en
  `CortexM3/ARM_GCC_541/Debug/` — quedarán desactualizados hasta el próximo build
  real.
