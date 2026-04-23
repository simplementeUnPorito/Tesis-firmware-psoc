"""gen_ricker.py — regenera ricker_table.h con los parametros deseados.
Uso: python gen_ricker.py [freq_hz] [n_points]
     python gen_ricker.py          → 1 Hz, 3000 puntos (default)

El wavelet se genera centrado en la ventana. Los extremos se tapean con
una rampa coseno de taper_samples muestras para que el array empiece y
termine exactamente en 128 (offset), eliminando la discontinuidad al
volver al reposo entre ciclos.
"""
import sys
import numpy as np

def gen(f=1.0, N=3000, fs=3000.0, taper_samples=150, out="ricker_table.h"):
    t = (np.arange(N) - N // 2) / fs
    u = (np.pi * f * t) ** 2
    w = (1.0 - 2.0 * u) * np.exp(-u)

    peak = np.max(np.abs(w))
    if peak > 0:
        w /= peak

    # Taper coseno: fade in los primeros taper_samples, fade out los ultimos
    tap = 0.5 * (1.0 - np.cos(np.pi * np.arange(taper_samples) / taper_samples))
    w[:taper_samples]  *= tap
    w[-taper_samples:] *= tap[::-1]

    table = np.clip(np.round(128.0 + w * 127.0), 0, 255).astype(np.uint8)

    # Verificar que primero y ultimo punto son 128
    assert table[0] == 128 and table[-1] == 128, \
        f"Extremos no son 128: [{table[0]}, {table[-1]}]"

    lines = [
        "#ifndef RICKER_TABLE_H",
        "#define RICKER_TABLE_H",
        "#include \"cytypes.h\"",
        f"/* Ricker wavelet: f_peak={f} Hz, {N} pts @ {int(fs)} Hz, taper={taper_samples} */",
        f"static const uint8 ricker_table[{N}] = {{",
    ]
    row = []
    for i, v in enumerate(table):
        row.append(f"{v:3d}")
        if len(row) == 20:
            lines.append("    " + ",".join(row) + ",")
            row = []
    if row:
        lines.append("    " + ",".join(row))
    lines += ["};", "#endif /* RICKER_TABLE_H */", ""]

    with open(out, "w") as fh:
        fh.write("\n".join(lines))

    nonflat = np.sum(np.abs(w) > 0.001)
    print(f"Escrito {out}: {N} pts, f_peak={f} Hz, taper={taper_samples} muestras")
    print(f"  Rango tabla: [{table.min()}, {table.max()}]  Extremos: [{table[0]}, {table[-1]}]")
    print(f"  Muestras activas (|w|>0.001): {nonflat}/{N}")

if __name__ == "__main__":
    f    = float(sys.argv[1]) if len(sys.argv) > 1 else 1.0
    N    = int(sys.argv[2])   if len(sys.argv) > 2 else 3000
    taper = int(sys.argv[3])  if len(sys.argv) > 3 else 150
    gen(f, N, taper_samples=taper)
