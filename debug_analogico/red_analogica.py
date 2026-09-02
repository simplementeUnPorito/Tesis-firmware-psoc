#!/usr/bin/env python3
"""Genera la tabla de resistencias esperadas entre pines del PSoC.

Toda la red vive en este archivo. Si un valor cambia en la implementacion,
se corrige aca y se regenera el CSV; no hay que editar 153 filas a mano.

Por defecto modela la topologia del TopDesign, que es la que esta construida:
las cuatro ramas de referencia son de 15k y van todas al mismo nodo Vref
(P3[6]), sin regulador externo.

    python red_analogica.py                 -> outputs/debug_analogico/medidas_analogicas.csv
    python red_analogica.py --portadora     -> variante de la placa JitX
    python red_analogica.py -o otro.csv

Fuentes: TopDesign.cysch del proyecto AcondicionamientoAnalogico,
cyfitter.h del build vigente y el netlist/BOM de PCBs/JitX
(designs/tesis_carrier.main.TesisCarrier).
"""

import argparse
import csv
import itertools
import sys
from pathlib import Path


def _cargar_dir_salida():
    """Importa el helper compartido aun al ejecutar desde el submodulo."""
    inicio = Path(__file__).resolve().parent
    for carpeta in (inicio, *inicio.parents):
        compartidos = carpeta / "scripts" / "shared"
        if (compartidos / "rutas.py").is_file():
            sys.path.insert(0, str(compartidos))
            from rutas import dir_salida
            return dir_salida
    raise RuntimeError("no se encontro scripts/shared/rutas.py")


dir_salida = _cargar_dir_salida()

# ---------------------------------------------------------------------------
# Pines fisicos del PSoC que participan de la parte analogica.
# El orden es el de la tabla de salida.
# ---------------------------------------------------------------------------
PINES = [
    ("P1[2]", "INp"),
    ("P1[7]", "INn"),
    ("P3[6]", "Vref"),
    ("P2[7]", "SEo"),
    ("P3[2]", "BPm"),
    ("P3[7]", "BPo"),
    ("P0[5]", "SUMm"),
    ("P0[0]", "SUMo"),
    ("P2[6]", "PGAo"),
    ("P0[3]", "LPm"),
    ("P0[1]", "LPo"),
    ("P3[0]", "Vref_PGA"),
    ("P3[1]", "Vref_BP"),
    ("P0[6]", "Vref_ADDER"),
    ("P0[7]", "Vref_LP"),
    ("P15[3]", "AMuxCapacitor"),
    ("-", "GND"),
    ("-", "+5V"),
]

# Cada pin cuelga de un nodo electrico de la red.
PIN_A_NODO = {
    "INp": "IN_P",
    "INn": "IN_N",
    "Vref": "VREF",
    "SEo": "SEo",
    "BPm": "BPm",
    "BPo": "BPo",
    "SUMm": "SUMm",
    "SUMo": "SUMo",
    "PGAo": "PGAo",
    "LPm": "LPm",
    "LPo": "LPo",
    "Vref_PGA": "PGA_REF",
    "Vref_BP": "BP_REF",
    "Vref_ADDER": "SUM_REF",
    "Vref_LP": "LP_REF",
    "AMuxCapacitor": "AMUX_CAP",
    "GND": "GND",
    "+5V": "V5",
}

# ---------------------------------------------------------------------------
# Valores. EDITAR ACA si la implementacion difiere.
# ---------------------------------------------------------------------------
VALORES = {
    "R2": 50e3,     # INp   -> Vref
    "R3": 50e3,     # Vref  -> INn
    "R4": 43e3,     # SEo   -> C1 (entrada del pasabanda)
    "R5": 47e3,     # BPm   -> BPo (realimentacion del pasabanda)
    "R6": 6.8e3,    # SEo   -> SUMm
    "R7": 6.8e3,    # BPo   -> RV1
    "R8": 27e3,     # SUMm  -> SUMo (realimentacion del sumador)
    "R9": 12e3,     # LP_A  -> LPm
    "R10": 150e3,   # LP_A  -> LPo
    "R15": 30e3,    # PGAo  -> LP_A
    "R11": 15e3,    # Vref_PGA    -> Vref
    "R12": 15e3,    # Vref_BP     -> Vref
    "R13": 15e3,    # Vref_ADDER  -> Vref
    "R14": 15e3,    # Vref_LP     -> Vref
}

# Trimmer del sumador: en la portadora el cursor y un extremo estan unidos,
# asi que lo que se mide es la fraccion del recorrido, no el valor nominal.
RV1_TOTAL = 2000.0
RV1_FRACCION = 0.317   # posicion guardada en el TopDesign; medirla en la placa

# ---------------------------------------------------------------------------
# Topologia. (referencia, nodo, nodo)
# ---------------------------------------------------------------------------
RESISTORES = [
    ("R2", "IN_P", "VREF"),
    ("R3", "VREF", "IN_N"),
    ("R4", "SEo", "N_C1"),
    ("R5", "BPm", "BPo"),
    ("R6", "SEo", "SUMm"),
    ("R7", "BPo", "N_RV1"),
    ("RV1", "N_RV1", "SUMm"),
    ("R8", "SUMm", "SUMo"),
    ("R15", "PGAo", "LP_A"),
    ("R9", "LP_A", "LPm"),
    ("R10", "LP_A", "LPo"),
    ("R11", "PGA_REF", "VREF"),
    ("R12", "BP_REF", "VREF"),
    ("R13", "SUM_REF", "VREF"),
    ("R14", "LP_REF", "VREF"),
]

# Capacitores: abiertos en continua. Solo se listan los que importan para
# interpretar la lectura; C1 ademas se usa para el transitorio.
CONDENSADORES = [
    ("C1", "N_C1", "BPm", "680uF"),
    ("C2", "BPm", "BPo", "27pF"),
    ("C17", "BPm", "BPo", "150pF"),
    ("C3", "SUMm", "SUMo", "15nF"),
    ("C4", "LP_A", "VREF", "47nF"),
    ("C5", "LPm", "LPo", "3.3nF"),
    ("C6", "VREF", "GND", "1uF"),
    ("C7", "VREF", "GND", "100nF"),
    ("C8", "PGA_REF", "GND", "1uF"),
    ("C9", "PGA_REF", "GND", "100nF"),
    ("C10", "BP_REF", "GND", "1uF"),
    ("C11", "BP_REF", "GND", "100nF"),
    ("C12", "SUM_REF", "GND", "1uF"),
    ("C13", "SUM_REF", "GND", "100nF"),
    ("C14", "LP_REF", "GND", "1uF"),
    ("C15", "LP_REF", "GND", "100nF"),
    ("C16", "AMUX_CAP", "GND", "100nF"),
]

# ---------------------------------------------------------------------------
# Variante de la portadora JitX (todavia no fabricada): las cuatro ramas son
# de 30k y no van a Vref sino al rail VREF_2V048, salida de un AMS1117-ADJ
# con divisor R22/R23. Eso deja Vref_XX abierto contra P3[6] y con un camino
# de 30k + 1k + 620 contra masa.
# ---------------------------------------------------------------------------
def aplicar_portadora():
    for ref in ("R11", "R12", "R13", "R14"):
        VALORES[ref] = 30e3
    VALORES["R22"] = 1.0e3
    VALORES["R23"] = 620.0
    for i, (ref, a, b) in enumerate(RESISTORES):
        if ref in ("R11", "R12", "R13", "R14"):
            RESISTORES[i] = (ref, a, "VREF_2V048")
    RESISTORES.append(("R22", "VREF_2V048", "ADJ"))
    RESISTORES.append(("R23", "ADJ", "GND"))
    CONDENSADORES.append(("C18", "VREF_2V048", "GND", "22uF"))
    CONDENSADORES.append(("C19", "V5", "GND", "10uF"))


# ---------------------------------------------------------------------------
# Resolucion de la red
# ---------------------------------------------------------------------------
def valor(ref):
    if ref == "RV1":
        return RV1_TOTAL * RV1_FRACCION
    return VALORES[ref]


def construir(cortocircuitos=()):
    """Devuelve (nodos, aristas) con los pares de `cortocircuitos` fusionados."""
    alias = {}

    def raiz(n):
        while alias.get(n, n) != n:
            n = alias[n]
        return n

    for a, b in cortocircuitos:
        ra, rb = raiz(a), raiz(b)
        if ra != rb:
            alias[ra] = rb

    aristas = [(ref, raiz(a), raiz(b)) for ref, a, b in RESISTORES]
    aristas = [e for e in aristas if e[1] != e[2]]
    nodos = sorted({n for _, a, b in aristas for n in (a, b)})
    return raiz, nodos, aristas


def componente(aristas, a):
    """Nodos alcanzables desde a por resistores."""
    ady = {}
    for _, u, v in aristas:
        ady.setdefault(u, set()).add(v)
        ady.setdefault(v, set()).add(u)
    visto = {a}
    pila = [a]
    while pila:
        for sig in ady.get(pila.pop(), ()):
            if sig not in visto:
                visto.add(sig)
                pila.append(sig)
    return visto


def resistencia(nodos, aristas, a, b):
    """Resistencia efectiva entre a y b. None si no hay camino resistivo."""
    if a == b:
        return 0.0
    if a not in nodos or b not in nodos:
        return None

    comp = componente(aristas, a)
    if b not in comp:
        return None

    import numpy as np

    locales = sorted(comp)
    idx = {n: i for i, n in enumerate(locales)}
    n = len(locales)
    L = np.zeros((n, n))
    for ref, u, v in aristas:
        if u not in idx:
            continue
        g = 1.0 / valor(ref)
        iu, iv = idx[u], idx[v]
        L[iu, iu] += g
        L[iv, iv] += g
        L[iu, iv] -= g
        L[iv, iu] -= g

    # Inyectar 1 A en a, extraer en b, referencia de tension en b.
    keep = [i for i in range(n) if i != idx[b]]
    Lr = L[np.ix_(keep, keep)]
    corriente = np.zeros(len(keep))
    corriente[keep.index(idx[a])] = 1.0
    v = np.linalg.solve(Lr, corriente)
    return float(v[keep.index(idx[a])])


def camino(aristas, a, b):
    """Camino de resistores entre a y b (unico mientras la red sea un arbol)."""
    ady = {}
    for ref, u, v in aristas:
        ady.setdefault(u, []).append((v, ref))
        ady.setdefault(v, []).append((u, ref))
    pila = [(a, [])]
    visto = {a}
    while pila:
        nodo, ruta = pila.pop()
        if nodo == b:
            return ruta
        for sig, ref in ady.get(nodo, []):
            if sig not in visto:
                visto.add(sig)
                pila.append((sig, ruta + [ref]))
    return []


def etiqueta(pin, senal):
    """'P3[6] Vref' para los pines del PSoC, 'GND' para el resto."""
    return senal if pin == "-" else "%s %s" % (pin, senal)


# ---------------------------------------------------------------------------
# Dibujo de la red
# ---------------------------------------------------------------------------
def vr(ref):
    """Valor de un resistor tal como se rotula en el esquematico."""
    if ref == "RV1":
        return "%s@%.1f%%" % (compacto(RV1_TOTAL), RV1_FRACCION * 100)
    return compacto(VALORES[ref])


def compacto(ohm):
    if ohm >= 1e3:
        s = "%.1fk" % (ohm / 1e3)
        return s.replace(".0k", "k")
    return "%.0f" % ohm


def vc(ref):
    """Valor de un capacitor."""
    for r, _, _, v in CONDENSADORES:
        if r == ref:
            return v
    raise KeyError(ref)


def nodo_de(ref):
    """Nodos de un resistor, como estan cargados en RESISTORES."""
    for r, a, b in RESISTORES:
        if r == ref:
            return a, b
    raise KeyError(ref)


DIAGRAMA = """
ENTRADA

   Vdda/2 --------|+\\
                  |  >------+-- Vref  P3[6] --+-- {R2} -- INp  P1[2] --+
               +--|-/       |                 |                               | J4 geofono
               |  |/        |                 +-- {R3} -- INn  P1[7] --+
               +------------+                 +-- {C6} -- Vss
                    OPAref                    |
                                              +-- {C7} -- Vss

   J4 queda en paralelo con R2 + R3: desconectarlo para medir.


PASA-BANDA

   SEo  P2[7] -- {R4} -- {NC1} -- {C1} --+-- BPm  P3[2] --|-\\
                                                      |                |  >---+-- BPo  P3[7]
                                                      |    BPm_ref ----|+/    |
                                                      |                |/     |
                                                      |              OPAbp    |
                                                      |                       |
                                                      +-- {R5} --------+
                                                      +-- {C2} --------+
                                                      +-- {C17} --------+


SUMADOR

   SEo  P2[7] -- {R6} ----------------------+-- SUMm  P0[5] --|-\\
                                                   |                 |  >----------+-- SUMo  P0[0]
   BPo  P3[7] -- {R7} -- {RV1} ----+                               |
                                                   |  SUMm_ref ------|+/           |
                                                   |                 |/            |
                                                   |               OPAsum          |
                                                   |                               |
                                                   +-- {R8} ----------------+
                                                   +-- {C3} ----------------+


PASA-BAJOS

   PGAo P2[6] -- {R15} --+-- LP_A -- {R9} --+-- LPm  P0[3] --|-\\
                               |                         |                |  >------+-- LPo  P0[1]
                               |                         |  LPm_ref ------|+/       |
                               |                         |                |/        |
                               |                         |              OPAlp       |
                               |                         |                          |
                               |                         +-- {C5} -----------+
                               +-- {R10} -------------------------------------+
                               |
                               +-- {C4} -- Vref  P3[6]


REFERENCIAS POR ETAPA   (una rama identica por cada IDAC8)

   IDAC8 --> Vref_PGA    P3[0] -- {R11} --+
   IDAC8 --> Vref_BP     P3[1] -- {R12} --+
                                                +-- {NODO_REF}
   IDAC8 --> Vref_ADDER  P0[6] -- {R13} --+
   IDAC8 --> Vref_LP     P0[7] -- {R14} --+
{EXTRA_REF}
   De cada pin Vref_xx cuelga:  +-- |1uF|   -- Vss
                                +-- |100nF| -- Vss
   Ese par de capacitores es el unico camino a masa: contra GND dan abierto.


AMUX

   AMuxCapacitor  P15[3] -- {C16} -- Vss
"""

EXTRA_REF_PORTADORA = """
   +5V ------| AMS1117-ADJ |--+-- VREF_2V048   (salida del regulador; NO es Vref)
                              |
                              +-- {C18} -- Vss
                              |
                              +-- {R22} -- ADJ -- {R23} -- Vss
"""


def dibujar():
    """Imprime la red tal como la tiene cargada el script."""
    def token_resistor(ref):
        ancho = 8 if ref == "RV1" else 5
        return "[%-3s %*s]" % (ref, ancho, vr(ref))

    def token_capacitor(ref):
        return "|%-3s %5s|" % (ref, vc(ref))

    nodo_ref = nodo_de("R11")[1]
    campos = {
        "NC1": nodo_de("R4")[1],
        "NODO_REF": ("Vref  P3[6]" if nodo_ref == "VREF" else nodo_ref),
    }
    for ref in VALORES:
        campos[ref] = token_resistor(ref)
    campos["RV1"] = token_resistor("RV1")
    for ref, _, _, _ in CONDENSADORES:
        campos[ref] = token_capacitor(ref)
    campos["EXTRA_REF"] = ("" if nodo_ref == "VREF"
                           else EXTRA_REF_PORTADORA.format(**campos))

    texto = DIAGRAMA.format(**campos)

    # El dibujo esta escrito a mano; esto impide que se desincronice del
    # modelo sin que nadie se entere.
    faltan = [r for r, _, _ in RESISTORES if ("[%s " % r) not in texto]
    faltan += [c for c, _, _, _ in CONDENSADORES
               if ("|%s " % c) not in texto and c not in ("C8", "C9", "C10",
                                                          "C11", "C12", "C13",
                                                          "C14", "C15", "C18",
                                                          "C19")]
    if faltan:
        texto += ("\n*** El dibujo no cubre: %s. Actualizar DIAGRAMA. ***\n"
                  % ", ".join(faltan))

    print(texto)
    print("NODOS Y PINES")
    for pin, senal in PINES:
        print("   %-8s %-14s nodo %s" % (pin, senal, PIN_A_NODO[senal]))
    print("\nCOMPONENTES CARGADOS")
    for ref, a, b in RESISTORES:
        print("   %-4s %9s  %s - %s" % (ref, vr(ref), a, b))
    for ref, a, b, v in CONDENSADORES:
        print("   %-4s %9s  %s - %s" % (ref, v, a, b))


def fmt(r):
    if r is None:
        return "abierto"
    if r < 1000:
        return "%.0f" % r
    if r < 1e6:
        return "%.2f k" % (r / 1e3)
    return "%.2f M" % (r / 1e6)


# ---------------------------------------------------------------------------
# Los doce chequeos que planteo Elias a mano. Se marcan con * en --tabla
# para poder hacer solo esos e ignorar el resto.
CHEQUEOS_BASE = {
    ("INp", "INn"), ("INp", "Vref"), ("INn", "Vref"),
    ("Vref_PGA", "Vref"), ("Vref_BP", "Vref"),
    ("Vref_ADDER", "Vref"), ("Vref_LP", "Vref"),
    ("SEo", "SUMm"), ("SEo", "BPo"),
    ("BPm", "BPo"), ("BPm", "SUMm"), ("BPm", "SEo"),
    ("SUMm", "SUMo"),
    ("PGAo", "LPm"), ("PGAo", "LPo"), ("LPm", "LPo"),
}


def es_base(a, b):
    return (a, b) in CHEQUEOS_BASE or (b, a) in CHEQUEOS_BASE


NOTAS_NODO = {
    "IN_P": "el geofono va en paralelo: medir con J4 desconectado",
    "IN_N": "el geofono va en paralelo: medir con J4 desconectado",
    "VREF_2V048": "solo en la portadora: rail del AMS1117, el regulador apagado agrega un camino propio",
    "GND": "",
    "V5": "",
}


# Columnas que completa el usuario a mano. Se preservan al regenerar el CSV.
ANOTABLES = ["medido_inicial", "espera_s", "medido", "ok",
              "esperado_corregido", "notas_medicion"]

TOL_R = 0.01    # resistencias al 1 %
TOL_C = 0.20    # capacitores al 20 %

_ESCALA_C = {"pF": 1e-12, "nF": 1e-9, "uF": 1e-6}


def faradios(texto):
    """'680uF' -> 6.8e-4"""
    for suf, esc in _ESCALA_C.items():
        if texto.endswith(suf):
            return float(texto[:-len(suf)]) * esc
    raise ValueError(texto)


def fmt_c(f):
    if f >= 1e-6:
        return "%.2fuF" % (f / 1e-6)
    if f >= 1e-9:
        return "%.1fnF" % (f / 1e-9)
    return "%.0fpF" % (f / 1e-12)


def fmt_k(r):
    if r >= 100e3:
        return "%.1fk" % (r / 1e3)
    if r >= 1e3:
        return "%.2fk" % (r / 1e3)
    return "%.0f" % r


def rango(valor, tol, fmt_fn):
    return "%s-%s" % (fmt_fn(valor * (1 - tol)), fmt_fn(valor * (1 + tol)))


def pares_capacitivos():
    """Pares de pines unidos DIRECTAMENTE por uno o mas capacitores.

    Solo esos dan una lectura util con el capacimetro. Los capacitores que
    tienen un nodo interno (C1 y C4) no aparecen: no hay dos pines entre los
    cuales medirlos.
    """
    por_par = {}
    for ref, a, b, v in CONDENSADORES:
        por_par.setdefault(frozenset((a, b)), []).append((ref, v))

    _, nodos, aristas = construir()
    filas = []
    for (pa, sa), (pb, sb) in itertools.combinations(PINES, 2):
        na, nb = PIN_A_NODO[sa], PIN_A_NODO[sb]
        grupo = por_par.get(frozenset((na, nb)))
        if not grupo:
            continue
        total = sum(faradios(v) for _, v in grupo)
        shunt = resistencia(nodos, aristas, na, nb)
        filas.append({
            "tipo": "C",
            "par": "%s - %s" % (etiqueta(pa, sa), etiqueta(pb, sb)),
            "pin_a": pa, "senal_a": sa, "pin_b": pb, "senal_b": sb,
            "componentes": " + ".join(r for r, _ in grupo),
            "valores": [v for _, v in grupo],
            "esperado": fmt_c(total),
            "rango": rango(total, TOL_C, fmt_c),
            "shunt": shunt,
        })
    return filas


def leer_anotaciones(ruta):
    """Rescata lo que el usuario ya anoto a mano, para no pisarlo.

    El CSV es para completar durante la medicion. Regenerarlo despues de
    cambiar un valor NO debe borrar las columnas de ANOTABLES.
    """
    try:
        with open(ruta, newline="", encoding="utf-8-sig") as fh:
            previo = {}
            for fila in csv.DictReader(fh):
                guardado = {c: (fila.get(c) or "") for c in ANOTABLES}
                # Compatibilidad con la version anterior de la columna.
                if not guardado["medido"] and fila.get("medido_ohm"):
                    guardado["medido"] = fila["medido_ohm"]
                if any(guardado.values()):
                    # Un mismo par puede aparecer como fila de resistencia y
                    # de capacitancia; la anotacion no debe cruzarse.
                    clave = (fila.get("tipo", "R"), fila.get("par", ""))
                    previo[clave] = guardado
            return previo
    except FileNotFoundError:
        return {}


def calcular():
    """Una fila por cada par de pines, con su resistencia esperada."""
    raiz, nodos, aristas = construir()
    # Transitorio: el electrolitico de 680 uF arranca como un corto.
    raiz_t, nodos_t, aristas_t = construir(cortocircuitos=[("N_C1", "BPm")])

    filas = []
    for (pa, sa), (pb, sb) in itertools.combinations(PINES, 2):
        na, nb = PIN_A_NODO[sa], PIN_A_NODO[sb]
        r = resistencia(nodos, aristas, na, nb)
        ruta = camino(aristas, na, nb) if r is not None else []
        rt = resistencia(nodos_t, aristas_t, raiz_t(na), raiz_t(nb))

        transitorio = (rt is not None
                       and (r is None or abs(rt - r) > 0.01 * max(r, 1.0)))

        nota = []
        for nodo in (na, nb):
            if NOTAS_NODO.get(nodo):
                nota.append(NOTAS_NODO[nodo])
        if transitorio:
            nota.append("C1 (680 uF) hace que la lectura arranque en %s y "
                        "suba al valor final en ~1-2 min" % fmt(rt))
        if r is None:
            nota.append("si mide bajo, hay un corto")

        filas.append({
            "tipo": "R",
            "par": "%s - %s" % (etiqueta(pa, sa), etiqueta(pb, sb)),
            "pin_a": pa,
            "senal_a": sa,
            "pin_b": pb,
            "senal_b": sb,
            "esperado_ohm": "" if r is None else round(r, 1),
            "min_ohm": "" if r is None else round(r * (1 - TOL_R), 1),
            "max_ohm": "" if r is None else round(r * (1 + TOL_R), 1),
            "esperado": fmt(r),
            "camino": " + ".join(ruta),
            "lectura_inicial": fmt(rt) if transitorio else "",
            "nota": "; ".join(dict.fromkeys(n for n in nota if n)),
        })
    return filas


def tabla(filas):
    """Imprime la planilla de medicion en la terminal."""
    con = [f for f in filas if f["esperado"] != "abierto"]
    abiertos = len(filas) - len(con)
    masa = sum(1 for f in filas
               if f["esperado"] == "abierto" and f["senal_b"] in ("GND", "+5V"))

    print()
    print("RESISTENCIA   (%d pares)      * = uno de los doce chequeos base"
          % len(con))
    print("   Rangos al %g %% de tolerancia. ~ = la fila incluye el trimmer RV1,"
          % (TOL_R * 100))
    print("   asi que el centro real es el que tenga puesto el cursor.")
    print()
    enc = ("      %-33s %9s %-17s %-19s %8s  %s"
           % ("par", "nominal", "rango 1%", "camino", "arranca", "medido"))
    print(enc)
    print("      " + "-" * (len(enc) - 6))
    for n, f in enumerate(con, 1):
        marca = "*" if es_base(f["senal_a"], f["senal_b"]) else " "
        trim = "~" if "RV1" in f["camino"] else " "
        r = float(f["esperado_ohm"])
        print(" %s%s%2d %-33s %9s %-17s %-19s %8s  %s"
              % (marca, trim, n, f["par"], fmt_k(r), rango(r, TOL_R, fmt_k),
                 f["camino"], f["lectura_inicial"].replace(" ", ""), "_" * 8))

    caps = pares_capacitivos()
    print()
    print("CAPACITANCIA   (%d pares)      rangos al %g %%"
          % (len(caps), TOL_C * 100))
    print()
    enc = ("      %-33s %9s %-17s %-9s %-23s %s"
           % ("par", "nominal", "rango 20%", "compon.", "en paralelo", "medido"))
    print(enc)
    print("      " + "-" * (len(enc) - 6))
    for n, f in enumerate(caps, 1):
        if f["shunt"] is None:
            estado = "nada: lectura limpia"
        else:
            estado = "%s: lectura dudosa" % fmt_k(f["shunt"])
        print("   %2d %-33s %9s %-17s %-9s %-23s %s"
              % (n, f["par"], f["esperado"], f["rango"], f["componentes"],
                 estado, "_" * 8))
    print()
    print("   C1 (680uF) y C4 (47nF) no figuran: tienen una pata en un nodo")
    print("   interno, no hay dos pines entre los cuales medirlos.")

    print()
    print("Los otros %d pares dan abierto y estan en el CSV; %d de ellos son"
          % (abiertos, masa))
    print("contra GND y +5V, que igual conviene recorrer para descartar cortos.")
    print()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("-o", "--salida",
                    help="forzar una ruta distinta de outputs/debug_analogico")
    ap.add_argument("--portadora", action="store_true",
                    help="usar la variante de la placa JitX (30k a VREF_2V048)")
    ap.add_argument("--red", action="store_true",
                    help="dibujar la red cargada y salir, sin generar el CSV")
    ap.add_argument("--tabla", action="store_true",
                    help="imprimir la planilla de medicion, sin generar archivo")
    ap.add_argument("--csv", action="store_true",
                    help="generar el CSV plano en vez del Excel con formulas")
    args = ap.parse_args()

    if args.portadora:
        aplicar_portadora()

    if args.red:
        dibujar()
        return

    filas = calcular()

    if args.tabla:
        tabla(filas)
        return

    # Las filas de capacitancia tambien van al CSV, para poder anotarlas.
    for c in pares_capacitivos():
        filas.append({
            "tipo": "C", "par": c["par"],
            "pin_a": c["pin_a"], "senal_a": c["senal_a"],
            "pin_b": c["pin_b"], "senal_b": c["senal_b"],
            "esperado": c["esperado"],
            "camino": c["componentes"],
            "nota": ("sin resistencia en paralelo: lectura limpia"
                     if c["shunt"] is None
                     else "%s en paralelo: la lectura puede no ser fiable"
                          % fmt_k(c["shunt"])),
        })

    if not args.csv:
        # Excel: la columna `ok` es una formula y se verifica sola.
        import planilla
        salida = (Path(args.salida) if args.salida
                  else dir_salida("debug_analogico") / "medidas_analogicas.xlsx")
        guardadas = planilla.escribir(
            salida,
            [f for f in filas if f["tipo"] == "R"],
            pares_capacitivos(),
            es_base, faradios, RV1_FRACCION * 100)
        res = [f for f in filas if f["tipo"] == "R"]
        con = sum(1 for f in res if f["esperado"] != "abierto")
        msg = ("%s\n  hoja Resistencia: %d pares (%d con valor, %d abiertos)"
               "\n  hoja Capacitancia: %d pares"
               % (salida, len(res), con, len(res) - con,
                  sum(1 for f in filas if f["tipo"] == "C")))
        if guardadas:
            msg += "\n  se preservaron %d filas ya anotadas" % guardadas
        print(msg, file=sys.stderr)
        return

    campos = ["tipo", "par", "pin_a", "senal_a", "pin_b", "senal_b",
              "esperado_ohm", "min_ohm", "max_ohm", "esperado", "camino",
              "lectura_inicial", "nota"] + ANOTABLES
    salida = (Path(args.salida) if args.salida
              else dir_salida("debug_analogico") / "medidas_analogicas.csv")

    # Rescatar lo ya anotado a mano antes de sobrescribir.
    anotado = leer_anotaciones(salida)
    for f in filas:
        for col in ANOTABLES:
            f[col] = anotado.get((f["tipo"], f["par"]), {}).get(col, "")

    with salida.open("w", newline="", encoding="utf-8") as fh:
        w = csv.DictWriter(fh, fieldnames=campos, extrasaction="ignore")
        w.writeheader()
        w.writerows(filas)

    res = [f for f in filas if f["tipo"] == "R"]
    con = sum(1 for f in res if f["esperado"] != "abierto")
    cap = sum(1 for f in filas if f["tipo"] == "C")
    guardadas = sum(1 for f in filas if any(f.get(c) for c in ANOTABLES))
    msg = ("%s\n  %d pares de resistencia (%d con valor, %d abiertos)"
           " + %d de capacitancia"
           % (salida, len(res), con, len(res) - con, cap))
    if guardadas:
        msg += "\n  se preservaron %d filas ya anotadas" % guardadas
    print(msg, file=sys.stderr)


if __name__ == "__main__":
    main()
