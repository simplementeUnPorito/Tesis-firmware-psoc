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

## 14. Primera corrida en hardware real (2026-06-14)

### 14.0 Contexto

§§13.4-13.5 fueron escritas describiendo un `psoc_calibration_run_blocking()`
que en el momento de escribirlas todavía no existía en código — la
implementación real (`calibration.c` actual) ya es la **máquina de estados
asíncrona** (`psoc_calibration_start_async()` /
`psoc_calibration_service_async()`, estados `CAL_ASYNC_IDLE` →
`CAL_ASYNC_DONE`). Esta sección documenta la primera corrida real de esa
máquina, capturada en COM12 el 2026-06-14 (mismo log que motivó la corrección
en `BUILD_PROGRAM_PSOC.md`: la comunicación ESP↔PSoC funciona bien, el ciclo
de captura de 100 batches completó OK, y luego se disparó `0xB5` —
"Calibrar").

**Corrección a §13.5**: la telemetría `CAL_STAGE_DAC`/`CAL_STAGE_MEAS` no se
emite solo al final de cada etapa. `cal_diag_point()` (línea 45-49 de
`calibration.c`) se llama desde `async_measure_service()` por **cada
medición** que termina su settle/promediado (`EVAL_INIT`, `EVAL_PROBE`, cada
`EVAL_ITER` de la bisección, y la pasada de `VERIFY`). Además,
`async_finish_stage()` vuelve a emitir `CAL_STAGE_DAC`/`CAL_STAGE_MEAS` con
`final_dac`/`final_measured`, seguido de `CAL_STAGE_OK` (0x16). Es decir, la
secuencia real por etapa es:

```
CAL_STAGE_BEGIN(stage_index)
  (DAC, MEAS_hi, MEAS_lo)        <- EVAL_INIT (dac=CAL_DAC_INIT=0x9C)
  (DAC, MEAS_hi, MEAS_lo)        <- EVAL_PROBE (dac=INIT±CAL_PROBE_STEP)
  (DAC, MEAS_hi, MEAS_lo) x N    <- EVAL_ITER, una por cada paso de bisección (hasta CAL_MAX_ITER)
  (DAC, MEAS_hi, MEAS_lo)        <- final_dac/final_measured (async_finish_stage)
  CAL_STAGE_OK(ok)
```

y al final, tras las 4 etapas, una pasada de verificación:

```
CAL_VERIFY_BEGIN(stage_index)
  (DAC, MEAS_hi, MEAS_lo)        <- medición con final_dac y verify_settle_samples
  CAL_VERIFY_OK(ok)
```
repetido para `stage_index=0..3`, y solo entonces `CAL_DONE(val)`
(`val = g_cal_async.ok`, AND de los 4 `ok` de etapa y los 4 `verify_ok`).

Esto es BUENA noticia: la telemetría es mucho más rica de lo que §13.5
anticipaba — se puede reconstruir la trayectoria completa de la búsqueda
binaria, no solo el punto final.

### 14.1 GEO_PGA (stage 0): primera convergencia confirmada en hardware

Decodificando la secuencia `(DAC, MEAS_hi<<8|MEAS_lo como int16)` para
`stage_index=0` del log pegado por el usuario:

| paso | dac | measured | abs_error vs target=0 |
|---|---|---|---|
| EVAL_INIT  | 156 (`0x9C`) | -15297 | 15297 |
| EVAL_PROBE | 188 | -7375 | 7375 |
| EVAL_ITER  | 222 | -1129 | 1129 |
| EVAL_ITER  | 239 | 2595  | 2595 |
| EVAL_ITER  | 230 | 352   | 352 |
| EVAL_ITER  | 226 | -367  | 367 |
| EVAL_ITER (final) | 228 | -85 | 85 |

`CAL_STAGE_OK val=1` — **`final_dac=228`, `final_measured=-85`, dentro de
`CAL_TOL_COUNTS_GEO_PGA=250`**. La bisección se comporta exactamente como se
describe en §13.3 (probe hacia +32 desde 0x9C, dirección `increasing`
detectada correctamente, y converge en 5 iteraciones, bien por debajo del
tope `CAL_MAX_ITER=12`).

**Esta es la primera prueba en hardware real de que el algoritmo de búsqueda
binaria + `stable_avg()` + telemetría funciona end-to-end para al menos una
etapa.**

### 14.2 GEO_BP (stage 1): primer punto saturado, pero el log se corta ahí

`CAL_STAGE_BEGIN(1)` aparece, y luego el primer `(DAC, MEAS)` —
`EVAL_INIT`, `dac=156=CAL_DAC_INIT` — aparece **~5.0 s después**. Eso coincide
exactamente con `CAL_SETTLE_SAMPLES_GEO_BP=15000` muestras a ~3 kSPS
(15000/3000 = 5.0 s). Esto confirma que es una **medición real post-settle**,
no el sentinel de timeout de `async_measure_service()` (que dispara tras
`CAL_ASYNC_EMPTY_POLL_LIMIT=2,000,000` iteraciones de polling vacío — eso, si
ocurriera, sería casi instantáneo en escala de ms, no ~5 s).

El valor de `measured` en la telemetría es `32767` (`0x7FFF`).

**Importante — esto NO implica necesariamente que el algoritmo se haya
colgado ni que sea el sentinel de timeout de §13.5**: `cal_diag_i16()`
(línea 35-42 de `calibration.c`) satura el promedio `int32` a rango `int16`
**solo para el envío por UART**. El valor que usa el algoritmo internamente
(`g_cal_async.measured`, comparado contra `target_counts=0` para
`best_abs_error` y para decidir `increasing`) es el `int32` SIN saturar — y el
ADC `CFG1` es diferencial de 18 bits (~±131071 counts), así que el valor real
podría ser mucho mayor que 32767 en magnitud. En cualquier caso
`abs_error >> tolerance_counts=250`, así que `EVAL_INIT` no converge en este
punto y la máquina avanza normalmente: como `GEO_BP` tiene `direction=1`,
`dac` pasa a `156+32=188` y arranca `EVAL_PROBE`. **El algoritmo sigue
corriendo** — el `32767` es simplemente "la salida de esta etapa está MUY
lejos de 0V diferencial en el punto de partida `0x9C`", lo cual es razonable
si `GEO_BP` (filtro pasabanda) tiene mucha más ganancia/desbalance de DC que
`GEO_PGA`.

### 14.3 El log está truncado, no necesariamente colgado

El log pegado por el usuario muestra líneas `[time][S1]...` y métricas
`#M,time,S1,...` intercaladas/desordenadas (dos streams async renderizados
fuera de orden en la terminal), y se corta a mitad de la etapa `GEO_BP`. **No
tenemos**:
- el resultado de la bisección completa de `GEO_BP` (`final_dac`,
  `final_measured`, `CAL_STAGE_OK`),
- los resultados de `GEO_ADDER` y `GEO_LP`,
- la pasada de `VERIFY` (4x `CAL_VERIFY_BEGIN`/`CAL_VERIFY_OK`),
- el `CAL_DONE val=?` final.

Por lo tanto **no se puede concluir todavía** que la calibración completa
falle ni que `GEO_BP` esté saturado de forma permanente — solo que su primer
punto de muestreo (en `dac=0x9C`) está lejos de 0.

### 14.4 Sobre "calibracion fallida" / "cfg busy sub=0xB5 pending=0xB5" en el log del master

Dos causas posibles, distinguibles por el tiempo transcurrido entre
"`S1 calibracion solicitada`" y la respuesta:

1. **Rechazo inmediato por busy (UX, no bug de firmware)**: si el usuario
   hace clic en "Calibrar" mientras una calibración anterior todavía está
   corriendo (`g_state==PSOC_CALIBRATING` en el PSoC, o
   `g_cfg_waiting==true` en el slave ESP — `handleSetConfig()`,
   `slave/src/main.cpp`), el slave responde **inmediatamente**
   `sendCfgAck(0xB5, 0)` con log `"cfg busy sub=0xB5 pending=0xB5"`, y el
   master loguea `"S1 calibracion fallida"` (`app.js` línea ~791) en
   **menos de 1 s** desde el clic. Dado que una corrida completa de las 4
   etapas + verify puede tardar del orden de **1-3 minutos** (ver §14.5), es
   muy probable que el patrón observado sea esto: el usuario reintentó
   "Calibrar" mientras la corrida anterior seguía viva.
2. **`CAL_DONE val=0` genuino**: si alguna etapa termina en
   `async_finish_stage(0)` (bisección agota `CAL_MAX_ITER`, `dac` llega a un
   riel 0/255, o `lo>hi`) o alguna pasada de `VERIFY` da `verify_ok=0`,
   `g_cal_async.ok` queda en 0, `CAL_DONE val=0`, y el PSoC responde
   `CFG_ACK(0xB5, 0)` → `"S1 calibracion fallida"`. Esto solo puede ocurrir
   **al final** de la secuencia completa (1-3 minutos después del clic).

Para distinguir ambos casos en la próxima prueba: medir el tiempo entre el
clic y la línea `"calibracion ..."` en el log del master.

### 14.5 Próximo paso de hardware recomendado

1. Hacer clic en "Calibrar" **una sola vez** y NO reintentar aunque tarde —
   estimar el peor caso: `GEO_PGA` (~16 s), `GEO_BP` (~80 s), `GEO_ADDER`
   (~41 s), `GEO_LP` (~31 s) + verify de las 4 etapas ≈ **hasta ~3 minutos**
   en el peor caso (si cada etapa necesita las `CAL_MAX_ITER=12`
   iteraciones; en la práctica `GEO_PGA` convergió en 5, así que probablemente
   sea más rápido).
2. Capturar el log COM12 completo desde `CAL_START` hasta `CAL_DONE` sin
   cortes.
3. Reconstruir, por cada etapa 0-3: la trayectoria `(dac, measured)` completa
   (como en la tabla de §14.1), `final_dac`/`final_measured`/`CAL_STAGE_OK`,
   y luego los 4 `CAL_VERIFY_BEGIN`/`CAL_VERIFY_OK`.
4. Si `CAL_DONE val=1`: éxito — repetir 1-2 veces más para chequear
   repetibilidad de `final_dac` (criterio de §13.8 punto 5).
5. Si `CAL_DONE val=0`: identificar CUÁL etapa/verify dio `ok=0` y con qué
   `final_dac`/`final_measured` — eso indica si el problema es de rango de la
   etapa (`dac` en riel 0/255), de tolerancia (`CAL_TOL_COUNTS`/`
   CAL_SETTLE_TOL_COUNTS` muy estrictos para el ruido real), o de `VERIFY`
   (la salida se mueve entre el fin de la bisección y la pasada de verify con
   `verify_settle_samples` más largo).

### 14.6 Confirmado: AMux_IN vuelve a 0 y los VDAC solo se tocan en boot/`0xB5` (osciloscopio + código)

El usuario observó en el osciloscopio que las salidas VDAC se mueven una vez
al arrancar y luego quedan fijas. Esto coincide con el código:

- **AMux_IN**: `psoc_calibration_start_async()` (`calibration.c:257`) pone
  `AMux_IN_Select(CAL_INPUT_REF_CHANNEL=1)` (tierra virtual) durante TODA la
  calibración (4 etapas + verify). Solo cuando la máquina llega a
  `CAL_ASYNC_VERIFY_BEGIN` con `stage_index >= PSOC_CAL_STAGE_COUNT`
  (`calibration.c:413-424`, es decir: las 4 etapas Y las 4 pasadas de verify
  ya terminaron) se llama `psoc_calibration_restore_capture_path()`
  (`calibration.c:130-137`), que hace
  `AMux_IN_Select(CAL_INPUT_NORMAL_CHANNEL=0)` +
  `AMux_ADC_Select(CAL_ADC_CAPTURE_CHANNEL=3)`. No hay rama de aborto que se
  salte este restore — incluso una etapa que no converge
  (`async_finish_stage(0)`) sigue normalmente a la próxima etapa, así que el
  restore siempre corre al llegar a `CAL_DONE`.
- **VDACs**: solo se escriben en dos momentos:
  1. Una vez al boot, sin condición, vía `psoc_calibration_start_references()`
     (`main.c:814`) → escribe `CAL_DAC_INIT=0x9C` en los 4 VDAC.
  2. Durante el autocal de arranque (`main.c:830-856`, disparado
     `PSOC_STARTUP_CAL_DELAY_MS` después de detectar al ESP) y en cualquier
     `0xB5`/`PSOC_CMD_CALIBRATE` posterior (`main.c:573-576`) — ambos pasan por
     `psoc_start_calibration_if_idle()` → `psoc_calibration_start_async()`, la
     misma máquina de búsqueda binaria que escribe cada VDAC repetidas veces
     mientras converge y lo deja en `final_dac`.
  - El comando legacy `0xAA` (`main.c:537-541`) solo actualiza la variable
    sombra `g_vdac_val`, NUNCA llama `VDAC_ref_*_SetValue()`. No hay ningún
    otro `stage->write()` en el firmware.

Conclusión: el comportamiento visto en el osciloscopio es el esperado y
coincide 1:1 con el diseño — VDAC fijos tras boot+autocal, AMux_IN/AMux_ADC
garantizados de vuelta a modo captura normal al llegar `CAL_DONE`.

### 14.7 Nueva telemetría `PSOC_EVT_CAL_AMUX_IN` (0x19) para verificar §14.6 por software

El usuario pidió una forma de confirmar por telemetría (no solo por
osciloscopio/lectura de código) que `AMux_IN` efectivamente vuelve a 0. Se
agregó un evento de diagnóstico nuevo:

- `psoc_hw.h` / `slave/src/psoc_uart.h`: `PSOC_EVT_CAL_AMUX_IN = 0x19`.
- `calibration.c`: nuevo helper `cal_amux_in_select(channel)` que hace
  `AMux_IN_Select(channel)` + `cal_diag(PSOC_EVT_CAL_AMUX_IN, channel)`.
  Reemplaza los 4 usos directos de `CAL_AMUX_IN_SELECT(...)`:
  - `psoc_calibration_start_references()` (boot, → `channel=0`)
  - `psoc_calibration_restore_capture_path()` (fin de calibración / cualquier
    `psoc_prepare_capture_path()`, → `channel=0`)
  - `psoc_calibration_reset_references()` (no usada actualmente, → `channel=1`)
  - `psoc_calibration_start_async()` (inicio de calibración, → `channel=1`)
- `slave/src/main.cpp`: `psocDiagName()` agrega `"CAL_AMUX_IN"`, y `onPsocDiag()`
  imprime una línea dedicada:
  `[SLAVE] CAL AMux_IN -> %u (normal/captura | referencia/tierra virtual)`
  además de la línea genérica `PSOC_EVT CAL_AMUX_IN event=0x19 val=%u ...`.

**Qué esperar en el log COM12 de una corrida `0xB5` completa**: una línea
`CAL_AMUX_IN -> 1 (referencia/tierra virtual)` justo después de `CAL_START`
(emitida dentro de `psoc_calibration_start_async()`), y una línea
`CAL_AMUX_IN -> 0 (normal/captura)` justo antes de `CAL_DONE` (emitida dentro
de `psoc_calibration_restore_capture_path()` al llegar a
`CAL_ASYNC_VERIFY_BEGIN` con las 4 etapas+verify completas). Si la segunda
línea NO aparece, o aparece con `val=1`, eso indicaría que la máquina no llegó
a `CAL_ASYNC_DONE` (se quedó colgada en alguna etapa) — sería la primera señal
de un problema real, distinto de lo analizado en §14.2-14.3.

**Pendiente**: compilar/programar este cambio y volver a correr "Calibrar" una
vez completa (§14.5) para confirmar en hardware que ambas líneas aparecen como
se espera.

### 14.8 Reporte del usuario (2026-06-14): 2da calibración "se cuelga", riesgo de quedar saturado

El usuario reportó, sin haber compilado/programado todavía los cambios de
§14.7 (ese paso sigue pendiente):

- La **primera** vez que aprieta "Calibrar" tras un reset del PSoC, calibra
  bien (esto coincide con §14.1, GEO_PGA convergió en la primera corrida real).
- Las veces **siguientes** parece quedarse en un "loop infinito": nunca
  vuelve, o al menos no hay ninguna indicación de que haya terminado.
- Le preocupa que el **estado final** de los operacionales pueda quedar
  **saturado** (en un extremo del DAC) — esto es peor que quedar "no
  calibrado", y pide que ante esa situación el firmware "vuelva atrás" a un
  candidato no saturado, guardando los mejores candidatos en vez de tirar el
  DAC a un extremo.
- Pide telemetría "elemental" de estado durante la calibración para que el
  ESP/maestro sepan que el PSoC sigue viva (no colgado).

**Hipótesis de la causa del "loop infinito"** (sin confirmar en hardware
todavía): `async_measure_service()` tiene un timeout por-medición de
`CAL_ASYNC_EMPTY_POLL_LIMIT = 2,000,000` polls cuando `psoc_adc_take_isr_sample()`
nunca devuelve una muestra fresca. Si en la 2da corrida la ISR del ADC
(`isr_DelSigReady`) deja de disparar por algún motivo de configuración, **cada
una de las ~60 mediciones** de la corrida (4 etapas × hasta 14 mediciones de
búsqueda + 4 de verify) tardaría el timeout completo antes de devolver el
sentinel `0x7FFF` — multiplicando un timeout de unos pocos segundos por 60 da
un total de varios minutos, que para el usuario "parece infinito" (y puede
superar incluso el `PSOC_CAL_ACK_TIMEOUT_MS=240000` del ESP, dejando
`g_cal_async.busy=1` para siempre → 3ra corrida rechazada con `CAL_BUSY`).
**No se modificó `async_measure_service`/ISR en esta sesión** — sería el
próximo paso si el watchdog de abajo no resuelve el síntoma.

**Cambios implementados en esta sesión (sin compilar/programar — pendiente
igual que §14.7)**:

1. **Watchdog global de calibración** (`calibration.c`):
   - Nuevas constantes `CAL_WATCHDOG_TICKS = 20000` (200 s a 10 ms/tick, por
     debajo del `PSOC_CAL_ACK_TIMEOUT_MS=240000` del ESP) y
     `CAL_PROGRESS_PERIOD_TICKS = 50` (~500 ms).
   - Nuevo getter `psoc_now_ticks()` (declarado en `psoc_hw.h`, implementado
     en `main.c` como wrapper de `timer_now_ticks()`/`g_timer_ticks`,
     10 ms/tick) para que `calibration.c` pueda medir tiempo real transcurrido.
   - `psoc_calibration_start_async()` guarda `start_ticks`/`last_progress_ticks`.
   - Al inicio de `psoc_calibration_service_async()`: si
     `now - start_ticks >= CAL_WATCHDOG_TICKS`, se llama
     `cal_async_abort_watchdog()` y se retorna `1u` (= "terminado", dispara
     `CAL_DONE ok=0` en `main.c` igual que un final normal).
   - `cal_async_abort_watchdog()`: si NO está en fase de verificación
     (`CAL_ASYNC_VERIFY_BEGIN`/`CAL_ASYNC_EVAL_VERIFY`), fuerza
     `CAL_DAC_INIT` (0x9C, punto medio, no saturado) en la etapa en curso y
     en las que faltan (las ya terminadas conservan su `best_dac`); si SÍ
     está en verificación, no toca ningún DAC (todas las etapas ya tienen su
     `best_dac` escrito). En ambos casos: `ok=0`, restaura el camino de
     captura (`psoc_calibration_restore_capture_path()` → `AMux_IN=0` +
     canal ADC de captura), re-habilita `isr_SyncIn`, `busy=0`, `done=1`,
     `state=CAL_ASYNC_DONE`, y emite el nuevo evento `PSOC_EVT_CAL_WATCHDOG`
     (0x1B) con `value=stage_index` donde se colgó.
   - **Garantiza** que toda corrida de calibración termina (`CAL_DONE`) en
     ≤200 s, sin que ninguna etapa quede con un DAC fuera de control.

2. **Nunca dejar un operacional fuera de rango operativo** (`calibration.c`,
   `async_finish_stage()`) — **CORREGIDO en §14.9**, ver ahí el motivo:
   - Nueva constante `CAL_OPERATING_RANGE_COUNTS = 26214` (`calibration_tables.h`,
     ±0.5 V derivados de `ADC_CFG1_COUNTS_PER_VOLT = 52429`) y helper
     `cal_measured_out_of_range(measured)` (`abs(measured) > 26214`).
   - Si la **medición** (`best_measured`, en counts del ADC) del `best_dac`
     encontrado por la búsqueda binaria queda fuera de ese rango, se descarta
     (aunque `ok` hubiera sido 1) y se escribe `CAL_DAC_INIT` en su lugar, con
     `final_measured = base_measured` (la medición tomada en `CAL_DAC_INIT` al
     inicio de la etapa) y `ok=0`. Implementa el pedido del usuario: "más
     conviene un sistema no calibrado perfectamente que uno que queda en
     cualquier extremo".
   - Caso especial **GEO_LP** (última etapa, `stage_index ==
     PSOC_CAL_STAGE_COUNT-1`, la que alimenta `CAL_ADC_CAPTURE_CHANNEL`): si
     incluso con `CAL_DAC_INIT` la medición sigue fuera de rango, se emite
     `PSOC_EVT_CAL_LP_BAD` (0x1C) — ver §14.9.

3. **Telemetría periódica de progreso** (`calibration.c` + `slave/src/main.cpp`):
   - Nuevo evento `PSOC_EVT_CAL_PROGRESS` (0x1A), emitido cada
     `CAL_PROGRESS_PERIOD_TICKS` (~500 ms) con `value = stage_index` actual
     (0-3, ver `psocCalStageName`).
   - En el slave, `onPsocDiag()` loguea cada `CAL_PROGRESS` (`[SLAVE] CAL
     progress stage=N/NOMBRE`), y si hay un `CMD_CALIBRATE` pendiente
     (`g_cfg_waiting && g_cfg_sub_cmd==PSOC_CMD_CALIBRATE`), reenvía al
     maestro `sendCfgAck(PSOC_CMD_CALIBRATE, 2)` (throttle de 3 s,
     `PSOC_CAL_PROGRESS_ACK_PERIOD_MS`). `ok=2` es un sentinel nuevo
     ("sigue calibrando", distinto de 0=fallo/1=ok) — el master firmware NO
     necesita cambios: `onCfgAck()` no tiene caso especial para
     `sub_cmd=0xB5`, así que cae directo a `matlab.sendAck()` y llega a la
     web como `PTYPE_ACK ackCmd=0xB5 ackVal=2`.
   - `PSOC_EVT_CAL_WATCHDOG` (0x1B) también se loguea en el slave
     (`[SLAVE] CAL WATCHDOG timeout en stage=N/NOMBRE -> abortado, valores
     seguros restaurados`).

4. **Indicador "calibrando..." en la web del maestro**:
   - `slave_panel.js`: `setDot()`/`setCalibrationLock()` ahora soportan un
     3er estado (`state=3`) → clase CSS `.dot.busy` (punto naranja
     pulsante, `style.css`), tooltip "Calibrando... (puede tardar hasta
     ~3 min)".
   - `app.js`: `onCalibrateRequested()` pone el dot en `state=3` al pedir la
     calibración. `handleAck()` intercepta `ackCmd===SUBCMD_CALIBRATE &&
     ackVal===2` ANTES del borrado genérico de `nd.pending` (para no perder
     el pending de la confirmación final), mantiene el dot en `state=3` y
     loguea `"S{idx} calibrando..."` con throttle de 15 s
     (`nd.calProgressLogMs`, nuevo campo en `data_store.js`) para no
     inundar el log durante los ~3 min que puede durar.

**Recordatorio (ya implementado, no es código nuevo)**: el "debounce" de
transitorio que pedía el usuario ("muestreás y cuando deja de variar decís que
ya pasó el transitorio") YA EXISTE — es `async_measure_service()` comparando
ventanas de `CAL_AVG_N=32` muestras consecutivas contra
`CAL_SETTLE_TOL_COUNTS=100` counts, hasta `CAL_SETTLE_MAX_WINDOWS=10` veces
(`calibration_tables.h`). Si en hardware el transitorio real es más lento/
ruidoso que eso, ajustar esas dos constantes (no hace falta rediseñar la
lógica).

### 14.9 Corrección del usuario (2026-06-14): el criterio es voltaje de salida, no código de DAC

El punto 2 de §14.8 ("nunca dejar una etapa en un rail del DAC",
`CAL_RAIL_MARGIN=4`/`cal_dac_is_rail(dac)`) fue **reemplazado** tras esta
aclaración del usuario:

> "No me refiero a que no podes dejar el VDAC en algún extremo, lo que digo es
> que la prioridad máxima es que todos los operacionales estén en rango
> operativo (máximo ±0.5 voltios fuera del 0, idealmente mucho menos) y lo más
> importante el VDAC LP que es la etapa que va al ADC debe estar SIEMPRE bien
> si no no vemos un carajo."

Es decir: lo que importa no es si el *código* del DAC (0-255) queda cerca de
0/255, sino si la **medición del ADC** (la salida real del op-amp de esa
etapa) queda dentro del rango operativo (±0.5 V = ±26214 counts, ver
`ADC_CFG1_COUNTS_PER_VOLT=52429` en §4/§13). Un `best_dac` cerca de un
extremo del rango 0-255 puede perfectamente corresponder a una salida dentro
de ±0.5 V (y viceversa), así que chequear el código del DAC no es lo correcto.

**Cambios aplicados** (reemplazan el punto 2 de §14.8, sin tocar los puntos
1/3/4):

- `calibration_tables.h`: nueva constante `CAL_OPERATING_RANGE_COUNTS = 26214L`
  (±0.5 V).
- `calibration.c`: se eliminó `CAL_RAIL_MARGIN`/`cal_dac_is_rail(dac)`, se
  agregó `cal_measured_out_of_range(measured)` (`abs_counts(measured) >
  CAL_OPERATING_RANGE_COUNTS`).
- `async_finish_stage()`: el chequeo ahora es sobre `final_measured`
  (counts del ADC), no sobre `final_dac` (código 0-255). Igual que antes, si
  está fuera de rango se vuelve a `CAL_DAC_INIT`/`base_measured`/`ok=0`.
- **Nuevo, específico para GEO_LP** (última etapa, `stage_index ==
  PSOC_CAL_STAGE_COUNT-1`, la que alimenta `CAL_ADC_CAPTURE_CHANNEL=3`): si
  incluso en `CAL_DAC_INIT` (el fallback "seguro") la medición sigue fuera de
  ±0.5 V, se emite el nuevo evento `PSOC_EVT_CAL_LP_BAD` (0x1C) — indica que la
  captura de datos va a ser inútil ("no vemos un carajo") porque la última
  etapa antes del ADC está fuera de rango incluso sin calibrar. No hay
  fallback adicional posible en firmware para este caso (es un problema de
  hardware/offset que excede lo que el VDAC de referencia puede compensar);
  el evento es solo diagnóstico.
- `psoc_hw.h` / `slave/src/psoc_uart.h`: `PSOC_EVT_CAL_LP_BAD = 0x1C`.
- `slave/src/main.cpp`: `psocDiagName()` devuelve `"CAL_LP_BAD"`; `onPsocDiag()`
  loguea como crítico: `[SLAVE] *** CAL CRITICO *** stage=N/GEO_LP (etapa de
  captura) quedo fuera de rango incluso en CAL_DAC_INIT -> la captura va a ser
  inutil` (vía `SLAVE_LOG_PRINTF` + `LOGM("CAL_LP_BAD", ...)`).
- **No se agregó plumbing nuevo hacia la web del maestro** para
  `CAL_LP_BAD` específicamente: si GEO_LP queda fuera de rango, `ok=0` ya
  provoca que `CAL_DONE` llegue con `ok=0` y la web marque el dot en
  `state=2` ("fallida"), que es la señal más importante. `CAL_LP_BAD` queda
  como detalle de diagnóstico en el log serie del esclavo
  (`DBG_STREAM`/`LOGM`, no llega a la web) — suficiente por ahora; si se
  necesita visibilidad en la web habría que sumar un campo/evento nuevo al
  protocolo ESP-NOW `MsgCfgAck`.

**Pendiente**: compilar/programar TODO lo de §14.7 + §14.8 + §14.9 junto, y
correr "Calibrar" 2-3 veces seguidas observando: (a) que cada corrida termine
(`CAL_DONE`) en ≤200 s con indicación visible en la web (`state=3`
"calibrando..."), (b) si aparece `CAL_WATCHDOG`, en qué `stage_index` queda,
(c) que ninguna etapa quede con `abs(final_measured) > 26214` counts
(±0.5 V) — revisar `CAL_STAGE_MEAS` de cada corrida, especialmente la última
(GEO_LP), y (d) que no aparezca `CAL_LP_BAD`; si aparece, GEO_LP necesita
revisión de hardware (offset fuera de lo que el VDAC de referencia puede
compensar).

### 14.10 Sesión 2026-06-14 (3ra parte): primer ensayo en hardware de §14.7/14.8/14.9, AMux_IN idle/activo, y promediado acumulativo contra overfitting

Tras compilar/programar §14.7+14.8+14.9, el usuario reportó la primera
corrida real:

> "buildee todo pero no parece haber funcionado bien podes probar vos? el
> psoc programé pero no parece autocalibrarse al arrancar igual esta medio mal
> porque tarda mucho en estabilizarse deberias aplicar la misma logica cuando
> ves que las señales estan calibradas ahi recien activas la rutina inicial de
> autocalibración. De default podes dejar tambien la entrada en AMux_In 1 para
> que siempre se verifique para autocalibrar si estamos idle pero igual la
> autocalibración aun no funciona sigue siendo peor que el valor de arranque
> inicial"

Del log pegado: ~80 líneas `HELLO` repetidas y luego "S1 calibracion
fallida" — **nunca apareció ninguna indicación "calibrando..."** (`state=3`)
en el log, es decir el indicador de progreso de §14.8 punto 2 no se vio
disparar (no se investigó la causa raíz en esta sesión: podría ser que la
corrida nunca llegó a `psoc_calibration_start_async()`, o que terminó/abortó
antes del primer `PSOC_EVT_CAL_PROGRESS`). Además reporta que el resultado de
calibración es **peor** que el valor de arranque (`CAL_DAC_INIT`).

#### a) AMux_IN: idle = referencia, activo = entrada real (implementado)

El usuario afinó el pedido con un mensaje posterior, más específico que la
sugerencia "AMux_IN=1 por default" de arriba:

> "Deberias mantener en IDLE el AMux_in en 1 y cuando vamos a medir algo ahi
> moves a 0 luego volves a 1, particularmente cuando llega el pre-start y
> estas en hot-wait ahi moves el mux"

Implementado en `calibration.c`/`calibration.h`/`main.c`:

- `calibration.c`: nuevas funciones exportadas `psoc_calibration_amux_active()`
  (AMux_IN → `CAL_INPUT_NORMAL_CHANNEL`=0, entrada real) y
  `psoc_calibration_amux_idle()` (AMux_IN → `CAL_INPUT_REF_CHANNEL`=1,
  referencia/tierra virtual), ambas wrappers de `cal_amux_in_select()` (que ya
  emite `PSOC_EVT_CAL_AMUX_IN`).
- `psoc_calibration_restore_capture_path()` (usada al terminar boot/captura/
  calibración) ahora deja `AMux_IN=REF(1)` en vez de `NORMAL(0)` — éste es el
  nuevo estado IDLE por defecto: `AMux_ADC` sigue mirando GEO_LP
  (`CAL_ADC_CAPTURE_CHANNEL=3`) pero contra la referencia, permitiendo
  monitorear en cualquier momento si el front-end sigue calibrado sin afectar
  la entrada real.
- `main.c`: nuevos helpers estáticos `psoc_set_amux_active()` /
  `psoc_set_amux_idle()` (wrappers 1:1 de las funciones de arriba, evitan que
  `main.c` necesite las macros `CAL_INPUT_*` de `calibration_tables.h`).
  Cableado en todas las transiciones IDLE↔activo:
  - `psoc_arm()` (PRESTART/`0xB1`, entra a "hot-wait" `PSOC_ARMED` esperando
    SYNC): `psoc_set_amux_active()` justo después de
    `psoc_prepare_capture_path()` — el mux se mueve a la entrada real ya
    durante el hot-wait, como pidió el usuario.
  - `psoc_start_now()` (`0xB4`): mismo cableado.
  - Branch debug-on (`0xB3` con `g_debug_psoc`): mismo cableado entre
    `psoc_prepare_capture_path()` y `psoc_enter_sampling(1u)`.
  - Transición SAMPLING→IDLE en `service_runtime()`: `psoc_set_amux_idle()`
    justo después de `g_state = PSOC_IDLE`.
  - `isr_SyncIn` flanco de bajada en `PSOC_ARMED` (abort, SYNC nunca llegó):
    `psoc_set_amux_idle()` tras volver a `PSOC_IDLE` en la sección crítica.
  - Boot: como `psoc_calibration_restore_capture_path()` ya deja `AMux_IN=REF`,
    el arranque normal termina en `AMux_IN=1` sin código adicional.

Resultado esperado tras compilar/programar: en cualquier corrida (`0xB1`,
`0xB4`, debug `0xB3`) debería verse `CAL_AMUX_IN -> 0` al entrar a hot-wait/
sampling y `CAL_AMUX_IN -> 1` al volver a idle. **Pendiente verificar en
hardware** junto con lo de §14.7.

#### b) Promediado acumulativo contra "overfitting" al ruido (implementado)

Pedido más importante de esta sesión, sobre por qué la calibración termina
peor que `CAL_DAC_INIT`:

> "Mi mayor problema es que siento que estas moviendo el DAC de más porque
> tratas de fitear demasiado, lo ideal seria que si hay demasiado ruido de
> medición sigas promediando, ejemplo en vez de promediar a lo loco un valor
> fijo, capaz lo que podrias hacer es seguir promediando hasta que el valor
> medido deje de variar una cierta resolución por ejemplo +-10 bits, ahi dejas
> de promediar y usas ese valor como el real, tambien no tengas miedo de
> volver a un valor anterior de la busqueda si daba un offset mejor porque en
> el osciloscopio veo que llegas un valor quasi-perfecto pero ya que seguis
> buscando como loco empezas a hacer un overfitting que se traduce como
> saturación en LP solo porque no esta en 0 perfecto"

Diagnóstico: la parte "no tengas miedo de volver a un valor anterior" ya
estaba implementada estructuralmente — `async_finish_stage()` siempre escribe
`best_dac`/`best_measured` (el mejor punto visto durante toda la búsqueda, no
el último). El problema real era el ruido en `measured`: cada punto se medía
con un solo promedio de `CAL_AVG_N=32` muestras, repetido hasta
`CAL_SETTLE_MAX_WINDOWS=10` veces, comparando ventanas consecutivas con
`CAL_SETTLE_TOL_COUNTS=100` — una ventana de 32 muestras ruidosa podía quedar
"casualmente" dentro de la tolerancia y marcar un punto saturado como
`best_measured` (mejor `abs_error` que el real) sin que la búsqueda lo supiera.

**Cambio aplicado** (`calibration.c`, `async_measure_service()` +
`PsocCalAsync`/`async_measure_begin()`):

- Nuevos campos `cum_sum`/`cum_count` (int32) en `PsocCalAsync`, reseteados en
  `async_measure_begin()`.
- Cada ventana completa de `avg_n` muestras ya NO se compara aislada: se suma
  a `cum_sum`/`cum_count` (promedio acumulado de TODAS las muestras tomadas
  para ese punto desde el último `write(dac)`).
- `g_cal_async.measured` pasa a ser siempre `cum_avg = cum_sum / cum_count` —
  tanto si se llega por "asentó" como por "se acabaron las ventanas". Antes,
  el camino de "se acabaron las ventanas" usaba la última ventana sola (la más
  ruidosa posible).
- La condición de asentado compara `cum_avg` consecutivo contra
  `CAL_SETTLE_TOL_COUNTS` (antes comparaba ventanas aisladas). Como `cum_avg`
  es un promedio creciente, cada ventana nueva lo mueve cada vez menos —
  converge naturalmente incluso con una tolerancia mucho más chica.
- `calibration_tables.h`: `CAL_SETTLE_TOL_COUNTS` 100 → **10** (±10 counts,
  el valor que pidió el usuario) y `CAL_SETTLE_MAX_WINDOWS` 10 → **40** (limite
  de seguridad si nunca asienta a ±10). Costo en el peor caso: 40×32=1280
  muestras/punto (~426 ms a ~3 kSPS) vs 320 muestras (~107 ms) antes; con
  ~60 puntos medidos en una corrida completa eso son ~19 s extra en el peor
  caso, muy por debajo de `CAL_WATCHDOG_TICKS`=200 s.

**Pendiente**: compilar/programar y repetir la calibración varias veces,
comparando con el osciloscopio que GEO_LP ya no termine saturado y que el
resultado sea igual o mejor que `CAL_DAC_INIT` (no peor). Si con
`CAL_SETTLE_TOL_COUNTS=10` algún punto sigue sin asentar nunca (siempre usa
las 40 ventanas), considerar relajar a ±20-30 antes de tocar `CAL_AVG_N`.

#### c) Abierto / no implementado en esta sesión

- **Disparo de auto-calibración al boot**: el trigger actual
  (`startup_cal_pending`/`startup_cal_due`,
  `PSOC_STARTUP_CAL_DELAY_MS=5000`, en `main.c`) es un *one-shot* a los 5 s de
  conectar el ESP: si en ese instante `g_state != PSOC_IDLE` (p.ej. ya llegó
  `0xB1`/`PSOC_ARMED`), `psoc_start_calibration_if_idle(0u)` no hace nada
  (emite `PSOC_EVT_CAL_BUSY`) y `startup_cal_pending` se limpia igual — la
  calibración de arranque nunca corre y no hay reintento. El usuario pidió
  reemplazar el delay fijo por un disparo basado en estabilidad ("cuando ves
  que las señales estan calibradas ahi recien activas la rutina inicial de
  autocalibración") — **no diseñado ni implementado todavía**.
- **"Invalid: fs must be positive"**: aparece en un panel
  Test/Ver/Probe-latency de la UI del maestro (captura de pantalla adjunta por
  el usuario); no se investigó alcance/causa en esta sesión.
- **Indicador "calibrando..." nunca visto en el log**: no se confirmó si
  `PSOC_EVT_CAL_PROGRESS`/`state=3` llegó a dispararse en la corrida reportada
  — revisar junto con el punto de "auto-calibración al boot" de arriba.

### 14.11 Sesión 2026-06-14 (4ta parte): "calibrando..." independizado del stream de diagnóstico + diagnóstico de fondo de "siempre termina en una esquina"

El usuario activó `/advisor opus` y `/effort max`, y mandó (verbatim):

> "Hace el update de master y slave vos porque cuando hago no aparecen los
> updades en master como el ver el estado actual de la calibración no veo en
> ningun lugar de la interfaz? porque mucha vueltas ya dimos algo esta mal en
> como medis y decidis mover los VDACs porque al ver en el osciloscopio estas
> moviendo bien las cosas pero no paras cunado debes parar y siempre concluis
> que la mejor señal es en una de las esquinas claro porque si esta enla
> esquina va a saturar y ahi para de tener ruido pero eso es estupido"

Dos pedidos:

- **(c1)** "Hacé el build vos" + por qué nunca se ve "calibrando..." en la interfaz.
- **(c2) — el de fondo**: la búsqueda mueve los VDAC en la dirección correcta
  (se ve en el osciloscopio) pero NUNCA PARA cuando debería, y siempre termina
  en una esquina del rango DAC (0 o 255) — porque una esquina satura la etapa,
  y "saturado" = "sin ruido", y el criterio de "convergió" (incluido el de
  §14.10b) sólo mira "¿dejó de moverse / tiene poco ruido?", que en una
  esquina saturada es SIEMPRE verdadero. No es un tema de afinar constantes:
  el criterio está mal de raíz.

#### a) c1 — Auditoría completa de la cadena "calibrando..." → web (sin bugs encontrados; se hizo más robusta de todos modos)

Se repasó TODO el camino PSoC → ESP esclavo → ESP maestro → web de punta a
punta, sin cortes:

1. `platformio.ini` (slave): el diff pendiente renombra `-DDBG_ENABLE=1` →
   `-DSLAVE_LOGS_ENABLE=1`, y `debug_log.h` lo reencadena a
   `DBG_ENABLE=SLAVE_LOGS_ENABLE`. **Valor efectivo de `DBG_ENABLE`=1 antes Y
   después** — rename cosmético, descartado como causa.
2. `calibration.c::psoc_calibration_service_async()`: `cal_diag(PSOC_EVT_CAL_PROGRESS,
   stage_index)` se emite cada `CAL_PROGRESS_PERIOD_TICKS=50` ticks (~500 ms)
   mientras `g_cal_async.busy` — dispara repetidamente durante TODA la
   calibración, no sólo al final.
3. `psoc_calibration_start_async()`: `start_ticks`/`last_progress_ticks` se
   inicializan con `psoc_now_ticks()` al arrancar — el watchdog
   (`CAL_WATCHDOG_TICKS=20000`≈200 s) no puede disparar instantáneo por un bug
   de init (descartado).
4. `slave/main.cpp::loop()`: mientras `g_state != SAMPLING` (CALIBRATING
   incluido) se llama `psoc.poll()` en cada iteración, que dispara
   `onPsocDiag()` vía `_diagCb`.
5. `onPsocDiag()`: el guard `if (g_state == SAMPLING && event.event !=
   PSOC_EVT_DUMP_DONE) return;` no bloquea nada durante CALIBRATING (state=3
   ≠ SAMPLING=2).
6. Rama `PSOC_EVT_CAL_PROGRESS` (antes del cambio de esta sesión): chequeaba
   `g_cfg_waiting && g_cfg_sub_cmd==PSOC_CMD_CALIBRATE` y mandaba
   `sendCfgAck(PSOC_CMD_CALIBRATE, 2)` cada `PSOC_CAL_PROGRESS_ACK_PERIOD_MS=3000` ms.
7. `g_cfg_waiting`: se pone `true` en `waitForPsocConfigAck()` al aceptar
   `0xB5`, y sólo se limpia con el ack final (`ackCmd==g_cfg_sub_cmd`) o tras
   `PSOC_CAL_ACK_TIMEOUT_MS=240000` ms — permanece `true` durante toda una
   calibración normal (<200 s por el watchdog del PSoC).
8. `master/data/js/app.js::handleAck()` línea 759: `if (ackCmd ===
   cfg.SUBCMD_CALIBRATE && ackVal === 2 ...) { ...; return; }` — corta ANTES
   de la rama "confirmada/fallida" (línea 803). Sin conflicto.
9. `data_store.js::NodeData`: `this.calProgressLogMs = 0` — el primer log
   "calibrando..." no se pierde por `NaN` (estaba bien inicializado).

**Cada eslabón individual está correcto**; no se encontró ningún bug estático
que explique "nunca apareció calibrando...". Podría ser un problema de
timing/orden de llegada de paquetes que sólo se ve en vivo (p. ej. si el
stream de diagnóstico y el ack final `0xC2` llegan "de a golpes" tras un
bloqueo largo del lado PSoC, ambos podrían procesarse en la misma iteración de
`loop()` del ESP y el dot pasaría de `busy`→`ok/bad` sin que el navegador
llegue a pintar el estado intermedio).

**Cambio aplicado** (`slave/src/main.cpp`, 3 ediciones; `calibration.c`/PSoC
sin tocar): el ping "calibrando..." (`sendCfgAck(PSOC_CMD_CALIBRATE, 2)`) ya
NO depende de que llegue `PSOC_EVT_CAL_PROGRESS` por el stream de diagnóstico
del PSoC:

- `waitForPsocConfigAck()`: si `sub_cmd==PSOC_CMD_CALIBRATE`, inicializa
  `g_cal_progress_ack_ms = g_cfg_start_ms - PSOC_CAL_PROGRESS_ACK_PERIOD_MS` →
  el primer ping sale en el `loop()` siguiente, sin esperar 3 s.
- `servicePsocConfigAck()`: al final, si `g_cfg_waiting &&
  g_cfg_sub_cmd==PSOC_CMD_CALIBRATE`, manda `sendCfgAck(PSOC_CMD_CALIBRATE,
  2)` cada `PSOC_CAL_PROGRESS_ACK_PERIOD_MS` — corre en CADA `loop()` mientras
  `g_state != SAMPLING`, usando sólo `millis()` y estado local del ESP (nada
  de UART del PSoC).
- `onPsocDiag()` rama `PSOC_EVT_CAL_PROGRESS`: se quitó el envío de ack
  (queda sólo el log `CAL_PROGRESS` para diagnóstico).

Resultado: el dot `.busy` ("Calibrando... (puede tardar hasta ~3 min)") y el
log "S{idx} calibrando..." (cada 15 s, throttle del lado web) deberían
aparecer dentro del primer segundo de aceptar `0xB5` y repetirse cada 3 s
hasta el ack final — **independiente de cualquier problema en el link de
diagnóstico PSoC→ESP**. Si en la próxima corrida TODAVÍA no aparece
"calibrando...", el bug está en `sendCfgAck`/ESP-NOW/`onCfgAck`/
`matlab.sendAck`/`protocol.js`/`app.js`/`slave_panel.js` — no en el lado
PSoC, lo cual acota mucho la búsqueda.

**Build verificado** (sólo compilación, sin flashear — eso queda para el
usuario):
- `pio run -e slave2` (GEOPHONE) → SUCCESS.
- `pio run -e esp32dev` (maestro) → SUCCESS.
- `pio run -e esp32dev -t buildfs` (imagen LittleFS con `slave_panel.js`, ya
  tenía el `.busy`/`setCalibrationLock(3)` de §14.10) → SUCCESS.

#### b) c2 — el problema de fondo: "converged" no distingue "centrado" de "saturado"

Cita completa de la parte que importa:

> "...algo esta mal en como medis y decidis mover los VDACs porque al ver en
> el osciloscopio estas moviendo bien las cosas pero no paras cunado debes
> parar y siempre concluis que la mejor señal es en una de las esquinas claro
> porque si esta enla esquina va a saturar y ahi para de tener ruido pero eso
> es estupido"

**Evidencia ya disponible (de §14.10, sin nueva corrida) consistente con este
diagnóstico**: el usuario reportó que, AÚN con el fallback de §14.9
(`cal_measured_out_of_range()`: si `|best_measured| >
CAL_OPERATING_RANGE_COUNTS=26214` ±0.5 V, se descarta `best_dac` y se vuelve a
`CAL_DAC_INIT`/`base_measured`/`ok=0`), **el resultado sigue siendo peor que
`CAL_DAC_INIT`**. Para que esto pase, `best_measured` tiene que estar PASANDO
el chequeo de rango (`|best_measured| ≤ 26214`) — el ADC lee un valor
"razonable" — mientras la etapa analógica está en un estado que el
osciloscopio muestra saturado. Una salida saturada (pegada a un riel) es una
señal DC constante: si esa constante cae cerca del "cero diferencial" que mide
el ADC, `measured≈0` Y el ruido entre ventanas ≈0 simultáneamente —
exactamente lo que el criterio de "convergió" (§14.10b y el anterior)
interpreta como "perfecto, paramos acá".

**El defecto de fondo** (enmarcado con el advisor, Opus 4.8): el criterio
actual de convergencia usa SOLO `(measured, ruido)` en UN punto del DAC. Eso
es estructuralmente incapaz de distinguir entre:

- **Cruce por cero real**: la etapa tiene ganancia normal ahí, `measured≈target`
  porque el VDAC de referencia está bien ajustado.
- **Plateau de saturación**: la etapa tiene ganancia ≈0 ahí (la salida del
  op-amp está pegada a un riel), y `measured≈target` es COINCIDENCIA (la
  constante saturada cae cerca de 0 diferencial).

Ambos casos se ven IDÉNTICOS para el criterio actual: "poco ruido, `abs_error`
chico → converged". El discriminador que falta es la PENDIENTE/RESPUESTA de
`measured` ante cambios de `dac`: en un cruce real, `measured` responde
(pendiente ≠ 0); en un plateau saturado, `measured` no responde (pendiente ≈
0) — la ganancia de esa etapa, en ese rango de DAC, es nula.

**Por qué NO se tocó el algoritmo todavía**: hay al menos una hipótesis
alternativa que explica el mismo síntoma — que la GANANCIA de la etapa sea tan
ALTA que **1 LSB del VDAC de referencia (≈16 mV) ya mueva `measured` en mucho
más que `CAL_TOL_COUNTS=250`**, en cuyo caso NINGÚN código 0-255 satisface la
tolerancia, la búsqueda agota `CAL_MAX_ITER=12` iteraciones, y `best_dac` queda
en el código más cercano al cruce real — que, si el offset de hardware es
grande, podría estar genuinamente cerca de 0 o 255 (una "esquina" real, no una
saturación). Esta hipótesis NO se arregla con un chequeo de
pendiente/respuesta — se arregla con una tolerancia relativa a la pendiente
local.

Ambas hipótesis predicen "termina en una esquina", pero la corrección es
distinta (chequeo de respuesta/pendiente vs. tolerancia relativa), y un fix
elegido sin datos puede no aplicar al caso real o enmascarar el síntoma sin
arreglarlo. **No se hizo ningún cambio al algoritmo de búsqueda/convergencia
esta sesión** — queda bloqueado hasta tener la traza de la próxima corrida.

#### c) Qué traer para destrabar (c2): una corrida completa, sin recortar

El log `[SLAVE] CAL point stage=X/NOMBRE dac=D meas=M` (más `CAL_BEGIN`/
`CAL_STAGE`/`CAL_VERIFY*`/`CAL_PROGRESS`/`CAL_AMUX_IN`/`CAL_WATCHDOG`) ya tiene
todo lo necesario — es el mismo que armó las tablas de §14.1/§14.2. Lo que
faltó en §14.2 fue que se cortó a la mitad de GEO_BP. Para la próxima corrida:

1. Programar (`pio run -t upload` para `slave2`, y para el maestro +
   `uploadfs`), presionar "Calibrar" y dejar el monitor serie del ESCLAVO
   corriendo SIN CORTAR hasta ver `S{idx} calibracion confirmada` o `fallida`
   en la web. Con el fix de (a), debería verse "Calibrando..." parpadeando en
   el dot mientras tanto — confirma (c1) en la misma corrida.
2. Pegar el log COMPLETO (todas las líneas `[SLAVE] CAL ...` de las 4 etapas +
   verify), no sólo un fragmento.

Con esa traza `(stage, dac, measured)` completa por etapa, esto se decide en
minutos:

- **Si `measured` se queda CONSTANTE (≈mismo valor, posiblemente cerca de 0)
  para varios `dac` distintos cerca del final de una etapa** → plateau de
  saturación confirmado → fix = chequeo de respuesta/pendiente antes de
  aceptar `best_dac` (hipótesis A).
- **Si `measured` SIGUE variando con `dac` (pendiente clara, sin aplanarse)
  pero nunca entra en `±250` y la búsqueda gasta las 12 iteraciones** →
  tolerancia demasiado chica para la ganancia real de esa etapa → fix =
  tolerancia relativa a la pendiente local, o `CAL_TOL_COUNTS` por etapa más
  realista (hipótesis B).
- También mirar `final_dac` de cada etapa: si literalmente es `0` o `255` (no
  sólo "cerca"), es más evidencia de plateau (las etapas suelen saturar ANTES
  del extremo absoluto del código DAC, pero el extremo absoluto SIEMPRE está
  saturado si el resto lo está).

No se requiere ninguna otra acción del usuario sobre hardware aparte de
programar y correr una calibración una vez.

### 14.12 Sesión 2026-06-14 (5ta parte): la corrida de §14.11c llegó completa — blind spot del clamping de telemetría, GEO_LP cruza de riel a riel en 1 LSB, idea de PID/dithering (NO implementada), y fix de telemetría sin clamping

#### a) La corrida completa pedida en §14.11c

El usuario pegó el log completo, sin cortar, de las 4 etapas + verify. Cita:
"Acá casi hizo lo que debia hacer, pero al final se volvio a ir al carajo,
podes analizar el log para ver que paso?"

Tabla `dac→meas` (el `meas` del log, que es `cal_diag_i16` — ver (b) sobre por
qué este valor puede estar clampeado):

| Etapa | Secuencia `dac→meas` (orden de la búsqueda) | Resultado |
|---|---|---|
| 0 GEO_PGA | 156→32767, 188→32767, 93→32767, 46→32767, 22→32767, 10→32767, 4→32767, 1→32767, 0→32767; [readback] 156→32767 | `ok=0 dac=156 meas=32767` |
| 1 GEO_BP | 156→32767, 188→32767, 93→-32768, 140→-14297, 164→32767, 152→32767, 146→32767, 143→16147, 141→-9427, 142→-6658; [readback] 142→-6658 | `ok=0 dac=142 meas=-6658` |
| 2 GEO_ADDER | 156→-21064, 188→14216, 93→-32768, 140→-32768, 164→-10352, 176→2256, 170→-4018, 173→-985, 174→1; [readback] 174→1 | `ok=1 dac=174 meas=1` |
| 3 GEO_LP | 156→32767, 188→32767, 93→-32768, 140→-32768, 164→-32768, 176→32767, 170→32767, 167→32767, 165→32767; **CAL_LP_BAD**; [readback] 156→32767 | `ok=0 dac=156 meas=32767` |

Esta secuencia se trazó punto por punto contra el algoritmo real
(`EVAL_INIT`→`EVAL_PROBE`→`PLAN_ITER`/`EVAL_ITER`→`async_finish_stage`,
`calibration.c` líneas ~399-560 y ~309-345) y **coincide exactamente** en las
4 etapas — confirma que la máquina de bisección hace lo que el código dice que
hace. En particular:

- **GEO_ADDER (etapa 2) es un caso de éxito de manual**: `f(156)=-21064` y
  `f(188)=+14216` (signos opuestos, ninguno saturado) → `increasing` se infiere
  correctamente de los dos primeros puntos → bisección limpia hasta
  `dac=174, meas=1, ok=1`. El algoritmo de bisección, cuando el primer par de
  sondeos es informativo, converge perfecto.
- **GEO_BP (etapa 1)** termina en `dac=142, meas=-6658` — fuera de
  `CAL_TOL_COUNTS=250` (por ~26x) pero MUY adentro de
  `CAL_OPERATING_RANGE_COUNTS=26214` (-6658 ≈ -0.13 V de offset). `ok=0` pero
  el valor SE CONSERVA (no hay revert a `CAL_DAC_INIT`, `cal_measured_out_of_range(-6658)`
  es falso). Caso "usable pero no ok" — ver (d).
- **GEO_PGA (etapa 0)**: los 9 códigos sondeados (156,188,93,46,22,10,4,1,0)
  leen TODOS `32767` (clampeado). `best` nunca mejora sobre el punto inicial
  (todos empatan), la búsqueda llega a `dac=0` y termina; revert a
  `CAL_DAC_INIT` (no-op, ya estaba ahí).
- **GEO_LP (etapa 3)**: la bisección converge correctamente hacia el límite
  `[164,165]` — `164→-32768` y `165`(visitado indirectamente vía 176→170→167→165)
  `→+32767`. Cada lectura `+32767` en la zona `[165,176]` empata (no supera,
  `<` estricto) el `best_abs_error=32767` del punto inicial `dac=156`. La
  búsqueda termina con `lo>hi`, `best` sigue siendo el punto inicial
  `(156,32767)`, `cal_measured_out_of_range(32767)`→true→revert a
  `CAL_DAC_INIT` (no-op) + **`CAL_LP_BAD`** porque es la última etapa.

#### b) Blind spot identificado por el advisor (Opus): `cal_diag_i16` CLAMPEA — los "32767"/"-32768" del log NO son necesariamente el riel físico

Antes de escribir esto al HANDOFF, se armó una hipótesis de bug de software
("Finding 1": el fallback `increasing=(direction>=0)` cuando
`measured(188)==measured(156)` sólo explora la MITAD del rango DAC; "Finding
2": `abs_error < best_abs_error` con `<` estricto nunca actualiza `best` ante
un empate exacto). El advisor señaló el blind spot que invalida ambas como
"demostradas con este log":

```c
static void cal_diag_i16(uint8 event, int32 value)
{
    uint16 u;
    if (value > 32767L) { value = 32767L; }
    if (value < -32768L) { value = -32768L; }
    u = (uint16)(int16)value;
    ...
}
```

`measured` internamente es `cum_avg`, un `int32` completo (línea ~272). SÓLO
la telemetría pasa por `cal_diag_i16`, que SATURA a ±32767 antes de mandar 2
bytes. Es decir: **cualquier `|measured|>32767` se ve IDÉNTICO en el log**
("siempre 32767" o "siempre -32768"), pero las comparaciones internas
(`abs_error`, `measured==base_measured`, `cal_measured_out_of_range`) usan el
valor `int32` REAL, no el clampeado.

Consecuencia: los "empates en 32767" de GEO_PGA y de la zona `[165,176]` de
GEO_LP **podrían no ser empates reales** — podrían ser, p. ej., 35000, 50000,
100000 (todos se ven "32767" en el log) — y entonces `increasing` SÍ pudo
inferirse del par real (no del fallback), y `best` SÍ pudo actualizarse vía
`<` estricto. **Finding 1 y Finding 2 quedan SIN CONFIRMAR / posiblemente no
aplicables** — no se descartan para siempre, pero no son la explicación
establecida de este log. No vale la pena perseguirlas más sin datos sin
clamp.

**Lo que SÍ sobrevive intacto** (un cambio de signo no se invierte por
clamping):

- GEO_ADDER funciona perfecto (prueba que la bisección, cuando tiene
  información, converge bien).
- GEO_BP termina en un punto "usable pero no ok" CONSERVADO (-6658, dentro de
  rango).
- **GEO_LP cruza de `-32768` a `+32767` entre `dac=164` y `dac=165` — un
  cambio de signo real entre dos códigos DAC ADYACENTES**. Esto sigue siendo
  la pieza dura del problema: el cruce de "salida muy negativa" a "salida muy
  positiva" para GEO_LP cae dentro de **1 LSB del VDAC de referencia (≈16
  mV)**, visto con la telemetría clampeada. Lo que NO sabemos todavía es la
  MAGNITUD real de `f(164)` y `f(165)` (¿-40000/+38000? ¿-27000/+27500?) — eso
  decide si esto es "apenas fuera de rango" (arreglable) o "saturación dura"
  (no arreglable por DAC).

#### c) Observación del usuario: calibración manual a 16mV YA logró resultados usables

Mensaje del usuario (verbatim), llegado durante la investigación de (a)/(b):

> "Lo que pasa tambien es que hay veces que llega a ser usable y esta bien
> pero seguis moviendo así que no te creo que no se pueda con una resolución
> de 16mV que es lo que tenemos actualmente, pues yo calibraba a mano en zonas
> usables esto y lograba deajar algo funcional, así que una cpu deberia
> tambein [poder]"

Esto es evidencia empírica directa (no se deriva del código): **el usuario ya
encontró, a mano, códigos VDAC (16 mV/LSB, el mismo DAC que usa el firmware)
que dejan el front-end en un estado funcional**. Esto:

1. Refuerza la prioridad de (b): hay que ver los valores SIN CLAMP antes de
   concluir "imposible con este DAC" para GEO_PGA/GEO_LP.
2. Aporta una pista de comportamiento adicional — "a veces llega a ser usable
   y esta bien pero seguís moviendo" — es decir, durante la búsqueda el
   sistema PASA por puntos razonables (ej. GEO_BP en -6658, ~-0.13V, es
   bastante "usable" en términos prácticos aunque `ok=0` por
   `CAL_TOL_COUNTS=250`) pero el criterio de paro actual (`abs_error<=250`,
   chequeo SOLO en el punto actual) no los reconoce como "suficientemente
   buenos" — sigue iterando. El caso GEO_BP (-6658, conservado porque está en
   rango) muestra que el VALOR final SÍ puede ser razonable aunque `ok=0`; el
   caso GEO_PGA/GEO_LP (revert a `CAL_DAC_INIT`) es el que hay que mirar con
   datos sin clamp para saber si está "apenas" o "muy" fuera de
   `CAL_OPERATING_RANGE_COUNTS=26214`.

#### d) Propuesta del usuario: PID/PI continuo por etapa — IDEA, NO IMPLEMENTADA

Mensaje del usuario (verbatim):

> "No crees que capaz sea más probable que converja si hacemos un PID para
> cada operacional que sigue operando eternamente hasta que llegue la señal de
> VER o START? capaz funciona mejor por la parte integral que se beneficia del
> tiempo. Pregunto es para que mandes al advisor y pongas, literal podriamos
> poner unos PID conservadores, probablemente PI nomás quita la D para no
> amplificar el ruido, y poniendo esto bien conservador podemos capaz obtener
> que se autocalibre mientras va iniciando. Igual deja como idea esto en el md
> primero veamos que hacemos con busqueda binaria pero bueno capaz podes
> mandarle a advisor para ver si es que vale la pena, porque hasta ahora no
> logramos que funcione la puerqueza con la busqueda binaria no sé muy bien
> que hacer más, no sé que más verificar"

Se consultó al advisor (Opus 4.8) con esta propuesta + el análisis de
(a)/(b). Resumen de su respuesta:

- **PID es ortogonal al fallo real**: un PI corriendo sobre el MISMO DAC de
  8 bits/16mV pega contra el MISMO piso de cuantización. Para un cruce
  sub-LSB (GEO_LP entre 164 y 165), un lazo PI no converge — *limit-cycla*
  entre los dos códigos adyacentes con el integrador "wind-up", lo cual es
  PEOR que el "me rindo" limpio de la bisección, no mejor. El promediado que
  el usuario espera de la parte integral YA lo hace
  `async_measure_service` (promediado acumulativo, ver §14.10b).
- **El núcleo salvable de la idea NO es "PID" sino "dithering"**: alternar
  (PWM) entre dos códigos DAC adyacentes para conseguir una resolución de
  referencia EFECTIVA sub-LSB. Esa es la ÚNICA técnica que podría calibrar
  GEO_LP — y SÓLO SI la etapa tiene una pendiente finita (aunque empinada)
  entre 164 y 165, no si está físicamente pegada a un riel en TODO el rango
  alcanzable. Esto se decide con los datos sin clamp de (b)/(f).
- Recomendación: **dejar la idea documentada (acá) como "dithering
  PWM-entre-códigos-adyacentes, eventualmente impulsado por un integrador
  PI conservador"**, pero NO implementarla todavía — primero conseguir UNA
  corrida con telemetría sin clamp (f) y ver si GEO_PGA/GEO_LP tienen algún
  código (o par de códigos adyacentes) con magnitud real cerca de
  `CAL_OPERATING_RANGE_COUNTS`. Si sí → dithering tiene sentido. Si está
  saturado en TODO el rango con magnitudes enormes → es un problema de
  ganancia de hardware, ningún controlador (bisección, PID o dithering)
  operando sobre este VDAC lo resuelve.

**No se tocó el algoritmo de búsqueda/convergencia ni se implementó PID ni
dithering esta sesión** — sólo se aplicó el fix de telemetría de (f), que es
el prerequisito de datos que tanto el advisor como la observación (c) del
usuario piden antes de elegir entre bisección-arreglada / dithering /
rediseño de hardware.

#### e) Fix aplicado esta sesión: telemetría SIN CLAMP — nuevo evento `PSOC_EVT_CAL_STAGE_MEAS32` (0x1D) / log `CAL_POINT32`

Cambio mínimo y aditivo: además de la telemetría clampeada existente
(`PSOC_EVT_CAL_STAGE_MEAS`, 2 bytes, ±32767), se manda AHORA TAMBIÉN el
`int32 measured` COMPLETO (4 bytes MSB-first, sin clamping) bajo un evento
nuevo. No se quitó ni modificó ningún evento existente — cero riesgo para
parsers/JS actuales (verificado: `master/data/js/slave_panel.js` no
referencia `LOGM`/`CAL_POINT` por nombre, así que una línea de log nueva no
rompe nada).

Archivos tocados:

- `psoc_hw.h` (PSoC): `#define PSOC_EVT_CAL_STAGE_MEAS32 0x1Du` (después de
  `PSOC_EVT_CAL_LP_BAD=0x1Cu`; `0x1D` estaba libre, el próximo usado era
  `PSOC_EVT_WAIT_ESP=0x20u`).
- `calibration.c`:
  - nueva `cal_diag_i32(uint8 event, int32 value)` (después de
    `cal_diag_i16`, ~línea 51) — manda los 4 bytes crudos de `value` (sin
    saturar) con el mismo framing que `cal_diag_i16` (N llamadas a
    `cal_diag` con el mismo `event`).
  - `cal_diag_point()`: ahora también llama
    `cal_diag_i32(PSOC_EVT_CAL_STAGE_MEAS32, measured)` — se emite por CADA
    punto sondeado durante la búsqueda/verify (mismo call site que ya emitía
    `CAL_STAGE_DAC` + `CAL_STAGE_MEAS`).
  - `async_finish_stage()`: idem, después de
    `cal_diag_i16(PSOC_EVT_CAL_STAGE_MEAS, result->final_measured)` — el
    resultado FINAL de cada etapa también viaja sin clamp.
- `slave/src/psoc_uart.h` (ESP): mismo `#define PSOC_EVT_CAL_STAGE_MEAS32
  0x1D`.
- `slave/src/main.cpp`:
  - `psocDiagName()`: nuevo caso `"CAL_STAGE_MEAS32"`.
  - nuevas estáticas `calMeasRaw` (`uint32_t`) / `calMeasRawByte` (`uint8_t`,
    contador 0-3), reseteadas junto con `calMeasHigh` en
    `CAL_STAGE_BEGIN`/`CAL_VERIFY_BEGIN` (mismo patrón que el ensamblado de 2
    bytes ya existente para `calMeas`).
  - nueva rama `else if (event.event == PSOC_EVT_CAL_STAGE_MEAS32)`: ensambla
    4 bytes big-endian en `calMeasRaw`, y al completar imprime:
    - `[SLAVE] CAL point32 stage=X/NOMBRE dac=D measRaw=<int32 real>`
    - `LOGM("CAL_POINT32", "stage=X,name=...,dac=D,measRaw=<int32 real>")`

**Build verificado**: `pio run -e slave2` → SUCCESS, mismo footprint
(RAM 13.5% / Flash 56.3%). El lado PSoC (`calibration.c`/`psoc_hw.h`) NO se
puede compilar desde CLI (requiere el fitter de PSoC Creator, ver §2) — los
cambios son sintácticamente análogos a `cal_diag_i16`/`cal_diag_point` (mismos
tipos `uint8`/`int32`/`uint32` ya usados en ese archivo), pero **falta
compilar+programar en PSoC Creator** antes de la próxima corrida.

#### f) Qué traer para la próxima corrida

1. Abrir `AcondicionamientoAnalogico.cydsn` en PSoC Creator, compilar (genera
   el fitter de nuevo, sin cambios de TopDesign) y programar.
2. `pio run -t upload` para `slave2` (ya verificado que compila).
3. Correr "Calibrar" y pegar el log COMPLETO sin cortar — ahora cada punto
   tendrá DOS líneas:
   ```
   [SLAVE] CAL point stage=3/GEO_LP dac=164 meas=-32768
   [SLAVE] CAL point32 stage=3/GEO_LP dac=164 measRaw=<valor real>
   ```
4. Con `measRaw` real para GEO_PGA (los 9 puntos, todos "32767" clampeado) y
   GEO_LP (en particular `dac=164` y `dac=165`), responde:
   - **¿Algún `measRaw` cae cerca de `±26214` (p. ej. entre ±26000 y
     ±35000)?** → "apenas fuera de rango", posiblemente arreglable ajustando
     `CAL_OPERATING_RANGE_COUNTS`/búsqueda, o con dithering entre los códigos
     vecinos.
   - **¿Todos los `measRaw` son enormes (decenas/cientos de miles) en TODO el
     rango sondeado?** → saturación dura de hardware; ningún algoritmo sobre
     este VDAC lo arregla — replantear ganancia de GEO_PGA/GEO_LP o el punto
     de inyección de la referencia (pregunta de hardware, no de firmware).
   - Para GEO_LP específicamente: ¿`measRaw(164)` y `measRaw(165)` son
     "moderados" (cerca del rango operativo) o "extremos"? Esto decide si
     dithering 164↔165 (idea de (d)) tiene sentido.

### 14.13 Sesion 2026-06-14: criterio de parada demasiado fino para GEO y autocalibracion de arranque obligatoria

Log nuevo analizado: la busqueda si encuentra puntos usables en etapas
intermedias, pero `CAL_TOL_COUNTS=250` era demasiado fino para un VDAC de 8
bits dentro de una cadena analogica en cascada. Ejemplos del log:

- `GEO_BP`: `dac=140`, `measRaw=1459`.
- `GEO_ADDER`: `dac=174`, `measRaw=341`.

Con tolerancia 250 esos puntos se rechazaban como "no suficientemente buenos",
asi que la busqueda seguia probando codigos y podia volver a caer en
saturacion. El sintoma visto por el usuario ("llega a un valor razonable y
luego sigue moviendo hasta saturar") coincide con ese criterio de parada.

Cambios aplicados:

- `main.c`: `PSOC_STARTUP_FULL_CAL_ENABLE` vuelve a `1`. El nodo debe hacer
  una calibracion completa al inicio, despues de detectar ESP/UART, sin esperar
  al boton "Calibrar".
- `main.c`: el servo PI no se ejecuta mientras `startup_cal_pending` esta
  activo. La calibracion completa arranca primero; el servo queda solo como
  mantenimiento lento posterior.
- `calibration_tables.h`: tolerancias GEO iniciales mas realistas:
  - `GEO_PGA = 5000 counts`
  - `GEO_BP = 5000 counts`
  - `GEO_ADDER = 2000 counts`
  - `GEO_LP = 5000 counts`
- `calibration_tables.h`: deadband del servo GEO sube a `5000 counts` por
  defecto (`GEO_ADDER = 2000 counts`) y `CAL_SERVO_RECOVERY_STEP` baja de 4 a
  1 LSB para evitar que el mantenimiento lento persiga saturaciones con pasos
  agresivos.

Verificacion local: `git diff --check` sin errores; solo warnings normales de
LF/CRLF. No se compilo ni programo PSoC porque Elias pidio hacerlo manualmente.

### 14.14 Sesion 2026-06-14: memoria de candidatos, deteccion de loop y rango de DAC por `center +/- max_change`

Pedido nuevo del usuario: evitar loops eternos de calibracion, no mover varias
etapas caoticamente, guardar mejores candidatos y limitar la busqueda sin
hardcodear rangos absolutos tipo `125..188`.

Cambios aplicados:

- `PsocCalStage` ahora incluye `dac_center` y `dac_max_change`. El rango de
  busqueda de cada operacional se calcula como:
  `dac_center - dac_max_change` .. `dac_center + dac_max_change`, con clamp
  solo al rango fisico del DAC de 8 bits.
- `calibration_tables.h` define macros tuneables por etapa:
  - `CAL_DAC_CENTER_GEO_PGA/BP/ADDER/LP`
  - `CAL_DAC_MAX_CHANGE_GEO_PGA/BP/ADDER/LP`
  - equivalentes Hammer para futuro.
  Por ahora GEO usa centro `0x9C` y `MAX_CHANGE=32`; ya no hay limites
  absolutos GEO hardcodeados.
- La calibracion larga guarda una lista pequena de candidatos:
  `CAL_BEST_CANDIDATE_COUNT=4`, con `dac`, `measured` y `abs_error`.
  La medicion guardada es el promedio acumulado estable que sale de
  `async_measure_service()`, no una muestra instantanea.
- La calibracion larga guarda los ultimos `CAL_VISIT_HISTORY_COUNT=8` DACs.
  Si vuelve a pedir un DAC ya visitado, emite `PSOC_EVT_CAL_LOOP`, corta esa
  etapa y escribe el mejor candidato visto.
- El servo de mantenimiento queda mas secuencial:
  - si una etapa no queda dentro de deadband y el paso mejora, sigue en esa
    misma etapa;
  - si el paso empeora, restaura el punto anterior, emite `CAL_LOOP` y recien
    ahi pasa a la siguiente etapa;
  - todo paso del servo tambien queda limitado por `center +/- max_change`.
- ESP esclavo entiende `PSOC_EVT_CAL_LOOP=0x22` y loguea:
  `CAL loop stage=... dac=... -> usando mejor candidato`.

Verificacion:

- `pio run -e slave2` en `src/esp/Nodo comunicacion/slave` -> SUCCESS
  (RAM 13.5%, Flash 56.4%).
- `git diff --check` acotado a fuentes tocadas -> sin errores, solo warnings
  normales LF/CRLF.
- `git diff --check` global no es util ahora porque hay artefactos PSoC
  `CortexM3/Debug/*.lst/*.map` modificados con trailing whitespace generado;
  no fueron revertidos.

### 14.15 Sesion 2026-06-14: simplificacion agresiva para que lockee y avance

Log nuevo del usuario mostro `GEO_BP dac=147 measRaw=-19894`. Eso estaba
dentro del rango operativo (`|-19894| < 26214`) pero fuera de la tolerancia
anterior (`5000`), asi que el firmware seguia buscando. Para salir del loop y
priorizar funcionamiento:

- Las tolerancias GEO ahora son `CAL_OPERATING_RANGE_COUNTS` para todas las
  etapas. Si cae dentro de +-0.5V diferencial, se lockea ese VDAC y se pasa a
  la siguiente etapa.
- `async_finish_stage()` ya no vuelve al centro/default cuando el mejor
  candidato queda fuera de rango. Siempre escribe el mejor candidato real que
  encontro; si esta fuera de rango, reporta `ok=0` y en LP tambien
  `CAL_LP_BAD`, pero NO pisa el VDAC por default.
- `async_finish_stage()` recalcula `ok=1` si el mejor candidato guardado entra
  en tolerancia aunque el camino de salida haya sido loop/max-iter.
- El servo PI de mantenimiento queda apagado desde `main.c`
  (`psoc_calibration_servo_enable(0u)`) y `CAL_SERVO_ENABLE_DEFAULT=0u`.
  Por ahora queda solo la calibracion inicial/secuencial; nada debe seguir
  tocando los VDACs despues.

Con este firmware, el punto del log `GEO_BP dac=147 measRaw=-19894` debe
producir `CAL_STAGE_OK=1` y luego `CAL_BEGIN stage=2/GEO_ADDER`.

### 14.16 Sesion 2026-06-15: redisenio de calibracion por VDAC, ventana deslizante, realcheck y boot-cal autonoma

Cambios implementados sobre el estado parcial dejado por Claude/Codex:

- `calibration_tables.h` queda como agregador global y cada VDAC GEO tiene su
  propio header: `calibration_tables_geo_pga.h`, `_bp.h`, `_adder.h`, `_lp.h`.
  Ahi viven target, direccion, rango de busqueda, probe step, tolerancia,
  descartes, promediado de biseccion/verify/realcheck, saturacion, nudge y
  parametros PI del servo.
- `PsocCalStage` ahora usa `PsocCalAvgCfg` y `PsocCalRealcheckCfg`. La
  calibracion principal sigue siendo biseccion en cascada; el PI sigue siendo
  solo servo lento y permanece deshabilitado por defecto.
- `async_measure_service()` ya no usa promedio acumulativo. Ahora usa un buffer
  circular de sumas-de-ventana: `avg_n * window_count` muestras con peso fijo,
  racha estable `stable_streak` y techo `max_samples`. Defaults GEO:
  biseccion 64x16/2048, verify 64x16/4096, realcheck 64x64/8192.
- Anti-saturacion: cada etapa tiene `CAL_SAT_COUNTS_GEO_*` (inicial 120000).
  Candidatos saturados no pueden ser mejor candidato si existe al menos un
  candidato no saturado. Si todos saturan, se conserva el menor error para
  diagnostico y se emite `PSOC_EVT_CAL_STAGE_SAT_ALL`.
- Fase final `realcheck`: despues de verify, el PSoC cambia a
  `AMux_IN_Select(0)`, mide las cuatro etapas GEO con la entrada real y aplica
  nudges de +/-1 LSB hasta 3 veces por etapa. Un nudge solo se acepta si mejora
  `|error|` y no satura; si empeora o satura, se revierte y se termina esa
  etapa.
- Nuevos eventos PSoC/ESP: `CAL_STAGE_SAT`, `CAL_STAGE_SAT_ALL`,
  `CAL_REALCHECK_BEGIN`, `CAL_REALCHECK_DAC`, `CAL_REALCHECK_MEAS32`,
  `CAL_REALCHECK_NUDGE`, `CAL_REALCHECK_OK`.
- Boot-cal autonoma: `PSOC_STARTUP_FULL_CAL_ENABLE=1`. El PSoC calibra al
  arrancar despues del handshake con ESP. Hasta que termine esa primera
  calibracion, el parser UART solo acepta PONG/status; arm/start/debug/config
  responden `CAL_BUSY`/ACK 0.
- El ESP slave ya no solicita auto-cal al detectar el PSoC
  (`PSOC_AUTO_CAL_ON_READY=0`), pero el comando manual de calibracion sigue
  existiendo para banco. El timeout de calibracion del slave sube a 450 s; el
  watchdog PSoC queda en 400 s.
- Visibilidad maestro/UI: el slave reusa `MsgCfgAck` para progreso autonomo:
  `2` generico, `3..6` biseccion por etapa, `7..10` verify, `11..14`
  realcheck, `0/1` resultado final. `slave_panel.js` vuelve a mostrar el punto
  de calibracion y `app.js` trata `ok>=2` como progreso sin consumir el ACK
  final.

Pendiente explicito: el split de `main.c` en modulos (`psoc_uart_proto`,
`psoc_capture`, `psoc_uart_cmds`) queda diferido hasta confirmar que esta
calibracion compila y corre en hardware. No se hizo push, flash ni upload.

### 14.17 Cierre 2026-06-15: estado listo para continuar manana

Estado final de esta tanda:

- El plan de calibracion quedo implementado en codigo fuente. No se subio,
  no se flasheo y no se pusheo nada.
- No usar el workspace viejo para validar esta parte. La validacion correcta
  es sobre el proyecto `AcondicionamientoAnalogico` y su dependencia local
  `src/psoc/Analog_LPF_v1_0.cylib`.
- El build PSoC se verifico en una copia temporal fuera del repo, con un
  workspace temporal que contiene solo `AcondicionamientoAnalogico.cyprj`.
  Esa copia temporal se elimino al terminar. Resultado: build OK.
- El build ESP slave se verifico con PlatformIO. Resultado: OK.
- El arbol de trabajo quedo sin artefactos generados de PSoC Creator; solo
  quedan fuentes/docs/JS y los headers GEO nuevos.

Archivos principales modificados:

- PSoC: `calibration.c`, `calibration.h`, `calibration_tables.h`,
  `psoc_hw.h`, `main.c`.
- PSoC nuevos headers por VDAC:
  `calibration_tables_geo_pga.h`, `calibration_tables_geo_bp.h`,
  `calibration_tables_geo_adder.h`, `calibration_tables_geo_lp.h`.
- ESP slave: `src/psoc_uart.h`, `src/main.cpp`, `platformio.ini`.
- UI master: `data/js/app.js`, `data/js/slave_panel.js`.
- Handoff: este archivo.

Comandos usados para verificar:

```powershell
git diff --check -- `
  'src/psoc/AcondicionamientoAnalogico.cydsn/calibration.c' `
  'src/psoc/AcondicionamientoAnalogico.cydsn/calibration.h' `
  'src/psoc/AcondicionamientoAnalogico.cydsn/calibration_tables.h' `
  'src/psoc/AcondicionamientoAnalogico.cydsn/psoc_hw.h' `
  'src/psoc/AcondicionamientoAnalogico.cydsn/main.c' `
  'src/esp/Nodo comunicación/slave/src/psoc_uart.h' `
  'src/esp/Nodo comunicación/slave/src/main.cpp' `
  'src/esp/Nodo comunicación/slave/platformio.ini' `
  'src/esp/Nodo comunicación/master/data/js/app.js' `
  'src/esp/Nodo comunicación/master/data/js/slave_panel.js' `
  'src/psoc/AcondicionamientoAnalogico.cydsn/HANDOFF_CALIBRATION.md'

cd 'src/esp/Nodo comunicación/slave'
pio run
```

Build PSoC validado:

- Se creo una copia temporal de `src/psoc/AcondicionamientoAnalogico.cydsn`
  y `src/psoc/Analog_LPF_v1_0.cylib`.
- Se genero ahi un workspace temporal `AcondicionamientoAnalogico.cywrk` con
  un solo proyecto: `AcondicionamientoAnalogico.cyprj`.
- Se ejecuto:
  `cyprjmgr.exe -wrk <temp>\AcondicionamientoAnalogico.cydsn\AcondicionamientoAnalogico.cywrk -prj AcondicionamientoAnalogico -build -c Debug`.
- Resultado: `Build Succeeded`, flash `23864/262144`, SRAM `49464/65536`.

Primeros pasos manana:

1. Abrir solo `src/psoc/AcondicionamientoAnalogico.cydsn/AcondicionamientoAnalogico.cyprj`
   en PSoC Creator si se quiere revisar visualmente.
2. Compilar/programar PSoC desde `AcondicionamientoAnalogico`; evitar usar
   `DiferencialToSingleEnded_ESP-000` como referencia para este trabajo.
3. Programar los ESP slave si corresponde.
4. Encender y mirar logs esperados:
   `CAL_START`, `CAL_STAGE_*`, `CAL_STAGE_SAT*`, `CAL_VERIFY_*`,
   `CAL_REALCHECK_*`, `CAL_DONE`.
5. Confirmar en UI que el punto `Calibracion` pasa por busy/fase/etapa y
   termina ok/fail.

Pendientes reales:

- Prueba en hardware de boot-cal autonoma completa.
- Ajuste fino de valores GEO si el realcheck muestra saturacion o nudges
  sistematicos.
- Split/refactor de `main.c` a modulos separados solo despues de confirmar
  calibracion en hardware.
- Cuando se prepare commit, no olvidar agregar los cuatro headers GEO nuevos
  que hoy estan como archivos no trackeados.

### 14.18 Sesion 2026-06-15 (cierre): auto-cal no disparaba — desajuste ESP/PSoC sin flashear

Sintoma reportado: tras programar el ESP slave (S1, COM9) con los cambios de
"anoche", la auto-calibracion no arranca sola al boot. El log de
`platformio device monitor` muestra `PSoC: DETECTADO` y `listo, esperando
ARM`, pero nunca aparece `CAL_START`/`CAL_DONE` cerca del boot — solo eventos
`CAL_AMUX_IN` horas despues (arm/start manual), sin telemetria de etapas.

Causa: desajuste de despliegue, no un bug de logica.

- El cambio de "anoche" movio el disparo de auto-cal del ESP al PSoC:
  `PSOC_STARTUP_FULL_CAL_ENABLE` 0→1 en `main.c` (PSoC) + gating con
  `g_startup_cal_done`, y en el ESP `PSOC_AUTO_CAL_ON_READY` 1→0 (deja de
  pedir 0xB5 al detectar el PSoC).
- Pero, segun §14.17, el PSoC **nunca se flasheo** ("no se flasheo"). El PSoC
  que esta corriendo en la placa sigue siendo el firmware viejo
  (`PSOC_STARTUP_FULL_CAL_ENABLE=0`, sin auto-cal propia).
- Resultado: el unico lado que se actualizo en hardware (el ESP) quedo con
  `PSOC_AUTO_CAL_ON_READY=0` (no pide cal), y el PSoC viejo tampoco se
  autocalibra al boot. Ningun lado dispara nada — exactamente el sintoma
  observado (sin `CAL_START` cerca del boot).

Fix aplicado (solo ESP, `src/esp/Nodo comunicación/slave/src/main.cpp`):

- `PSOC_AUTO_CAL_ON_READY` vuelto a `1` (linea ~86). Restaura el disparo
  ESP→PSoC (`requestPsocCalibration("AUTO")` via 0xB5) al detectar el PSoC,
  igual que en HEAD (`4a827551`). Funciona contra el PSoC viejo actualmente
  flasheado sin tocarlo.
- Logging agregado en `scheduleAutoCalibration()`/`serviceAutoCalibration()`
  (eventos `[AUTO_CAL] scheduled...`, `[AUTO_CAL] deferred (...)`,
  `[AUTO_CAL] due -> requesting calibration`, `[AUTO_CAL] request
  failed...`, mas `LOGM("AUTO_CAL", ...)` para el log del maestro) para poder
  ver en el monitor serie por que una auto-cal se programa, se posterga o se
  dispara.
- Build verificado: `pio run -e slave2` → SUCCESS.
- **No se toco el PSoC**: `PSOC_STARTUP_FULL_CAL_ENABLE=1` y
  `g_startup_cal_done` quedan en el arbol de trabajo tal cual los dejo la
  sesion de anoche (inertes hasta que se flashee el PSoC).

Importante para la proxima sesion / si se flashea el PSoC nuevo:

- Con el PSoC nuevo (`PSOC_STARTUP_FULL_CAL_ENABLE=1`), el PSoC se
  autocalibra solo al boot. Si en ese momento el ESP sigue con
  `PSOC_AUTO_CAL_ON_READY=1`, **ambos lados disparan** una calibracion: el
  PSoC la propia al boot, y el ESP cuando detecta el PSoC (reintenta cada
  `PSOC_AUTO_CAL_RETRY_MS`=3000ms mientras el PSoC responde `CAL_BUSY`
  durante su boot-cal). Cuando el PSoC termina su boot-cal
  (`g_startup_cal_done=1`), el siguiente reintento del ESP puede arrancar
  una **segunda** calibracion completa.
- Decision pendiente para cuando se flashee el PSoC nuevo: volver a poner
  `PSOC_AUTO_CAL_ON_READY=0` en el ESP (boot-cal queda 100% del lado PSoC), o
  dejarlo en 1 como red de seguridad aceptando una posible doble corrida la
  primera vez.
