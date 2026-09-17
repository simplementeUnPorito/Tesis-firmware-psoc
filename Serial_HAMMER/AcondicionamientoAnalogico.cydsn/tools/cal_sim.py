#!/usr/bin/env python3
"""
Simulacion en Python del lazo de auto-calibracion (PID Q12 + media movil) de
calibration.c, usando las constantes reales de calibration_tables.h.

Objetivo: sin hardware, explorar:
  (A) cuantas iteraciones tarda en converger segun la ganancia de la planta
      (counts de ADC por LSB de DAC), y a partir de que ganancia
      CAL_TOL_COUNTS=250 deja de ser alcanzable (saturacion/oscilacion).
  (B) que pasa si el signo `direction` de una etapa no coincide con el signo
      real de la planta (pregunta abierta de HANDOFF_CALIBRATION.md #4).
  (C) que pasa si el target esta fuera del rango fisico alcanzable por el DAC
      0..255 (saturacion).
  (D) tiempo total estimado de psoc_calibration_run_blocking() para Geo (4
      etapas) y Hammer (3 etapas) bajo distintas ganancias supuestas, para
      contrastar contra PSOC_CAL_ACK_TIMEOUT_MS (HANDOFF #5).
  (E)-(G) ejemplo realista Geo + clasificacion/verificacion de los dos
      regimenes de FAIL (RESOLUCION vs DINAMICA, ver HANDOFF #11.A/F/G).
  (H) sensibilidad del PID a ruido gaussiano del ADC promediado con
      CAL_AVG_N=32 (make_noisy_plant, opt-in; ver HANDOFF #11.H).

Es un puerto 1:1 de calibrate_stage() en calibration.c — incluye el detalle de
que `control >>= 12` en C (ARM GCC, ints con signo) es un shift aritmetico =
floor division por 2^12, que es exactamente lo que hace el operador `>>` de
Python sobre enteros (con signo, precision arbitraria). No hace falta
reimplementar manualmente la division.

Uso:
    python cal_sim.py
"""

import random
from dataclasses import dataclass, field
from typing import Callable, Optional

# ---------------------------------------------------------------------------
# Constantes de calibration_tables.h / calibration.c (NO inventar valores
# nuevos aca - si cambian en el .h, actualizar aca tambien)
# ---------------------------------------------------------------------------
CAL_PID_KP_Q12 = 4          # calibration_tables.h:58
CAL_PID_KI_Q12 = 1          # calibration_tables.h:59
CAL_PID_KD_Q12 = 0          # calibration_tables.h:60
CAL_AVG_N = 32              # calibration_tables.h:61 (no afecta la sim determinista)
CAL_MAX_ITER = 90           # calibration_tables.h:62
CAL_TOL_COUNTS = 250        # calibration_tables.h:63
CAL_DAC_INIT = 128          # calibration_tables.h:64
CAL_INTEGRAL_LIMIT = 400000  # calibration.c:21

CAL_TARGET_1V_COUNTS = 52429   # calibration_tables.h:23 == ADC_CFG1_COUNTS_PER_VOLT
CAL_TARGET_1V5_COUNTS = 78644  # calibration_tables.h:27 (~52429*1.5)

ADC_FULL_SCALE = 131071  # 2^17-1, techo plausible para 18 bits con signo (solo clip)


# ---------------------------------------------------------------------------
# Puerto 1:1 de calibration.c
# ---------------------------------------------------------------------------
def clamp_dac_code(value: int) -> int:
    if value < 0:
        return 0
    if value > 255:
        return 255
    return value


def clamp_integral(value: int) -> int:
    if value > CAL_INTEGRAL_LIMIT:
        return CAL_INTEGRAL_LIMIT
    if value < -CAL_INTEGRAL_LIMIT:
        return -CAL_INTEGRAL_LIMIT
    return value


@dataclass
class StageResult:
    converged: bool
    iterations: int           # iteraciones efectivamente ejecutadas (<= max_iter)
    final_dac: int             # lo que quedo escrito en el VDAC al salir
    best_dac: int
    best_abs_error: int
    history: list = field(default_factory=list)  # (iter, dac_code, measured, error)


def calibrate_stage(target_counts: int, direction: int,
                     plant_fn: Callable[[int], int],
                     kp=CAL_PID_KP_Q12, ki=CAL_PID_KI_Q12, kd=CAL_PID_KD_Q12,
                     max_iter=CAL_MAX_ITER, tol=CAL_TOL_COUNTS,
                     initial_dac=CAL_DAC_INIT) -> StageResult:
    """Puerto directo de calibrate_stage() en calibration.c (sin AMux/CyDelay)."""
    integral = 0
    prev_error = 0
    dac_code = initial_dac
    best_dac = initial_dac
    best_abs_error = 0x7FFFFFFF
    history = []

    for it in range(max_iter):
        measured = plant_fn(dac_code)
        error = target_counts - measured
        abs_error = abs(error)
        derivative = error - prev_error

        history.append((it, dac_code, measured, error))

        if abs_error < best_abs_error:
            best_abs_error = abs_error
            best_dac = dac_code

        if abs_error <= tol:
            return StageResult(True, it + 1, dac_code, best_dac, best_abs_error, history)

        integral = clamp_integral(integral + error)
        control = (kp * error + ki * integral + kd * derivative) >> 12  # arithmetic shift, matches ARM GCC
        if direction < 0:
            control = -control

        if control == 0:
            control = direction if error >= 0 else -direction

        dac_code = clamp_dac_code(dac_code + control)
        prev_error = error

    return StageResult(False, max_iter, best_dac, best_dac, best_abs_error, history)


# ---------------------------------------------------------------------------
# Modelo de planta: measured(dac) = clip(offset + gain*dac, 0, ADC_FULL_SCALE)
# ---------------------------------------------------------------------------
def make_linear_plant(gain: float, offset: float) -> Callable[[int], int]:
    def plant(dac_code: int) -> int:
        val = offset + gain * dac_code
        if val < 0:
            val = 0
        if val > ADC_FULL_SCALE:
            val = ADC_FULL_SCALE
        return int(round(val))
    return plant


# ---------------------------------------------------------------------------
# Modelo de ruido del ADC: envuelve una planta determinista para reproducir
# avg_counts() (calibration.c:42-51), llamada UNA vez por iteracion del PID
# (calibration.c:68, `measured = avg_counts(stage->avg_n)`):
#   for i in range(avg_n): acc += psoc_adc_read_single_counts()  # 1 lectura c/u
#   return acc / avg_n  # division entera C, truncada hacia 0
# Aca cada "lectura individual" = valor determinista de plant_fn + ruido
# gaussiano aditivo (sigma counts), clippeada a [0, ADC_FULL_SCALE] (un ADC no
# devuelve negativos ni > full scale), sumada y dividida con `//` (== truncar
# de C para acumulador no-negativo). OPT-IN: no toca calibrate_stage() ni
# make_linear_plant(), por lo que (A)-(G) siguen siendo deterministas/reproducibles.
# ---------------------------------------------------------------------------
def make_noisy_plant(base_plant_fn: Callable[[int], int], sigma: float,
                      avg_n: int = CAL_AVG_N,
                      rng: Optional[random.Random] = None) -> Callable[[int], int]:
    if rng is None:
        rng = random.Random()
    if avg_n <= 0:
        avg_n = 1

    def plant(dac_code: int) -> int:
        true_val = base_plant_fn(dac_code)
        acc = 0
        for _ in range(avg_n):
            sample = int(round(true_val + rng.gauss(0.0, sigma)))
            if sample < 0:
                sample = 0
            elif sample > ADC_FULL_SCALE:
                sample = ADC_FULL_SCALE
            acc += sample
        return acc // avg_n

    return plant


# ---------------------------------------------------------------------------
# (A) Barrido de ganancia: offset=0, plant(dac)=clip(gain*dac, 0, FULL_SCALE),
#     dac arranca en CAL_DAC_INIT=128 (NO en el optimo). direction=+1 (valor
#     hardcodeado hoy en calibration_tables.h para TODAS las etapas).
#     Umbrales relevantes para gain (counts/LSB) con target=52429, kp=4:
#       - reachability: gain >= target/255 = 205.6  (si no, ni dac=255 alcanza
#         measured>=target-tol)
#       - granularity (dado reachable): gain <= 2*tol = 500 garantiza que
#         existe ALGUN codigo dentro de tol del target
#       - deadbeat: gain ~= 4096/kp = 1024 (1 iteracion corrige casi todo el error)
#       - estabilidad lineal: |1 - gain*kp/4096| < 1  =>  gain < 2*4096/kp = 2048
# ---------------------------------------------------------------------------
def true_optimum(target_counts: int, plant_fn: Callable[[int], int]):
    """Mejor dac en 0..255 por busqueda exhaustiva (referencia para juzgar al PID)."""
    best_dac, best_err = 0, abs(target_counts - plant_fn(0))
    for dac in range(1, 256):
        err = abs(target_counts - plant_fn(dac))
        if err < best_err:
            best_err, best_dac = err, dac
    return best_dac, best_err


def experiment_gain_sweep():
    print("=" * 78)
    print("(A) Barrido de ganancia counts/LSB, dac arranca en CAL_DAC_INIT=128 (offset=0)")
    print(f"    target=CAL_TARGET_1V_COUNTS={CAL_TARGET_1V_COUNTS}, "
          f"tol={CAL_TOL_COUNTS}, max_iter={CAL_MAX_ITER}, direction=+1")
    print(f"    Umbrales: reachability gain>={CAL_TARGET_1V_COUNTS/255:.1f}, "
          f"granularity gain<={2*CAL_TOL_COUNTS} (garantiza existencia de codigo "
          "dentro de tol; por encima depende de target mod gain), "
          "estabilidad gain<2048")
    print("    'optimo?' compara best_dac del PID contra el optimo global "
          "(busqueda exhaustiva 0..255) - si es 'si' pero FAIL, es un problema "
          "de RESOLUCION (gain>2*tol), no de sintonia del PID.")
    print("-" * 78)
    target = CAL_TARGET_1V_COUNTS
    gains = [1, 50, 100, 150, 200, 205, 206, 250,
             300, 400, 500, 600, 800, 1000, 1024, 1200, 1500, 2000, 2048, 2100, 3000, 5000, 10000]
    print(f"{'gain':>8} {'resultado':>10} {'iters':>6} {'best_dac':>9} "
          f"{'best_|err|':>11} {'true_best':>10} {'true_|err|':>11} {'optimo?':>8}")
    for gain in gains:
        plant = make_linear_plant(gain, 0)
        r = calibrate_stage(target, +1, plant)
        true_dac, true_err = true_optimum(target, plant)
        status = "OK" if r.converged else "FAIL"
        is_optimal = "si" if r.best_dac == true_dac else "NO"
        print(f"{gain:8d} {status:>10} {r.iterations:6d} {r.best_dac:9d} "
              f"{r.best_abs_error:11d} {true_dac:10d} {true_err:11d} {is_optimal:>8}")
    print()


# ---------------------------------------------------------------------------
# (B) Signo direction vs signo real de la planta.
#     "NORMAL": plant(dac) = gain*dac (offset=0), direction=+1 coincide.
#     "INVERTIDA": plant(dac) = gain*255 - gain*dac (subir dac BAJA measured),
#     direction=+1 NO coincide -> realimentacion positiva -> dac satura, pero
#     calibration.c hace stage->write(best_dac) al salir por FAIL (linea 97),
#     asi que el VDAC queda en best_dac, no en el dac saturado.
# ---------------------------------------------------------------------------
def experiment_direction_sign():
    print("=" * 78)
    print("(B) direction=+1 (hardcodeado hoy) vs signo real de la planta")
    print("-" * 78)
    target = CAL_TARGET_1V_COUNTS
    gain_mag = 300  # counts/LSB, dentro de la zona de convergencia segun (A)

    for plant_sign, label in [(+1, "planta NORMAL (subir DAC -> sube ADC)"),
                               (-1, "planta INVERTIDA (subir DAC -> baja ADC)")]:
        if plant_sign > 0:
            offset = 0
            gain = gain_mag
        else:
            offset = gain_mag * 255
            gain = -gain_mag
        plant = make_linear_plant(gain, offset)
        r = calibrate_stage(target, +1, plant)  # direction=+1 fijo (codigo actual)
        status = "OK (converge)" if r.converged else "FAIL (best_dac fallback)"
        print(f"  {label}: plant(0)={plant(0)}, plant(255)={plant(255)}, "
              f"plant(128)={plant(128)} (initial_dac), target={target}")
        print(f"    -> {status}, iters={r.iterations}, final_dac_escrito={r.final_dac}, "
              f"best_dac={r.best_dac}, best_|err|={r.best_abs_error}")
        if not r.converged:
            sat_at = r.history[-1][1]  # ultimo dac_code usado dentro del loop (antes del write final de best_dac)
            print(f"    -> dac_code llego a saturar en {sat_at} durante la busqueda, "
                  f"pero stage->write(best_dac={r.best_dac}) al final deja el VDAC ahi "
                  f"(best_dac == initial_dac={CAL_DAC_INIT}? {r.best_dac == CAL_DAC_INIT})")
            print("    -> conclusion: signo invertido => 'fallida' (ok=0) PERO el VDAC "
                  "queda igual que antes de calibrar (no rompe nada), indistinguible "
                  "en el ACK de un 'busy' o de una etapa real con tol inalcanzable")
    print()


# ---------------------------------------------------------------------------
# (C) Target fuera de rango del DAC (planta no llega al target ni en 0 ni en 255)
# ---------------------------------------------------------------------------
def experiment_unreachable_target():
    print("=" * 78)
    print("(C) Target fuera del rango fisico [plant(0), plant(255)]")
    print("-" * 78)
    target = CAL_TARGET_1V5_COUNTS  # 78644
    gain = 100  # gain*255=25500 << target -> ni con dac=255 se alcanza
    plant = make_linear_plant(gain, 0)
    print(f"  plant(0)={plant(0)}, plant(255)={plant(255)}, target={target}")
    r = calibrate_stage(target, +1, plant)
    status = "OK" if r.converged else "FAIL (esperado: target inalcanzable)"
    print(f"  -> {status}, iters={r.iterations}, final_dac={r.final_dac}, "
          f"best_dac={r.best_dac}, best_|err|={r.best_abs_error}")
    print(f"  -> saturo en dac=255? {r.best_dac == 255} "
          "(esperado: si la planta es creciente y el target esta por encima "
          "del techo, el mejor DAC posible es 255)")
    print()


# ---------------------------------------------------------------------------
# (D) Tiempo total estimado para Geo (4 etapas) / Hammer (3 etapas) segun
#     ganancia supuesta (igual para todas las etapas, simplificacion).
#     T_iter = CAL_AVG_N * T_read + CAL_DAC_SETTLE_MS, ver HANDOFF_CALIBRATION.md #5
# ---------------------------------------------------------------------------
GEO_STAGES = [
    ("GEO_PGA",   CAL_TARGET_1V_COUNTS),
    ("GEO_BP",    CAL_TARGET_1V5_COUNTS),
    ("GEO_ADDER", CAL_TARGET_1V5_COUNTS),
    ("GEO_LP",    CAL_TARGET_1V5_COUNTS),
]
HAMMER_STAGES = [
    ("HAMMER_IN",  CAL_TARGET_1V_COUNTS),
    ("HAMMER_PGA", CAL_TARGET_1V5_COUNTS),
    ("HAMMER_LP",  CAL_TARGET_1V5_COUNTS),
]

CAL_DAC_SETTLE_MS = 5  # calibration_tables.h:19
CAL_AMUX_SETTLE_MS = 5  # calibration_tables.h:15, una vez por etapa


def t_iter_ms(t_read_ms: float) -> float:
    return CAL_AVG_N * t_read_ms + CAL_DAC_SETTLE_MS


def experiment_timing_table():
    print("=" * 78)
    print("(D) Tiempo total estimado psoc_calibration_run_blocking() vs gain supuesta")
    print("    (misma gain para todas las etapas, offset=0, dac arranca en 128)")
    print(f"    T_read_CFG2: optimista=1/2475s={1000/2475:.4f}ms, "
          f"pesimista=4x={4000/2475:.4f}ms (HANDOFF #5)")
    print(f"    PSOC_CAL_ACK_TIMEOUT_MS=15000 (slave/src/main.cpp:129)")
    print("-" * 78)

    for label, t_read in [("optimista", 1000 / 2475), ("pesimista", 4000 / 2475)]:
        t_iter = t_iter_ms(t_read)
        print(f"\n  -- T_read={t_read:.4f} ms/lectura ({label}), "
              f"T_iter={t_iter:.3f} ms --")
        print(f"  {'gain':>6} {'GEO iters':>10} {'GEO ms':>9} "
              f"{'HAMMER iters':>13} {'HAMMER ms':>10}")
        for gain in [50, 100, 200, 500, 1000]:
            geo_iters = 0
            geo_ms = 0.0
            for _name, target in GEO_STAGES:
                plant = make_linear_plant(gain, 0)
                r = calibrate_stage(target, +1, plant)
                geo_iters += r.iterations
                geo_ms += CAL_AMUX_SETTLE_MS + r.iterations * t_iter

            ham_iters = 0
            ham_ms = 0.0
            for _name, target in HAMMER_STAGES:
                plant = make_linear_plant(gain, 0)
                r = calibrate_stage(target, +1, plant)
                ham_iters += r.iterations
                ham_ms += CAL_AMUX_SETTLE_MS + r.iterations * t_iter

            geo_flag = " [!]>15s" if geo_ms > 15000 else ""
            ham_flag = " [!]>15s" if ham_ms > 15000 else ""
            print(f"  {gain:6d} {geo_iters:10d} {geo_ms:8.0f}{geo_flag:>7} "
                  f"{ham_iters:13d} {ham_ms:9.0f}{ham_flag:>7}")
    print()


# ---------------------------------------------------------------------------
# (E) Caso real con las 4 etapas Geo, ganancias DISTINTAS por etapa (mas
#     realista que (D)): cada etapa probablemente tiene ganancia counts/LSB
#     diferente (PGA vs BP vs Adder vs LP). Esto es solo ILUSTRATIVO -
#     reemplazar por valores medidos en banco.
# ---------------------------------------------------------------------------
def experiment_geo_realistic_example():
    print("=" * 78)
    print("(E) Ejemplo ilustrativo Geo con ganancias distintas por etapa")
    print("    (valores de gain INVENTADOS para mostrar el formato del reporte -")
    print("     reemplazar por mediciones reales counts/LSB de cada VDAC_ref_*)")
    print("-" * 78)
    assumed_gains = {
        "GEO_PGA": 80,
        "GEO_BP": 300,
        "GEO_ADDER": 600,   # > umbral 500 -> probablemente 'fallida' siempre
        "GEO_LP": 150,
    }
    for label, t_read in [("optimista", 1000 / 2475), ("pesimista", 4000 / 2475)]:
        t_iter = t_iter_ms(t_read)
        total_ms = 0.0
        rows = []
        for name, target in GEO_STAGES:
            gain = assumed_gains[name]
            plant = make_linear_plant(gain, 0)
            r = calibrate_stage(target, +1, plant)
            stage_ms = CAL_AMUX_SETTLE_MS + r.iterations * t_iter
            total_ms += stage_ms
            status = "OK" if r.converged else "FAIL->best_dac"
            rows.append((name, gain, status, r, stage_ms))

        print(f"  -- T_read {label} ({t_read:.4f} ms/lectura) --")
        for name, gain, status, r, stage_ms in rows:
            print(f"  {name:10s} gain={gain:4d} -> {status:14s} iters={r.iterations:2d} "
                  f"final_dac={r.final_dac:3d} best_dac={r.best_dac:3d} "
                  f"best_|err|={r.best_abs_error:6d}  ({stage_ms:6.1f} ms)")
        verdict = "OK, < 15000" if total_ms <= 15000 else "EXCEDE PSOC_CAL_ACK_TIMEOUT_MS=15000"
        print(f"  TOTAL ({label}): {total_ms:.0f} ms ({verdict})\n")
    print()


# ---------------------------------------------------------------------------
# (F) Clasificacion de los FAIL de (A) en dos regimenes, segun si el PID
#     encontro o no el optimo global (true_optimum, busqueda exhaustiva 0..255):
#       - RESOLUCION (optimo? = si): el PID YA esta en el mejor dac posible;
#         gain > 2*tol implica que NINGUN codigo entero cae dentro de tol.
#         Ningun retuneo de KP/KI/KD puede arreglar esto (ver (G)): hay que
#         subir CAL_TOL_COUNTS (> true_|err|) o cambiar la ganancia fisica HW.
#       - DINAMICA (optimo? = NO): el PID se quedo lejos del mejor dac posible
#         (overshoot/oscilacion con pasos de control grandes, gain cerca o por
#         encima del limite de estabilidad lineal gain*kp/4096<2 => gain<2048
#         para kp=4). Aca retunear KP_Q12 SI puede ayudar (ver (G)).
#     NOTA: corrige una atribucion previa de esta sesion que llamaba a
#     gain=800/1000/1200 "limit-cycle del PID" y sugeria retunear KP/KI -- los
#     datos de (A) muestran optimo?=si para esos tres, o sea son RESOLUCION
#     pura, no dinamica. Ver HANDOFF_CALIBRATION.md #11.A.
# ---------------------------------------------------------------------------
def experiment_fail_regime_classification():
    print("=" * 78)
    print("(F) Clasificacion de los FAIL de (A): RESOLUCION vs DINAMICA")
    print("-" * 78)
    target = CAL_TARGET_1V_COUNTS
    fail_gains = [1, 50, 100, 150, 200, 800, 1000, 1200, 2000, 2048, 2100, 3000, 5000, 10000]
    for gain in fail_gains:
        plant = make_linear_plant(gain, 0)
        r = calibrate_stage(target, +1, plant)
        if r.converged:
            continue
        true_dac, true_err = true_optimum(target, plant)
        is_optimal = (r.best_dac == true_dac)
        print(f"  gain={gain:5d}: best_dac={r.best_dac:3d} (err={r.best_abs_error:6d})  "
              f"true_dac={true_dac:3d} (err={true_err:6d})  optimo={'si' if is_optimal else 'NO'}")
        if is_optimal:
            print(f"    -> RESOLUCION: PID ya esta en el optimo global; "
                  f"gain>2*tol => ningun entero cae en tol")
            print(f"    -> remedio: subir CAL_TOL_COUNTS > {true_err} "
                  f"o cambiar ganancia HW de la etapa (NO sirve retunear KP/KI/KD)")
        else:
            print(f"    -> DINAMICA: PID NO llego al optimo global "
                  f"(se quedo a distancia |{r.best_abs_error}-{true_err}|="
                  f"{abs(r.best_abs_error - true_err)} counts del mejor punto)")
            if true_err <= CAL_TOL_COUNTS:
                print(f"    -> remedio: retunear KP_Q12 (true_|err|={true_err}<=tol "
                      f"=> SI convergeria si el PID llegara a dac={true_dac}) -- ver (G)")
            else:
                print(f"    -> remedio parcial: retunear KP_Q12 acerca a "
                      f"true_|err|={true_err}, pero eso aun >tol={CAL_TOL_COUNTS} "
                      f"=> tambien hace falta subir CAL_TOL_COUNTS o cambiar ganancia HW")
            tail = r.history[-4:]
            print(f"    -> ultimas 4 iters (it,dac,measured,error): {tail}")
    print()


# ---------------------------------------------------------------------------
# (G) Verificacion empirica de los remedios propuestos en (F):
#     - DINAMICA (gain=2100, true_dac=25, true_|err|=71<=tol=250): el limite de
#       estabilidad lineal es gain*kp/4096<2 => kp<2*4096/gain~=3.9, o sea
#       kp<=3 deberia converger para gain=2100 con kp=4 (FAIL).
#     - RESOLUCION (gain=1000, true_dac=52, true_|err|=429>tol=250): NINGUN kp
#       deberia converger (el optimo mismo viola tol); en cambio, subir
#       CAL_TOL_COUNTS por encima de 429 SI deberia converger sea cual sea kp.
# ---------------------------------------------------------------------------
def experiment_remedy_verification():
    print("=" * 78)
    print("(G) Verificacion de remedios: retunear KP_Q12 vs subir CAL_TOL_COUNTS")
    print("-" * 78)
    target = CAL_TARGET_1V_COUNTS

    gain = 2100
    plant = make_linear_plant(gain, 0)
    true_dac, true_err = true_optimum(target, plant)
    print(f"  -- Caso DINAMICA: gain={gain} (true_dac={true_dac}, "
          f"true_|err|={true_err}<=tol={CAL_TOL_COUNTS}) --")
    print(f"  {'kp':>4} {'resultado':>10} {'iters':>6} {'best_dac':>9} {'best_|err|':>11}")
    for kp in [1, 2, 3, 4]:
        r = calibrate_stage(target, +1, plant, kp=kp)
        status = "OK" if r.converged else "FAIL"
        print(f"  {kp:4d} {status:>10} {r.iterations:6d} {r.best_dac:9d} {r.best_abs_error:11d}")
    print(f"    -> estabilidad lineal: gain*kp/4096<2 => kp<{2*4096/gain:.2f} "
          f"(kp<=3 deberia ser estable para gain={gain})")

    gain = 1000
    plant = make_linear_plant(gain, 0)
    true_dac, true_err = true_optimum(target, plant)
    print(f"\n  -- Caso RESOLUCION: gain={gain} (true_dac={true_dac}, "
          f"true_|err|={true_err}>tol={CAL_TOL_COUNTS}) --")
    print(f"  {'kp':>4} {'resultado':>10} {'iters':>6} {'best_dac':>9} {'best_|err|':>11}")
    for kp in [1, 2, 3, 4, 8, 16]:
        r = calibrate_stage(target, +1, plant, kp=kp)
        status = "OK" if r.converged else "FAIL"
        print(f"  {kp:4d} {status:>10} {r.iterations:6d} {r.best_dac:9d} {r.best_abs_error:11d}")
    print(f"    -> prediccion: NINGUN kp converge (best_|err| no puede bajar de "
          f"true_|err|={true_err}>tol={CAL_TOL_COUNTS})")

    new_tol = true_err + 21  # 450 para gain=1000
    r = calibrate_stage(target, +1, plant, tol=new_tol)
    status = "OK" if r.converged else "FAIL"
    print(f"\n  -- Mismo caso RESOLUCION (gain={gain}), pero CAL_TOL_COUNTS={new_tol} "
          f"(>{true_err}), kp=4 (default) --")
    print(f"    -> {status}, iters={r.iterations}, best_dac={r.best_dac}, "
          f"best_|err|={r.best_abs_error}")
    print("    -> prediccion: OK (subir CAL_TOL_COUNTS arregla RESOLUCION sin tocar PID)")
    print()


# ---------------------------------------------------------------------------
# (H) Sensibilidad a ruido del ADC con CAL_AVG_N=32 lecturas promediadas por
#     iteracion (make_noisy_plant). Tres ganancias representativas:
#       - gain=300:  caso de (B) (direction sign), reachable y bien dentro de
#         tol (true_|err| chico).
#       - gain=1000: caso RESOLUCION de (A)/(G) (best_dac==true_dac, pero
#         true_|err|=429>tol=250 => FAIL incluso sin ruido).
#       - gain=2100: caso DINAMICA de (G) con kp=4 default (best_dac!=true_dac,
#         FAIL incluso sin ruido por inestabilidad lineal gain*kp/4096>2).
#     Para cada gain se corre primero el baseline determinista (sigma=0 via
#     make_linear_plant, igual a (A)/(F)/(G)), luego un sanity check de que
#     make_noisy_plant(sigma=0) reproduce ese baseline EXACTO (gauss(0,0)==0,
#     y avg_n*v // avg_n == v), y luego un barrido de sigma>0 con n_trials
#     semillas distintas por punto para ver dispersion de best_dac/iters.
#     RNG seedeado (reproducible): seed = (gain*100003) ^ (sigma_x10*1009) ^ trial.
# ---------------------------------------------------------------------------
def experiment_noise_sensitivity():
    print("=" * 78)
    print("(H) Sensibilidad a ruido del ADC (CAL_AVG_N=32 lecturas promediadas/iter)")
    print("-" * 78)
    target = CAL_TARGET_1V_COUNTS
    sigmas = [5, 15, 30, 60, 125, 250, 500, 1000]
    n_trials = 5

    for gain in (300, 1000, 2100):
        plant0 = make_linear_plant(gain, 0)
        true_dac, true_err = true_optimum(target, plant0)
        r0 = calibrate_stage(target, +1, plant0)
        status0 = "OK" if r0.converged else "FAIL"
        print(f"\n  gain={gain} (true_dac={true_dac}, true_|err|={true_err}, "
              f"tol={CAL_TOL_COUNTS})")
        print(f"  baseline sin ruido:        {status0:>4}, iters={r0.iterations:2d}, "
              f"best_dac={r0.best_dac:3d}, best_|err|={r0.best_abs_error:6d}")

        plant_sigma0 = make_noisy_plant(plant0, 0.0, rng=random.Random(0))
        r_sigma0 = calibrate_stage(target, +1, plant_sigma0)
        same = (r_sigma0.best_dac == r0.best_dac and
                r_sigma0.converged == r0.converged and
                r_sigma0.iterations == r0.iterations)
        print(f"  sigma=0 via make_noisy_plant (sanity check): "
              f"{'igual al baseline (OK)' if same else 'DIFERENTE -> BUG en wrapper'}")

        print(f"  {'sigma':>6} {'trial':>5} {'resultado':>9} {'iters':>6} "
              f"{'best_dac':>8} {'best_|err|':>10} {'dDac vs base':>12}")
        for sigma in sigmas:
            for trial in range(n_trials):
                seed = (gain * 100003) ^ (sigma * 10 * 1009) ^ trial
                rng = random.Random(seed)
                plant = make_noisy_plant(plant0, sigma, rng=rng)
                r = calibrate_stage(target, +1, plant)
                status = "OK" if r.converged else "FAIL"
                ddac = r.best_dac - r0.best_dac
                print(f"  {sigma:6d} {trial:5d} {status:>9} {r.iterations:6d} "
                      f"{r.best_dac:8d} {r.best_abs_error:10d} {ddac:12d}")
    print()


if __name__ == "__main__":
    experiment_gain_sweep()
    experiment_direction_sign()
    experiment_unreachable_target()
    experiment_timing_table()
    experiment_geo_realistic_example()
    experiment_fail_regime_classification()
    experiment_remedy_verification()
    experiment_noise_sensitivity()
