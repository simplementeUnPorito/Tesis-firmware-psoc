"""Verificacion offline de la matematica del autotest (bloque de calibration.c).

Porta a Python st_isqrt64, st_sincos y el lazo de Goertzel de
psoc_selftest_measure_series, y los contrasta contra el math de Python.

Por que existe: esas funciones estan escritas a mano porque no se puede
enlazar libm sin editar el .cyprj, y producen los numeros con los que el
autotest decide PASS/FAIL. Un error ahi no se ve como un bug: se ve como una
placa mala.

Ya encontro dos defectos reales:

  1. El Goertzel NO rechazaba la continua. Con N = 2048 el indice del bin queda
     k = 39,32 a 50 Hz, el bin no cae en su centro y la fuga de un offset de
     50000 counts se medía como 61 % de error. Corregido usando N = fs (bin
     exacto, k = f0 para cualquier frecuencia entera) y restando ademas una
     estimacion de continua.
  2. st_sincos tenia los cuadrantes 1 y 3 INTERCAMBIADOS en la rama del
     complemento: devolvia seno y coseno con los dos signos dados vuelta. No
     afectaba la medicion de 50 Hz (cae en el cuadrante 0 por la otra rama),
     pero corrompia la magnitud para tonos por encima de ~325 Hz.

Correr:  python test_selftest_math.py     (no necesita hardware ni pyserial)

Si se toca alguna de esas funciones en calibration.c, actualizar la
transcripcion de aca y volver a correrlo.
"""
import math

U64 = (1 << 64) - 1

# ---------------------------------------------------------------- st_isqrt64
def st_isqrt64(x, trace=False):
    """Transcripcion literal del C."""
    if x == 0:
        return 0
    r = x
    prev = 0
    # semilla: 2^ceil(bits/2)
    bits = 0
    t = x
    while t != 0:
        t >>= 1
        bits += 1
    r = 1 << ((bits + 1) // 2)

    guard = 0
    while r != prev:
        guard += 1
        if guard > 500:
            raise RuntimeError("st_isqrt64 NO CONVERGE para x=%d" % x)
        prev = r
        r = (r + x // r) // 2
        if r == 0:
            return 0
        if prev > r and (prev - r) == 1:
            break

    guard = 0
    while r * r > x:
        r -= 1
        guard += 1
        if guard > 500:
            raise RuntimeError("ajuste final no termina para x=%d" % x)
    return r


def test_isqrt():
    fallos = []
    # casos chicos exhaustivos
    for x in range(0, 20000):
        got = st_isqrt64(x)
        want = math.isqrt(x)
        if got != want:
            fallos.append((x, got, want))
            if len(fallos) > 5:
                break

    # potencias de dos y sus vecinos, hasta el tope de uint64
    casos = []
    for b in range(0, 64):
        for d in (-1, 0, 1):
            v = (1 << b) + d
            if 0 <= v <= U64:
                casos.append(v)
    # cuadrados exactos grandes y sus vecinos
    for b in range(1, 32):
        n = 1 << b
        for d in (-1, 0, 1):
            v = n * n + d
            if 0 <= v <= U64:
                casos.append(v)
    casos += [U64, U64 - 1, 3, 8, 15, 24, 35, 48, 63, 80, 99]

    for x in casos:
        got = st_isqrt64(x)
        want = math.isqrt(x)
        if got != want:
            fallos.append((x, got, want))

    if fallos:
        print("[FAIL] st_isqrt64: %d discrepancias" % len(fallos))
        for x, got, want in fallos[:8]:
            print("        x=%d  got=%d  want=%d" % (x, got, want))
        return False
    print("[PASS] st_isqrt64: exacta en 0..19999, potencias de 2 +-1, "
          "cuadrados exactos +-1 y el tope de uint64")
    return True


# ---------------------------------------------------------------- st_sincos
PI = 3.14159265358979323846
TWO_PI = 6.28318530717958647692
HALF_PI = 1.57079632679489661923


def st_cos_core(x):
    x2 = x * x
    return 1.0 - x2 * (1.0 / 2.0
        - x2 * (1.0 / 24.0
        - x2 * (1.0 / 720.0
        - x2 * (1.0 / 40320.0
        - x2 * (1.0 / 3628800.0)))))


def st_sin_core(x):
    x2 = x * x
    return x * (1.0 - x2 * (1.0 / 6.0
        - x2 * (1.0 / 120.0
        - x2 * (1.0 / 5040.0
        - x2 * (1.0 / 362880.0)))))


def st_sincos(x):
    while x < 0.0:
        x += TWO_PI
    while x >= TWO_PI:
        x -= TWO_PI

    quad = int(x / HALF_PI)          # 0..3
    r = x - quad * HALF_PI

    if r > (HALF_PI / 2.0):
        c = HALF_PI - r
        sc = st_sin_core(c)
        cc = st_cos_core(c)
        if quad == 0:   return (cc, sc)
        elif quad == 1: return (sc, -cc)
        elif quad == 2: return (-cc, -sc)
        else:           return (-sc, cc)
    else:
        sr = st_sin_core(r)
        cr = st_cos_core(r)
        if quad == 0:   return (sr, cr)
        elif quad == 1: return (cr, -sr)
        elif quad == 2: return (-sr, -cr)
        else:           return (-cr, sr)


def test_sincos():
    peor_s = 0.0
    peor_c = 0.0
    peor_x = None
    fallos = []

    # barrido fino en dos vueltas completas, mas valores negativos
    n = 20000
    for i in range(n):
        x = -TWO_PI + (4.0 * TWO_PI) * i / n
        s, c = st_sincos(x)
        es = abs(s - math.sin(x))
        ec = abs(c - math.cos(x))
        if es > peor_s:
            peor_s, peor_x = es, x
        if ec > peor_c:
            peor_c = ec
        if es > 1e-7 or ec > 1e-7:
            fallos.append((x, s, math.sin(x), c, math.cos(x)))

    # puntos criticos exactos
    criticos = [0.0, HALF_PI, PI, 3 * HALF_PI, TWO_PI,
                HALF_PI / 2, HALF_PI / 2 + 1e-9, HALF_PI / 2 - 1e-9,
                -0.1, -HALF_PI, TWO_PI - 1e-9]
    for x in criticos:
        s, c = st_sincos(x)
        if abs(s - math.sin(x)) > 1e-7 or abs(c - math.cos(x)) > 1e-7:
            fallos.append((x, s, math.sin(x), c, math.cos(x)))

    # el caso que realmente usa el autotest: w = 2*pi*50/2604
    w = TWO_PI * 50.0 / 2604.0
    s, c = st_sincos(w)
    err = max(abs(s - math.sin(w)), abs(c - math.cos(w)))

    if fallos:
        print("[FAIL] st_sincos: %d puntos con error > 1e-7" % len(fallos))
        for f in fallos[:6]:
            print("        x=%+.6f  sin %+.12f vs %+.12f   cos %+.12f vs %+.12f" % f)
        return False
    print("[PASS] st_sincos: error maximo sin=%.2e cos=%.2e en [-2pi, 6pi]" % (peor_s, peor_c))
    print("       w del Goertzel (50 Hz / 2604 Hz) = %.6f rad, error %.2e" % (w, err))
    return True


# --------------------------------------------------- Goertzel de punta a punta
def goertzel_como_el_firmware(muestras, tone_hz, fs, dc_pre=64):
    """Replica exacta del lazo de psoc_selftest_measure_series."""
    w = TWO_PI * tone_hz / fs
    sw, cw = st_sincos(w)
    coeff = 2.0 * cw
    s1 = s2 = 0.0
    dc = sum(muestras[:dc_pre]) / float(dc_pre) if dc_pre else 0.0
    for x in muestras[dc_pre:] if dc_pre else muestras:
        s0 = coeff * s1 - s2 + (float(x) - dc)
        s2 = s1
        s1 = s0
    re = s1 - s2 * cw
    im = s2 * sw
    p = re * re + im * im
    if p < 0.0:
        p = 0.0
    n_used = len(muestras) - (dc_pre if dc_pre else 0)
    return 2.0 * st_isqrt64(int(p)) / n_used


def test_goertzel():
    fs = 2604.0
    n = 2604   # = fs: el bin cae exacto (k = f0)
    fallos = []
    print()
    print("  amplitud inyectada -> amplitud medida (deberian coincidir):")
    for amp in (100.0, 1000.0, 25000.0):
        for f0 in (50.0,):
            xs = [amp * math.sin(TWO_PI * f0 * k / fs) for k in range(n)]
            got = goertzel_como_el_firmware(xs, int(f0), fs)
            err_rel = abs(got - amp) / amp
            marca = "ok " if err_rel < 0.05 else "MAL"
            print("    %s  %8.0f Hz=%g -> %10.1f   (error %.2f %%)"
                  % (marca, amp, f0, got, err_rel * 100))
            if err_rel >= 0.05:
                fallos.append((amp, f0, got))

    # con offset de continua grande: el Goertzel a 50 Hz tiene que rechazarlo
    amp = 1000.0
    xs = [50000.0 + amp * math.sin(TWO_PI * 50.0 * k / fs) for k in range(n)]
    got = goertzel_como_el_firmware(xs, 50, fs)
    err_rel = abs(got - amp) / amp
    marca = "ok " if err_rel < 0.10 else "MAL"
    print("    %s  con offset DC de 50000: %10.1f  (error %.2f %%)" % (marca, got, err_rel * 100))
    if err_rel >= 0.10:
        fallos.append(("dc", 50.0, got))

    # sin tono: tiene que dar chico
    xs = [0.0] * n
    got = goertzel_como_el_firmware(xs, 50, fs)
    print("    %s  sin senal: %.3f (deberia ser ~0)" % ("ok " if got < 1.0 else "MAL", got))
    if got >= 1.0:
        fallos.append(("cero", 50.0, got))

    if fallos:
        print("[FAIL] Goertzel: %d casos mal" % len(fallos))
        return False
    print("[PASS] Goertzel: amplitud correcta, rechaza la continua, cero sin senal")
    return True


if __name__ == "__main__":
    print("=== Verificacion offline de la matematica del autotest ===\n")
    r1 = test_isqrt()
    r2 = test_sincos()
    r3 = test_goertzel()
    print()
    print("RESULTADO:", "TODO OK" if (r1 and r2 and r3) else "HAY FALLOS")
    raise SystemExit(0 if (r1 and r2 and r3) else 1)
