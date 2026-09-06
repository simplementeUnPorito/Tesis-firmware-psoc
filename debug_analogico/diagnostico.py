#!/usr/bin/env python3
"""Deduce que componente esta fallado a partir de la planilla ya medida.

No usa reglas escritas a mano ("si INp-INn da bajo entonces R2"). Toma el
mismo modelo de red de `red_analogica.py`, le inyecta una falla por vez
(un resistor abierto, en corto, con otro valor, un pin sin soldar, un puente
de estano entre dos pines, un capacitor en corto), vuelve a resolver la red
y se queda con las hipotesis que reproducen lo que dice el multimetro.

Eso da tres cosas que una lista de reglas no da:

  - Distingue lo que NO se puede distinguir. Si R5 abierto y el pin BPo sin
    soldar predicen exactamente lo mismo en los 153 pares, las agrupa en una
    sola hipotesis en vez de acusar a una de las dos.
  - Sirve con la planilla a medio llenar: usa las filas que haya.
  - Dice cual es la proxima medicion mas util, la que mas separa las
    hipotesis que siguen vivas.

    python diagnostico.py                  # lee outputs/debug_analogico/*.xlsx
    python diagnostico.py -i planilla.csv
    python diagnostico.py --md             # ademas escribe el informe a disco
    python diagnostico.py --autotest       # se verifica con fallas inventadas

Convencion para anotar: en la columna `medido kohm` va un numero en kohm, o
`OL` si el multimetro no cierra (asi se distingue "abierto y verificado" de
"todavia no lo medi", que es la celda vacia).
"""

import argparse
import itertools
import math
import sys
from pathlib import Path

import numpy as np

import red_analogica as red

INF = math.inf

# Tokens con los que se puede anotar un "no cierra" en la planilla.
_ABIERTO = {"ol", "o.l", "o.l.", "abierto", "inf", "infinito", "open",
            "oo", "--", "-", "x", "∞", "1.", "of", "overload"}

# Un multimetro no lee menos que esto en las puntas.
PISO_OHM = 50.0
# Por encima de esto, cualquier lectura es un abierto.
TECHO_OHM = 20e6
# Tolerancia con la que se compara medicion contra prediccion. No es la del
# componente (1 %): incluye el error del instrumento y las puntas.
TOL_DIAG = 0.05
# Cuanto hay que esperar antes de creerle a una lectura. No es un numero
# elegido: se midio una fila que seguia subiendo a los 3 minutos, asi que el
# minuto que parecia suficiente no lo es.
ESPERA_MINIMA = 300.0


# ---------------------------------------------------------------------------
# Red: lista explicita de aristas (ref, nodo, nodo, ohm) + mapa pin -> nodo.
# Se trabaja sobre copias para poder mutarlas por hipotesis.
# ---------------------------------------------------------------------------
def red_base():
    aristas = [(ref, a, b, red.valor(ref)) for ref, a, b in red.RESISTORES]
    return aristas, dict(red.PIN_A_NODO)


def _colapsar(aristas):
    """Fusiona los nodos unidos por aristas de 0 ohm y saca las abiertas."""
    alias = {}

    def raiz(n):
        while alias.get(n, n) != n:
            n = alias[n]
        return n

    for ref, a, b, r in aristas:
        if r == 0.0:
            ra, rb = raiz(a), raiz(b)
            if ra != rb:
                alias[ra] = rb
    vivas = [(ref, raiz(a), raiz(b), r) for ref, a, b, r in aristas
             if r != 0.0 and r != INF and raiz(a) != raiz(b)]
    return raiz, vivas


class Red:
    """Red resuelta: da la resistencia entre dos nodos, con memoria."""

    def __init__(self, aristas, pin_a_nodo):
        self.raiz, self.aristas = _colapsar(aristas)
        self.pin_a_nodo = pin_a_nodo
        self._ady = {}
        for _, u, v, _r in self.aristas:
            self._ady.setdefault(u, set()).add(v)
            self._ady.setdefault(v, set()).add(u)
        self._cache = {}
        self._comp = {}

    def _componente(self, a):
        if a in self._comp:
            return self._comp[a]
        visto = {a}
        pila = [a]
        while pila:
            for sig in self._ady.get(pila.pop(), ()):
                if sig not in visto:
                    visto.add(sig)
                    pila.append(sig)
        for n in visto:
            self._comp[n] = visto
        return visto

    def entre_nodos(self, na, nb):
        """Resistencia efectiva. INF si no hay camino resistivo."""
        a, b = self.raiz(na), self.raiz(nb)
        if a == b:
            return 0.0
        clave = (a, b) if a < b else (b, a)
        if clave in self._cache:
            return self._cache[clave]
        comp = self._componente(a)
        if b not in comp:
            self._cache[clave] = INF
            return INF

        locales = sorted(comp)
        idx = {n: i for i, n in enumerate(locales)}
        n = len(locales)
        L = np.zeros((n, n))
        for _ref, u, v, r in self.aristas:
            if u not in idx:
                continue
            g = 1.0 / r
            iu, iv = idx[u], idx[v]
            L[iu, iu] += g
            L[iv, iv] += g
            L[iu, iv] -= g
            L[iv, iu] -= g
        keep = [i for i in range(n) if i != idx[b]]
        Lr = L[np.ix_(keep, keep)]
        corr = np.zeros(len(keep))
        j = keep.index(idx[a])
        corr[j] = 1.0
        v = np.linalg.solve(Lr, corr)
        valor = float(v[j])
        self._cache[clave] = valor
        return valor

    def entre_senales(self, sa, sb):
        return self.entre_nodos(self.pin_a_nodo[sa], self.pin_a_nodo[sb])


# ---------------------------------------------------------------------------
# Hipotesis de falla
# ---------------------------------------------------------------------------
class Hipotesis:
    """Una falla candidata: sabe construir la red que resultaria de ella."""

    def __init__(self, clave, texto, culpables, mutar, familia):
        self.clave = clave              # identificador corto
        self.texto = texto              # como se le explica al usuario
        self.culpables = culpables      # refs de componentes involucrados
        self._mutar = mutar             # (aristas, pinmap) -> (aristas, pinmap)
        self.familia = familia          # sana | componente | pin | puente
        self._red = None
        self._red_t = None

    def red(self):
        if self._red is None:
            self._red = Red(*self._mutar(*red_base()))
        return self._red

    def red_transitorio(self):
        """La misma red pero con C1 todavia cargandose, o sea en corto."""
        if self._red_t is None:
            aristas, pinmap = self._mutar(*red_base())
            aristas = list(aristas) + [("C1_carga", "N_C1", "BPm", 0.0)]
            self._red_t = Red(aristas, pinmap)
        return self._red_t


def _sin_cambios(aristas, pinmap):
    return aristas, pinmap


def _cambiar(ref, nuevo):
    def mutar(aristas, pinmap):
        return ([(r, a, b, (nuevo if r == ref else v))
                 for r, a, b, v in aristas], pinmap)
    return mutar


def _aislar_pin(senal):
    def mutar(aristas, pinmap):
        pinmap = dict(pinmap)
        pinmap[senal] = "AISLADO_%s" % senal
        return aristas, pinmap
    return mutar


def _puentear(na, nb, ohm=0.0):
    def mutar(aristas, pinmap):
        return list(aristas) + [("PUENTE", na, nb, ohm)], pinmap
    return mutar


def _es_pin_libre(senal):
    """Pines que no cuelgan de ningun resistor: aislarlos no cambia nada."""
    nodo = red.PIN_A_NODO[senal]
    return not any(nodo in (a, b) for _, a, b in red.RESISTORES)


def hipotesis_discretas():
    """Todas las fallas de si/no: abierto, corto, pin suelto, puente."""
    hs = [Hipotesis("SANA", "la red esta como dice el modelo", [],
                    _sin_cambios, "sana")]

    for ref, a, b in red.RESISTORES:
        hs.append(Hipotesis(
            "%s:abierto" % ref,
            "%s abierto o sin soldar (%s - %s)" % (ref, a, b),
            [ref], _cambiar(ref, INF), "componente"))
        hs.append(Hipotesis(
            "%s:corto" % ref,
            "%s en corto o puenteado con estano (%s - %s)" % (ref, a, b),
            [ref], _cambiar(ref, 0.0), "componente"))

    for ref, a, b, v in red.CONDENSADORES:
        hs.append(Hipotesis(
            "%s:corto" % ref,
            "%s (%s) en corto: %s y %s quedan unidos" % (ref, v, a, b),
            [ref], _puentear(a, b), "componente"))

    for pin, senal in red.PINES:
        if senal in ("GND", "+5V") or _es_pin_libre(senal):
            continue
        hs.append(Hipotesis(
            "PIN:%s" % senal,
            "%s %s sin soldar o con la pista cortada" % (pin, senal),
            [], _aislar_pin(senal), "pin"))

    for (pa, sa), (pb, sb) in itertools.combinations(red.PINES, 2):
        na, nb = red.PIN_A_NODO[sa], red.PIN_A_NODO[sb]
        hs.append(Hipotesis(
            "PUENTE:%s-%s" % (sa, sb),
            "puente de estano entre %s y %s" % (red.etiqueta(pa, sa),
                                                red.etiqueta(pb, sb)),
            [], _puentear(na, nb), "puente"))
    return hs


def hipotesis_de_valor(ref, nuevo):
    return Hipotesis(
        "%s:valor" % ref, "%s con otro valor" % ref, [ref],
        _cambiar(ref, nuevo), "componente")


# ---------------------------------------------------------------------------
# Lectura de la planilla
# ---------------------------------------------------------------------------
def _numero(txt):
    """Convierte la celda a ohm. Devuelve INF si es un abierto, None si vacia.

    La columna esta en kohm; se aceptan sufijos por si alguien anota '2M'.
    """
    if txt is None:
        return None
    if isinstance(txt, (int, float)):
        return float(txt) * 1e3
    s = str(txt).strip().lower().replace(",", ".")
    if not s:
        return None
    if s in _ABIERTO:
        return INF
    if s.startswith(">"):
        # '>45' es "al menos 45 y seguia subiendo": una cota, no un abierto.
        # Solo '>20M' y parecidos, por encima del techo, son un abierto real.
        v = _numero(s[1:])
        return INF if v is None or v >= TECHO_OHM else v
    escala = 1e3
    if s.endswith("m"):
        s, escala = s[:-1], 1e6
    elif s.endswith("k"):
        s, escala = s[:-1], 1e3
    elif s.endswith("r") or s.endswith("ohm"):
        s = s.rstrip("mho")
        escala = 1.0
    try:
        return float(s) * escala
    except ValueError:
        return None


def _es_cota(txt):
    """'>45' -> la lectura no llego a la meseta; vale como minimo, no como
    medicion. Es la unica forma de anotar 'lo deje subiendo y me fui'."""
    return str(txt).strip().startswith(">") if txt is not None else False


def _segundos(txt):
    if txt is None:
        return None
    if isinstance(txt, (int, float)):
        return float(txt)
    s = str(txt).strip().replace(",", ".")
    try:
        return float(s)
    except ValueError:
        return None


def mapa_pares():
    """'P1[2] INp - P1[7] INn' -> ('INp', 'INn')."""
    m = {}
    for (pa, sa), (pb, sb) in itertools.combinations(red.PINES, 2):
        m["%s - %s" % (red.etiqueta(pa, sa), red.etiqueta(pb, sb))] = (sa, sb)
    return m


class Medicion:
    def __init__(self, par, sa, sb, final, inicial, espera, transitorio,
                 esperado):
        self.par = par
        self.sa = sa
        self.sb = sb
        self.final = final          # ohm, INF o None
        self.inicial = inicial      # ohm, INF o None
        self.espera = espera        # s o None
        self.transitorio = transitorio   # la fila la afecta C1
        self.esperado = esperado    # ohm o INF, segun el modelo sano
        self.repetidas = []         # lecturas de la hoja Remedir
        self.cota = False           # anotada con '>': es un minimo
        self.origen = "planilla"
        self.notas = ""


class MedicionC:
    def __init__(self, par, componentes, esperado, medido, shunt):
        self.par = par
        self.componentes = componentes   # ['C2', 'C17']
        self.esperado = esperado         # faradios
        self.medido = medido             # faradios o None
        self.shunt = shunt               # ohm en paralelo o None


def leer_xlsx(ruta):
    from openpyxl import load_workbook
    wb = load_workbook(ruta, data_only=False)
    pares = mapa_pares()
    med, medc = [], []

    if "Resistencia" in wb.sheetnames:
        for f in wb["Resistencia"].iter_rows(min_row=2, values_only=True):
            par = (f[1] or "").strip()
            if par not in pares:
                continue
            sa, sb = pares[par]
            esperado = INF if str(f[2]).strip() == "abierto" else (
                float(f[2]) * 1e3)
            med.append(Medicion(par, sa, sb, _numero(f[10]), _numero(f[8]),
                                _segundos(f[9]), f[6] not in (None, ""),
                                esperado))

    if "Capacitancia" in wb.sheetnames:
        for f in wb["Capacitancia"].iter_rows(min_row=2, values_only=True):
            par = (f[0] or "").strip()
            if not par or f[3] in (None, ""):
                continue
            escala = {"pF": 1e-12, "nF": 1e-9, "uF": 1e-6}[str(f[2]).strip()]
            crudo = f[9]
            medido = None
            if isinstance(crudo, (int, float)):
                medido = float(crudo) * escala
            elif crudo not in (None, ""):
                try:
                    medido = float(str(crudo).replace(",", ".")) * escala
                except ValueError:
                    medido = INF if str(crudo).strip().lower() in _ABIERTO \
                        else None
            shunt = None
            texto = str(f[6] or "")
            if "kohm" in texto:
                try:
                    shunt = float(texto.split("kohm")[0].strip()) * 1e3
                except ValueError:
                    shunt = None
            medc.append(MedicionC(par, [c.strip() for c in
                                        str(f[1] or "").split("+") if c.strip()],
                                  float(f[3]) * escala, medido, shunt))

    # La tanda de Remedir pisa a la original: se hizo despues y a proposito.
    por_par, directas, marcadas = leer_remedir(wb)
    por_nombre = {m.par: m for m in med}
    for par, ((sa, sb), valor, lecturas, notas) in por_par.items():
        m = por_nombre.get(par)
        if m is None:
            m = Medicion(par, sa, sb, None, None, None, False, INF)
            med.append(m)
        m.final = valor     # None si la lectura todavia estaba subiendo
        m.espera = 120
        m.repetidas = lecturas
        m.origen = "remedir"
        m.notas = notas
        m.cota = par in marcadas
    return med, medc, directas


def leer_remedir(wb):
    """Lee la hoja Remedir: es la tanda nueva, hecha a proposito y con cuidado.

    Devuelve (por_par, directas). Cada fila puede traer hasta tres lecturas;
    se usa la mediana, y si no coinciden entre si eso ES el dato: un par que
    da tres valores distintos tiene contacto intermitente, no un componente
    con un valor raro.
    """
    por_par, directas, marcadas_cota = {}, {}, set()
    pares = mapa_pares()
    filas = []
    for hoja in ("Medido", "Remedir"):
        # 'Medido' primero: si un par esta en las dos, vale la tanda nueva.
        if hoja in wb.sheetnames:
            filas += list(wb[hoja].iter_rows(min_row=3, values_only=True))
    for f in filas:
        if len(f) < 8 or not f[1]:
            continue
        par = str(f[1]).strip()
        lecturas = [v for v in (_numero(f[5]), _numero(f[6]), _numero(f[7]))
                    if v is not None]
        if not lecturas:
            continue
        valor = _estabilizada(lecturas)
        if any(_es_cota(v) for v in (f[5], f[6], f[7])):
            marcadas_cota.add(par)
        if par.startswith("pads de "):
            ref = par[len("pads de "):].strip()
            if valor != INF:
                directas[ref] = (valor, lecturas)
            continue
        if par in pares:
            por_par[par] = (pares[par], valor, lecturas, str(f[8] or ""))
    for par, (nodos, valor, lecturas, notas) in list(por_par.items()):
        if _clasificar(lecturas) == "subiendo":
            # Se guarda como cota, no como valor: el ajuste no la puede usar.
            por_par[par] = (nodos, None, lecturas, notas)
    return por_par, directas, marcadas_cota


def _estabilizada(lecturas, umbral=0.03):
    """El valor asentado, no la mediana cruda.

    Las lecturas se toman en orden y el transitorio capacitivo siempre SUBE:
    un capacitor en paralelo arranca como un corto y se va abriendo. Asi que
    lo bueno es la meseta del final, no el promedio de todo. Se toma la
    ultima y se promedia con las que coincidan con ella.
    """
    if not lecturas:
        return None
    ultima = lecturas[-1]
    if ultima == INF:
        return INF
    iguales = [v for v in lecturas
               if v != INF and abs(v - ultima) <= umbral * ultima + PISO_OHM]
    return sum(iguales) / len(iguales)


def _clasificar(lecturas):
    """Que tipo de secuencia es. Distingue transitorio de contacto malo.

    La diferencia es el signo: un transitorio capacitivo empieza BAJO y sube
    hasta la meseta. Una lectura que empieza ALTA y despues cae no es ningun
    capacitor: es resistencia de contacto en la primera pasada. Y si las dos
    ultimas no coinciden entre si, no hay meseta y el contacto es
    intermitente.
    """
    finitas = [v for v in lecturas if v != INF]
    if len(lecturas) < 2:
        return "unica"
    if len(finitas) != len(lecturas):
        return "intermitente"   # algunas cierran y otras no
    a, b = finitas[-2], finitas[-1]
    if abs(b - a) <= 0.03 * max(b, 1.0) + PISO_OHM:
        if finitas[0] < b * 0.97 - PISO_OHM:
            return "transitorio"   # subio y se planto: la meseta es buena
        return "estable"
    if b > a:
        # Sigue subiendo: el capacitor no termino de cargarse. El ultimo
        # numero NO es la resistencia, es una cota inferior. Usarlo como si
        # fuera la medida mete un error del tamano de lo que falte subir.
        return "subiendo"
    if finitas[0] > b * 1.10 + PISO_OHM:
        return "contacto"       # arranco alto y bajo: no lo hace un capacitor
    return "intermitente"       # nunca llego a una meseta


def leer_csv(ruta):
    import csv
    pares = mapa_pares()
    med, medc = [], []
    with open(ruta, newline="", encoding="utf-8-sig") as fh:
        for fila in csv.DictReader(fh):
            par = (fila.get("par") or "").strip()
            if fila.get("tipo") != "R" or par not in pares:
                continue
            sa, sb = pares[par]
            crudo = fila.get("esperado_ohm") or ""
            esperado = INF if not crudo else float(crudo)
            # El CSV guarda ohm, no kohm.
            def _ohm(txt):
                v = _numero(txt)
                return v if v in (None, INF) else v / 1e3
            med.append(Medicion(par, sa, sb, _ohm(fila.get("medido")),
                                _ohm(fila.get("medido_inicial")),
                                _segundos(fila.get("espera_s")),
                                bool(fila.get("lectura_inicial")), esperado))
    return med, medc, {}


# ---------------------------------------------------------------------------
# Comparacion medicion / prediccion
# ---------------------------------------------------------------------------
def coincide(pred, medido, tol=TOL_DIAG):
    if medido is None:
        return True
    if pred == INF or medido == INF:
        # Un abierto medido contra un abierto predicho, o casi: por encima de
        # TECHO_OHM el multimetro y el modelo dicen lo mismo.
        a = pred == INF or pred > TECHO_OHM
        b = medido == INF or medido > TECHO_OHM
        return a == b
    return abs(medido - pred) <= tol * max(pred, PISO_OHM) + PISO_OHM


def residuo(pred, medido):
    """Distancia en decadas; acotada para que un abierto no domine la suma."""
    if medido is None:
        return 0.0
    p = min(pred, TECHO_OHM) if pred != INF else TECHO_OHM
    m = min(medido, TECHO_OHM) if medido != INF else TECHO_OHM
    p = max(p, PISO_OHM)
    m = max(m, PISO_OHM)
    return abs(math.log10(m / p))


class Evaluacion:
    def __init__(self, h):
        self.h = h
        self.fallan = []        # pares que la hipotesis no explica
        self.fallan_t = []      # idem, en las primeras lecturas
        self.residuo = 0.0
        self.explica = []       # pares que estaban mal y ahora cierran


def evaluar(h, mediciones, malas_sanas, tol=TOL_DIAG):
    ev = Evaluacion(h)
    r = h.red()
    rt = h.red_transitorio()
    for m in mediciones:
        if m.final is not None:
            pred = r.entre_senales(m.sa, m.sb)
            if not coincide(pred, m.final, tol):
                ev.fallan.append((m, pred))
            elif m.par in malas_sanas:
                ev.explica.append(m.par)
            ev.residuo += residuo(pred, m.final)
        if m.inicial is not None:
            predt = rt.entre_senales(m.sa, m.sb)
            if not coincide(predt, m.inicial, tol):
                ev.fallan_t.append((m, predt))
            ev.residuo += 0.5 * residuo(predt, m.inicial)
    return ev


def firma(h, pares):
    """Prediccion sobre TODOS los pares: dos hipotesis con la misma firma son
    indistinguibles con el ohmetro, por mas que sean componentes distintos."""
    r = h.red()
    salida = []
    for sa, sb in pares:
        v = r.entre_senales(sa, sb)
        salida.append("inf" if v == INF else "%.4g" % v)
    return tuple(salida)


# ---------------------------------------------------------------------------
# Ajuste de valor de un resistor
# ---------------------------------------------------------------------------
def _afectadas(ref, mediciones):
    """Filas cuya lectura cambia si `ref` cambia de valor.

    Son las que tienen a `ref` en su camino. Las demas dan lo mismo que en la
    red sana, asi que no hace falta volver a resolverlas para cada valor de
    prueba: eso es lo que hace que el barrido no tarde un minuto.
    """
    _r, _n, aristas = red.construir()
    raiz_t, _nt, aristas_t = red.construir(cortocircuitos=[("N_C1", "BPm")])
    idx = []
    for i, m in enumerate(mediciones):
        na, nb = red.PIN_A_NODO[m.sa], red.PIN_A_NODO[m.sb]
        if m.final is not None and ref in red.camino(aristas, na, nb):
            idx.append(i)
        elif m.inicial is not None and ref in red.camino(aristas_t, raiz_t(na),
                                                         raiz_t(nb)):
            idx.append(i)
    return idx


def _puntaje_sub(h, mediciones, idx, tol):
    """(fallan, fallan_t, residuo) mirando solo las filas de `idx`."""
    r, rt = h.red(), h.red_transitorio()
    n, nt, res = 0, 0, 0.0
    for i in idx:
        m = mediciones[i]
        if m.final is not None:
            pred = r.entre_senales(m.sa, m.sb)
            n += not coincide(pred, m.final, tol)
            res += residuo(pred, m.final)
        if m.inicial is not None:
            predt = rt.entre_senales(m.sa, m.sb)
            nt += not coincide(predt, m.inicial, tol)
            res += 0.5 * residuo(predt, m.inicial)
    return n, nt, res


def ajustar_valor(ref, mediciones, tol=TOL_DIAG):
    """Busca el valor de `ref` que mejor explica lo medido.

    Devuelve (valor, evaluacion completa con ese valor).
    """
    idx = _afectadas(ref, mediciones)
    if not idx:
        # Ninguna medicion lo ve: el valor queda indeterminado, se deja el
        # nominal para que la hipotesis no aparezca en el informe.
        return red.valor(ref), evaluar(hipotesis_de_valor(ref, red.valor(ref)),
                                       mediciones, set(), tol)

    nominal = red.valor(ref)
    mejor = None
    grilla = [nominal * (10 ** e) for e in np.linspace(-2, 2, 61)]
    for _ronda in range(3):
        for v in grilla:
            if v <= 0:
                continue
            marca = _puntaje_sub(hipotesis_de_valor(ref, v), mediciones, idx,
                                 tol)
            if mejor is None or marca < mejor[0]:
                mejor = (marca, v)
        centro = mejor[1]
        paso = (grilla[1] / grilla[0]) ** 0.5
        grilla = [centro * (paso ** (k / 5.0)) for k in range(-10, 11)]
    v = mejor[1]
    return v, evaluar(hipotesis_de_valor(ref, v), mediciones, set(), tol)


def ajustar_puente(na, nb, mediciones, tol=TOL_DIAG):
    """Resistencia desconocida en paralelo entre dos nodos.

    Cubre tres cosas reales: el geofono que quedo enchufado en J4, un
    componente montado donde no va, y la fuga por flux o suciedad entre dos
    pistas. Devuelve (ohm, evaluacion).
    """
    mejor = None
    grilla = [10.0 * (10 ** e) for e in np.linspace(0, 5, 51)]
    for _ronda in range(3):
        for v in grilla:
            h = Hipotesis("FUGA", "", [], _puentear(na, nb, v), "puente")
            ev = evaluar(h, mediciones, set(), tol)
            marca = (len(ev.fallan), len(ev.fallan_t), ev.residuo)
            if mejor is None or marca < mejor[0]:
                mejor = (marca, v, ev)
        centro = mejor[1]
        paso = (grilla[1] / grilla[0]) ** 0.5
        grilla = [centro * (paso ** (k / 5.0)) for k in range(-10, 11)]
    return mejor[1], mejor[2]


def fraccion_rv1(mediciones):
    """RV1 depende de donde este el cursor: se despeja de BPo - SUMm.

    Esa fila tambien la toca C1, asi que si se anoto apurada da un valor
    absurdo. Es preferible declarar el cursor desconocido antes que ajustar
    el modelo a una lectura a medio transitorio: eso corrompe todo el resto
    del diagnostico, no solo las cuatro filas del trimmer.
    """
    if getattr(red, "RV1_FIJO", None) is not None:
        # No hay cursor: RV1 es un fijo. Ajustar una "fraccion" aca seria
        # absorber en el modelo cualquier problema real de esa rama.
        return None, ("es un fijo de %s, no un trimmer: no hay cursor que "
                      "despejar" % red.fmt_k(red.RV1_FIJO))
    for m in mediciones:
        if {m.sa, m.sb} != {"BPo", "SUMm"} or m.final in (None, INF):
            continue
        if m.transitorio and m.espera is not None and m.espera < 60:
            return None, "esa fila se anoto a los %gs, a medio transitorio" % m.espera
        rv1 = m.final - red.VALORES["R7"]
        if 0 <= rv1 <= red.RV1_TOTAL:
            return rv1 / red.RV1_TOTAL, ""
        # No es el cursor: R7 o el propio trimmer estan mal.
        return None, ("da %s, que no es R7 mas algo entre 0 y %s"
                      % (fmt_r(m.final), fmt_r(red.RV1_TOTAL)))
    return None, "todavia no esta medida"


# ---------------------------------------------------------------------------
# Capacitores
# ---------------------------------------------------------------------------
def diagnosticar_capacitores(medc, tol=red.TOL_C):
    lineas = []
    for c in medc:
        if c.medido is None:
            continue
        if c.medido == INF:
            lineas.append((c, "no cierra: %s abierto o sin soldar"
                           % " y ".join(c.componentes)))
            continue
        if abs(c.medido - c.esperado) <= tol * c.esperado:
            continue
        detalle = "%s en vez de %s" % (red.fmt_c(c.medido),
                                       red.fmt_c(c.esperado))
        # Con dos capacitores en paralelo se puede decir cual falta.
        if len(c.componentes) > 1:
            for ref in c.componentes:
                resto = c.esperado - red.faradios(red.vc(ref))
                if resto > 0 and abs(c.medido - resto) <= tol * resto:
                    detalle += ": es justo la suma sin %s, o sea que %s " \
                               "falta o esta abierto" % (ref, ref)
                    break
        if c.medido > c.esperado * (1 + tol) and c.shunt:
            detalle += " (ojo: %s en paralelo, la lectura ya venia dudosa)" \
                       % red.fmt_k(c.shunt)
        lineas.append((c, detalle))
    return lineas


# ---------------------------------------------------------------------------
# Proxima medicion util
# ---------------------------------------------------------------------------
def discriminantes(finalistas, mediciones, tope=5):
    """Pares sin medir que mas separan a las hipotesis que siguen vivas."""
    ya = {frozenset((m.sa, m.sb)) for m in mediciones if m.final is not None}
    candidatos = []
    for (pa, sa), (pb, sb) in itertools.combinations(red.PINES, 2):
        if frozenset((sa, sb)) in ya:
            continue
        preds = [h.red().entre_senales(sa, sb) for h in finalistas]
        distintas = 0
        for x, y in itertools.combinations(preds, 2):
            if not coincide(x, y) and not coincide(y, x):
                distintas += 1
        if distintas:
            candidatos.append((distintas, sa, sb, preds))
    candidatos.sort(key=lambda t: -t[0])
    return candidatos[:tope]


# ---------------------------------------------------------------------------
# Informe
# ---------------------------------------------------------------------------
def fmt_r(v):
    if v is None:
        return "sin medir"
    if v == INF or v > TECHO_OHM:
        return "abierto"
    if v < 1e3:
        return "%.0f ohm" % v
    return "%.3f k" % (v / 1e3)


def informe(mediciones, medc, tol=TOL_DIAG, sugerir=True, directas=None):
    salida = []
    def p(txt=""):
        salida.append(txt)

    con_dato = [m for m in mediciones if m.final is not None]
    p("PLANILLA")
    p("   %d de %d filas de resistencia medidas." % (len(con_dato),
                                                     len(mediciones)))
    if not con_dato:
        p("   No hay nada anotado todavia: no hay nada que diagnosticar.")
        return "\n".join(salida)

    # El trimmer no tiene un valor "correcto": el que vale es el que tenga
    # puesto el cursor. Se despeja de BPo-SUMm y se usa ese, o las cuatro
    # filas que pasan por RV1 dan falsos positivos.
    frac, motivo = fraccion_rv1(mediciones)
    if frac is not None:
        p("   RV1: el cursor medido queda en %.1f %% (%s); el script traia "
          "%.1f %%." % (frac * 100, red.fmt_k(frac * red.RV1_TOTAL),
                        red.RV1_FRACCION * 100))
        if abs(frac - red.RV1_FRACCION) > 0.05:
            p("        Conviene corregir RV1_FRACCION en red_analogica.py.")
        red.RV1_FRACCION = frac
    elif getattr(red, "RV1_FIJO", None) is not None:
        p("   RV1: %s. La ganancia del sumador queda fija: no es un grado de"
          % motivo)
        p("        libertad del modelo ni una incognita de la medicion.")
    else:
        p("   RV1: no se sabe donde esta el cursor (BPo - SUMm: %s);" % motivo)
        p("        las cuatro filas que pasan por el trimmer valen poco.")

    sana = Hipotesis("SANA", "la red esta como dice el modelo", [],
                     _sin_cambios, "sana")
    ev_sana = evaluar(sana, mediciones, set(), tol)
    malas = {m.par for m, _ in ev_sana.fallan}

    # Filas fuera de rango que solo estan a medio transitorio.
    a_esperar = [(m, pr) for m, pr in ev_sana.fallan
                 if m.transitorio and m.espera is not None
                 and m.espera < ESPERA_MINIMA]
    if a_esperar:
        p()
        p("FALTA ESPERAR   (C1 de 680 uF todavia cargandose)")
        for m, pred in a_esperar:
            p("   %-34s medido %-10s esperado %-10s espero %ss"
              % (m.par, fmt_r(m.final), fmt_r(pred), m.espera))
        p("   No se toman como falla ni entran al ajuste. Repetir esperando")
        p("   a que el numero deje de moverse (hasta %g s)." % ESPERA_MINIMA)
    ignorar = {m.par for m, _ in a_esperar}
    duras = [(m, pr) for m, pr in ev_sana.fallan if m.par not in ignorar]

    caps = diagnosticar_capacitores(medc)

    if not duras:
        p()
        if caps:
            p("RESULTADO: las resistencias coinciden con el modelo, pero hay")
            p("           capacitores fuera de rango (mas abajo).")
        else:
            p("RESULTADO: todo lo medido coincide con el modelo.")
        if ev_sana.fallan_t:
            p()
            p("   Salvo las primeras lecturas de estas filas, que no dan lo")
            p("   que predice C1 en corto. No es una falla del circuito, pero")
            p("   revisar si la 1a lectura se anoto realmente al apoyar:")
            for m, pred in ev_sana.fallan_t[:6]:
                p("      %-34s anotado %-10s deberia arrancar en %s"
                  % (m.par, fmt_r(m.inicial), fmt_r(pred)))
        _cerrar(p, caps, mediciones, tol)
        return "\n".join(salida)

    p()
    p("MEDICIONES QUE NO CIERRAN   (%d)" % len(duras))
    p("   %-34s %-12s %-12s %s" % ("par", "medido", "esperado", "camino"))
    p("   " + "-" * 76)
    for m, pred in duras:
        camino = red.camino(red.construir()[2], red.PIN_A_NODO[m.sa],
                            red.PIN_A_NODO[m.sb])
        p("   %-34s %-12s %-12s %s" % (m.par, fmt_r(m.final), fmt_r(pred),
                                       " + ".join(camino) or "-"))

    # Un par que da tres valores distintos no tiene un componente raro:
    # tiene contacto intermitente. Se dice antes que nada, porque invalida
    # cualquier cosa que se deduzca de esa fila.
    clases = {m.par: _clasificar(m.repetidas) for m in mediciones
              if m.repetidas}
    malos = [m for m in mediciones
             if clases.get(m.par) in ("intermitente", "contacto")]
    if malos:
        p()
        p("LECTURAS REPETIDAS QUE DELATAN LA PUNTA, NO EL CIRCUITO   (%d)"
          % len(malos))
        p("   Un transitorio capacitivo siempre SUBE: el capacitor arranca")
        p("   como un corto y se va abriendo. Una lectura que arranca ALTA y")
        p("   despues baja no la hace ningun capacitor: es resistencia de")
        p("   contacto. Y si las dos ultimas no coinciden, no hubo meseta.")
        p()
        for m in malos:
            p("   %-34s %-28s %s"
              % (m.par, ", ".join(fmt_r(v) for v in m.repetidas),
                 "contacto malo en la 1a pasada"
                 if clases[m.par] == "contacto" else "INTERMITENTE"))

    subiendo = [m for m in mediciones if clases.get(m.par) == "subiendo"]
    if subiendo:
        p()
        p("TODAVIA SUBIAN AL ANOTARLAS   (%d)" % len(subiendo))
        p("   El capacitor no habia terminado de cargarse, asi que el ultimo")
        p("   numero no es la resistencia: es una COTA INFERIOR. No entran al")
        p("   ajuste, porque usarlas como si fueran la medida mete un error")
        p("   del tamano de lo que faltaba subir.")
        p()
        for m in subiendo:
            v = Red(*red_base()).entre_senales(m.sa, m.sb)
            p("   %-34s %-28s >= %-10s (deberia dar %s)"
              % (m.par, ", ".join(fmt_r(x) for x in m.repetidas),
                 fmt_r(m.repetidas[-1]), fmt_r(v)))

    asentadas = [m for m in mediciones if clases.get(m.par) == "transitorio"]
    if asentadas:
        p()
        p("   Otras %d filas mostraron el transitorio y se asentaron bien; se"
          % len(asentadas))
        p("   usa la meseta del final, no la primera lectura.")

    # Si el transitorio aparece en filas que el modelo no marca (no solo las
    # de C1), entonces la espera hace falta en todos lados y cualquier fila
    # vieja anotada rapido es sospechosa, este o no fuera de rango.
    inesperado = [m for m in asentadas if not m.transitorio]
    apuradas = [m for m in mediciones
                if m.final is not None and not m.repetidas
                and m.espera is not None and m.espera < ESPERA_MINIMA]
    if inesperado and apuradas:
        p()
        p("MEDIDAS APURADAS   (%d)" % len(apuradas))
        p("   %d filas se asentaron despacio SIN que el modelo prediga ningun"
          % len(inesperado))
        p("   transitorio ahi, asi que la espera hace falta en toda la placa")
        p("   y no solo alrededor de C1. Estas se anotaron con menos de %gs y"
          % ESPERA_MINIMA)
        p("   no se repitieron: valen poco aunque hayan dado bien.")
        p()
        for m in sorted(apuradas, key=lambda m: m.espera)[:12]:
            p("   %-34s %-11s espero %gs" % (m.par, fmt_r(m.final), m.espera))
        if len(apuradas) > 12:
            p("   ... y %d mas." % (len(apuradas) - 12))

    _seccion_cotas(p, mediciones)
    aj = ajustar_red(mediciones, directas=directas)
    sospechosos = _seccion_ajuste(p, aj, tol)
    incoherente = [t for t in aj.residuo
                   if abs(t[2]) > max(4 * aj.escala, 0.04)]
    if incoherente:
        p()
        p("   Mientras haya lecturas que se contradicen, lo de abajo vale")
        p("   poco: el buscador de fallas supone que todas las lecturas son")
        p("   ciertas a la vez, y aca no pueden serlo. Primero remedir.")

    # --- hipotesis -------------------------------------------------------
    evs = []
    for h in hipotesis_discretas():
        if h.familia == "sana":
            continue
        evs.append(evaluar(h, mediciones, malas, tol))
    for ref in list(red.VALORES) + ["RV1"]:
        if ref not in [r for r, _, _ in red.RESISTORES]:
            continue
        v, ev = ajustar_valor(ref, mediciones, tol)
        ev.h = hipotesis_de_valor(ref, v)
        ev.h.texto = ("%s vale %s y no %s (%+.0f %%): valor equivocado, "
                      "mal leido o en paralelo con algo"
                      % (ref, red.fmt_k(v), red.fmt_k(red.valor(ref)),
                         100.0 * (v / red.valor(ref) - 1)))
        if abs(v / red.valor(ref) - 1) > max(tol, 2 * red.TOL_R):
            evs.append(ev)

    # Fuga de los electroliticos y demas capacitores en serie con la senal.
    # Un capacitor con fuga es una resistencia en paralelo que NO desaparece
    # al cargarse: deja todas las lecturas de su lazo un poco por debajo del
    # nominal, para siempre. Es distinto de un capacitor en corto y distinto
    # de un resistor con el valor cambiado, y no lo cubre ninguna de las
    # otras hipotesis cuando el capacitor no tiene sus dos patas en pines.
    pines = set(red.PIN_A_NODO.values())
    for ref, na, nb, val in red.CONDENSADORES:
        if "GND" in (na, nb):
            continue        # esos ya los cubren las hipotesis de puente
        v, ev = ajustar_puente(na, nb, mediciones, tol)
        if v > 20e6:
            continue        # sin fuga apreciable
        ev.h.texto = ("%s (%s) tiene fuga: %s en paralelo con el capacitor"
                      % (ref, val, fmt_r(v)))
        ev.h.culpables = [ref]
        evs.append(ev)

    # Algo en paralelo donde el modelo dice que no hay nada: solo se prueba
    # en los pares que efectivamente leen de menos, no en los 153.
    for m, pred in duras:
        if m.final == INF or (pred != INF and m.final >= pred):
            continue
        na, nb = red.PIN_A_NODO[m.sa], red.PIN_A_NODO[m.sb]
        v, ev = ajustar_puente(na, nb, mediciones, tol)
        if v < 100.0:
            continue    # eso ya lo cubre la hipotesis de corto
        if {m.sa, m.sb} == {"INp", "INn"}:
            ev.h.texto = ("el geofono quedo enchufado en J4: lo que se lee "
                          "es la bobina, %s" % fmt_r(v))
        else:
            ev.h.texto = ("algo de %s en paralelo entre %s y %s: componente "
                          "mal ubicado, fuga por flux o pistas sucias"
                          % (fmt_r(v), m.sa, m.sb))
        evs.append(ev)

    def dureza(ev):
        return len([1 for m, _ in ev.fallan if m.par not in ignorar])

    evs = [e for e in evs if dureza(e) < len(duras)]
    evs.sort(key=lambda e: (dureza(e), len(e.fallan_t), e.residuo))

    p()
    if not evs:
        p("HIPOTESIS: ninguna falla simple explica esto.")
        p("   Puede haber mas de un componente mal, o el modelo no describe")
        p("   la placa que tenes delante (revisar --portadora y VALORES).")
        return "\n".join(salida)

    # Agrupar las que predicen exactamente lo mismo en los 153 pares.
    pares = [(sa, sb) for (_, sa), (_, sb)
             in itertools.combinations(red.PINES, 2)]
    grupos = []
    vistas = {}
    for ev in evs[:40]:
        f = firma(ev.h, pares)
        if f in vistas:
            vistas[f].append(ev)
        else:
            vistas[f] = [ev]
            grupos.append(vistas[f])

    p("HIPOTESIS ORDENADAS   (una sola falla por vez)")
    for i, grupo in enumerate(grupos[:6], 1):
        ev = grupo[0]
        resto = dureza(ev)
        estado = ("explica TODO lo medido" if resto == 0
                  else "deja %d medicion(es) sin explicar" % resto)
        p()
        p("   %d) %s" % (i, ev.h.texto))
        p("      %s; residuo %.2f" % (estado, ev.residuo))
        if len(grupo) > 1:
            p("      No se distingue de: %s"
              % "; ".join(o.h.texto for o in grupo[1:4]))
        if resto:
            for m, pred in ev.fallan[:3]:
                if m.par in ignorar:
                    continue
                p("      queda mal: %-30s medido %s, predice %s"
                  % (m.par, fmt_r(m.final), fmt_r(pred)))

    ganadoras = [g[0] for g in grupos if dureza(g[0]) == 0]
    p()
    if len(ganadoras) == 1:
        p("CONCLUSION: una sola hipotesis explica todo lo medido.")
        p("   %s" % ganadoras[0].h.texto)
        if len(grupos[0]) > 1:
            p("   Con el ohmetro no se puede ir mas fino: las variantes")
            p("   listadas dan exactamente lo mismo en los 153 pares.")
    elif ganadoras:
        p("CONCLUSION: %d hipotesis siguen en pie con lo medido hasta ahora."
          % len(ganadoras))
    else:
        p("CONCLUSION: ninguna falla simple explica todo. La primera de la")
        p("   lista es la que mas se acerca; puede haber dos componentes mal.")

    if sugerir:
        vivas = ganadoras or [g[0].h for g in grupos[:4]]
        vivas = [e.h if isinstance(e, Evaluacion) else e for e in vivas]
        if len(vivas) > 1:
            cand = discriminantes(vivas, mediciones)
            if cand:
                p()
                p("QUE MEDIR AHORA   (lo que mas separa las hipotesis vivas)")
                for _n, sa, sb, preds in cand:
                    p("   %s - %s" % (sa, sb))
                    for h, v in zip(vivas, preds):
                        p("      %-10s si fuera: %s" % (fmt_r(v), h.texto))

    _cerrar(p, caps, mediciones, tol)
    return "\n".join(salida)


def _seccion_ajuste(p, aj, tol):
    """Valor reconstruido de cada resistor y lecturas que se contradicen."""
    if not aj.estimado and not aj.grupos:
        return []

    p()
    p("VALOR RECONSTRUIDO DE CADA RESISTOR")
    p("   Cada lectura es la suma del camino, asi que con 34 filas y 15")
    p("   resistores el sistema sobra: cada valor sale promediado sobre")
    p("   TODAS las filas que lo tocan, no sobre la unica que lo mide solo.")
    p()
    p("   %-6s %10s %10s %8s %7s %4s %4s  %s"
      % ("ref", "nominal", "medido", "desvio", "error", "usa", "ok", ""))
    p("   'error' es cuanto lo fijan las mediciones: un camino que sale de")
    p("   restar numeros grandes queda mal determinado aunque las lecturas")
    p("   sean buenas. 'usa' = lecturas que lo tocan; 'ok' = cuantas de esas")
    p("   estan asentadas. Con menos de dos, es un reparto, no una medida.")
    p("   " + "-" * 74)
    sospechosos = []
    for r in aj.refs:
        if r in aj.sin_datos or not aj.identificable.get(r):
            continue
        v = aj.estimado[r]
        d = v / red.valor(r) - 1
        tiradas = aj.descartadas_por_ref.get(r, 0)
        if tiradas >= aj.cuenta[r]:
            # Mas lecturas tiradas que buenas: el numero no significa nada.
            p("   %-6s %10s %10s %8s %6d  <-- INDETERMINADO: %d lecturas "
              "descartadas, hay que remedir"
              % (r, red.fmt_k(red.valor(r)), "?", "", aj.cuenta[r], tiradas))
            sospechosos.append((r, None, None))
            continue
        marca = ""
        sig = aj.sigma.get(r, 0.0) / red.valor(r)
        # Un desvio solo cuenta si supera su propia barra de error, no la
        # dispersion global: hay componentes que las mediciones fijan mucho
        # peor que otros aunque las lecturas sean igual de buenas.
        fuera = abs(d) > max(3 * aj.escala, 0.03) and abs(d) > 2 * sig
        buenas = aj.asentadas.get(r, 0)
        if fuera and buenas < 2:
            # El desvio existe, pero repartirlo entre este componente y sus
            # vecinos de camino es arbitrario mientras no haya otra lectura
            # asentada que los separe.
            marca = "<-- MAL DETERMINADO: %s asentada" % (
                "1 sola lectura" if buenas else "ninguna lectura")
            sospechosos.append((r, v, d))
        elif fuera:
            marca = "<-- fuera de lo que explica el instrumento"
            sospechosos.append((r, v, d))
        if tiradas:
            marca += "  (%d descartada(s))" % tiradas
        p("   %-6s %10s %10s %+7.1f%% %7s %4d %4d  %s"
          % (r, red.fmt_k(red.valor(r)), red.fmt_k(v), 100 * d,
             ("+-%.1f%%" % (100 * sig)) if sig else "", aj.cuenta[r],
             buenas, marca))
    for grupo, suma, nominal in aj.grupos:
        p("   %-6s %10s %10s %+7.1f%% %6s  solo se conoce la suma"
          % ("+".join(grupo), red.fmt_k(nominal), red.fmt_k(suma),
             100 * (suma / nominal - 1),
             max(aj.cuenta[g] for g in grupo)))
    if aj.sin_datos:
        p("   sin ninguna lectura que los toque: %s"
          % ", ".join(aj.sin_datos))
    p()
    p("   Dispersion tipica de las lecturas coherentes entre si: %.1f %%."
      % (100 * aj.escala))
    p("   Ese numero es lo que se puede pedirle al instrumento y a las")
    p("   puntas; nada por debajo de eso es una falla.")

    descartadas = {m.par for m in aj.descartadas}
    contra = sorted((t for t in aj.residuo
                     if t[0].par in descartadas
                     or abs(t[2]) > max(4 * aj.escala, 0.04)),
                    key=lambda t: -abs(t[2]))
    if contra:
        p()
        p("LECTURAS QUE SE CONTRADICEN CON EL RESTO   (%d)" % len(contra))
        p("   Ninguna red pasiva puede dar A-B mas B-C distinto de A-C. Si")
        p("   esto pasa no es el circuito: es contacto malo, punta movida o")
        p("   una lectura anotada antes de que terminara el transitorio.")
        p()
        p("   %-34s %10s %10s %8s" % ("par", "medido", "coherente", "desvio"))
        p("   " + "-" * 66)
        for m, pred, rel in contra:
            p("   %-34s %10s %10s %8s"
              % (m.par, fmt_r(m.final), fmt_r(pred),
                 "-" if pred <= 0 else "%+.1f%%" % (100 * rel)))
    return sospechosos


def _seccion_cotas(p, mediciones):
    """Cada cota contra el valor NOMINAL, sin pasar por el ajuste.

    Es la pregunta que importa cuando media placa quedo en cotas: no "cuanto
    mide cada resistor" —eso no se puede saber— sino algo mas modesto y mas
    util, "hay alguna lectura que ya sea IMPOSIBLE con los valores de
    diseno". Una cota solo puede refutar el nominal si lo supera, porque el
    transitorio unicamente lee de menos.
    """
    sano = Red(*red_base())
    filas = []
    for m in mediciones:
        es_cota = (getattr(m, "cota", False)
                   or (m.transitorio and not m.repetidas
                       and (m.espera or 0) < ESPERA_MINIMA))
        if not es_cota or m.final in (None, INF):
            continue
        nominal = sano.entre_senales(m.sa, m.sb)
        filas.append((m, nominal, m.final > nominal * 1.02 + PISO_OHM))
    if not filas:
        return
    p()
    p("COTAS CONTRA EL VALOR DE DISENO   (%d)" % len(filas))
    p("   Lecturas que no llegaron a la meseta. Solo pueden decir 'el valor")
    p("   real es AL MENOS esto'. Si ese minimo ya esta por debajo del")
    p("   nominal, la lectura es compatible con el diseno y no prueba nada")
    p("   en contra; si lo supera, ahi si hay algo que el diseno no explica.")
    p()
    p("   %-34s %11s %11s   %s" % ("par", "al menos", "de diseno", ""))
    p("   " + "-" * 72)
    for m, nominal, refuta in sorted(filas, key=lambda t: -t[0].final):
        p("   %-34s %11s %11s   %s"
          % (m.par, fmt_r(m.final), fmt_r(nominal),
             "REFUTA el nominal" if refuta else "compatible"))
    if not any(t[2] for t in filas):
        p()
        p("   Todas compatibles. O sea: en esa rama no hay ni una sola")
        p("   medicion que contradiga los valores de diseno. Tampoco los")
        p("   confirma, y no se puede avanzar mas sin abrir el lazo de C1.")


def _cerrar(p, caps, mediciones, tol):
    """Capacitores y componentes ya descartados: cierran los dos finales."""
    if caps:
        p()
        p("CAPACITANCIA")
        for c, detalle in caps:
            p("   %-34s %s" % (c.par, detalle))

    verificados = componentes_verificados(mediciones, tol)
    if verificados:
        p()
        p("YA VERIFICADOS   (una fila en rango que los recorre sola)")
        p("   %s" % ", ".join(sorted(verificados)))


# ---------------------------------------------------------------------------
# Reconstruccion de los valores reales a partir de todas las lecturas
#
# La red es un bosque: entre dos pines hay un solo camino, asi que cada
# lectura es la SUMA de los resistores de ese camino. Con 34 lecturas y 15
# resistores el sistema queda sobredeterminado, y de ahi salen dos cosas que
# el ohmetro solo no da:
#
#   - el valor real de cada resistor, promediado sobre todas las lecturas que
#     lo tocan, y no sobre la unica fila que lo mide directo;
#   - cuales lecturas se contradicen entre si. Una contradiccion no es una
#     falla del circuito: ninguna red pasiva puede dar A-B mas B-C distinto
#     de A-C. Es contacto malo, punta que se movio, o una lectura anotada
#     antes de que terminara el transitorio.
# ---------------------------------------------------------------------------
class Ajuste:
    def __init__(self):
        self.refs = []          # resistores en el orden de la solucion
        self.estimado = {}      # ref -> ohm ajustados
        self.cuenta = {}        # ref -> cuantas lecturas lo tocan
        self.identificable = {}  # ref -> bool
        self.grupos = []        # [(refs, suma_ajustada, suma_nominal)]
        self.sin_datos = []
        self.residuo = []       # (medicion, ajustado, error relativo)
        self.escala = 0.0       # dispersion tipica de las lecturas coherentes
        self.descartadas = []   # lecturas que ninguna red puede satisfacer
        self.descartadas_por_ref = {}
        self.asentadas = {}      # ref -> lecturas asentadas que lo tocan
        self.sigma = {}          # ref -> incertidumbre del ajuste, en ohm


def _nucleo(A, tol=1e-9):
    """Base del espacio nulo de A: lo que las mediciones no pueden separar."""
    if A.size == 0:
        return np.zeros((A.shape[1], 0))
    _u, s, vt = np.linalg.svd(A)
    rango = int((s > tol * (s[0] if s.size else 1.0)).sum())
    return vt[rango:].T


def ajustar_red(mediciones, corte=0.08, directas=None):
    """Minimos cuadrados robustos sobre todas las lecturas finitas.

    `corte` es a partir de que desvio una lectura se considera irreconciliable
    con el resto y se saca del ajuste: ningun multimetro se equivoca tanto,
    asi que es contacto o procedimiento, no instrumento.
    """
    _raiz, _nodos, aristas = red.construir()
    refs = [r for r, _, _ in red.RESISTORES]
    idx = {r: i for i, r in enumerate(refs)}

    filas, b, usadas, cotas = [], [], [], []
    for m in mediciones:
        if m.final is None or m.final == INF or m.final <= 0:
            continue

        ruta = red.camino(aristas, red.PIN_A_NODO[m.sa], red.PIN_A_NODO[m.sb])
        if not ruta:
            continue
        f = np.zeros(len(refs))
        for r in ruta:
            f[idx[r]] += 1.0
        filas.append(f)
        b.append(m.final)
        usadas.append(m)
        # Una lectura sin asentar no es una igualdad: como el transitorio
        # capacitivo siempre lee de MENOS, lo unico que afirma es "el valor
        # real es al menos esto". Se marca para que el ajuste no la castigue
        # por quedar corta, pero si por pasarse.
        cotas.append(bool(getattr(m, "cota", False)
                          or (m.transitorio and not m.repetidas
                              and (m.espera or 0) < ESPERA_MINIMA)))

    # Una medida sobre los pads del propio componente es una ecuacion mas,
    # con un solo termino. Es la mas fuerte de todas: no depende del modelo.
    class _Pads:
        def __init__(self, ref, v):
            self.par = "pads de %s" % ref
            self.sa = self.sb = ref
            self.final = v
            self.transitorio = False
            self.espera = None
            self.inicial = None
            self.repetidas = []
    for ref, (v, lecturas) in (directas or {}).items():
        if ref not in idx or v <= 0:
            continue
        f = np.zeros(len(refs))
        f[idx[ref]] = 1.0
        filas.append(f)
        b.append(v)
        cotas.append(False)
        m = _Pads(ref, v)
        m.repetidas = lecturas
        usadas.append(m)

    aj = Ajuste()
    aj.refs = refs
    if not filas:
        return aj

    A = np.array(filas)
    y = np.array(b)
    cota = np.array(cotas, dtype=bool)

    # Un resistor no puede ser negativo: sin esa restriccion, dos lecturas
    # contradictorias sobre el mismo componente se "resuelven" inventando un
    # valor negativo y el disparate se reparte por toda la tabla.
    from scipy.optimize import nnls

    def resolver(mascara, peso=None):
        # Peso relativo: 300 ohm de error no valen lo mismo en 6.8k que en 150k.
        w = 1.0 / y[mascara]
        if peso is not None:
            w = w * np.sqrt(peso[mascara])
        return nnls(A[mascara] * w[:, None], y[mascara] * w)[0]

    def dispersion(x, vive):
        rel = (y - A @ x) / np.maximum(A @ x, 1.0)
        s = 1.4826 * np.median(np.abs(rel[vive] - np.median(rel[vive])))
        return rel, max(float(s), 0.005)

    # Primero se sacan las lecturas groseras: una por contacto malo puede
    # estar 100 veces fuera, y ahi no alcanza con bajarle el peso.
    vive = np.ones(len(y), dtype=bool)
    x = resolver(vive)
    for _ronda in range(len(y)):
        rel, s = dispersion(x, vive)
        # Quedarse corta es lo que se espera de una cota: no se la descarta
        # por eso. Pasarse, en cambio, un transitorio no lo puede explicar.
        peor = np.where(vive & ~(cota & (rel < 0)), np.abs(rel), -1.0)
        j = int(np.argmax(peor))
        if peor[j] <= max(4 * s, corte) or vive.sum() <= len(refs) + 1:
            break
        vive[j] = False
        x = resolver(vive)

    # Despues, Huber sobre las que quedaron: una fila con 5 % de dispersion
    # no debe pesar lo mismo que una que cierra al 0,3 %, o el valor de un
    # resistor bien medido en directo termina arrastrado por sus vecinas.
    for _ronda in range(20):
        rel, s = dispersion(x, vive)
        c = max(1.5 * s, 0.015)
        peso = np.clip(c / np.maximum(np.abs(rel), 1e-9), 0.0, 1.0)
        # Peso casi nulo, no cero: la fila sigue sosteniendo el valor cuando
        # no hay ninguna otra que lo mida, pero no le gana a una buena.
        peso = np.where(cota & (rel < 0), 0.02, peso)
        # Al reves, que el ajuste quede POR DEBAJO de una cota es imposible:
        # el transitorio solo puede leer de menos. Si la unica forma de
        # cerrar es violarla, lo que esta mal es otra lectura, no la cota.
        peso = np.where(cota & (rel > 0), 2.0, peso)
        nuevo = resolver(vive, peso)
        if np.allclose(nuevo, x, rtol=1e-6, atol=1e-6):
            x = nuevo
            break
        x = nuevo
    _rel, aj.escala = dispersion(x, vive)

    # Incertidumbre de cada valor, de la covarianza del ajuste. Importa mas
    # de lo que parece: R15, R9 y R10 salen de restar numeros grandes entre
    # si (R15 = (a + b - c)/2), asi que un 1.5 % en cada lectura se amplifica
    # a varios por ciento en el resultado. Sin esta barra, ese ruido se lee
    # como un componente fuera de tolerancia.
    # Con los pesos FINALES, no con todas las filas por igual: una cota que
    # el ajuste casi no usa tampoco puede achicar la barra de error.
    _rel_f, _s_f = dispersion(x, vive)
    peso_f = np.clip(max(1.5 * _s_f, 0.015)
                     / np.maximum(np.abs(_rel_f), 1e-9), 0.0, 1.0)
    peso_f = np.where(cota & (_rel_f < 0), 0.02, peso_f)
    peso_f = np.where(cota & (_rel_f > 0), 2.0, peso_f)
    Aw = A[vive] * ((1.0 / y[vive]) * np.sqrt(peso_f[vive]))[:, None]
    try:
        cov = np.linalg.pinv(Aw.T @ Aw) * (aj.escala ** 2)
        aj.sigma = {r: float(np.sqrt(max(cov[i, i], 0.0)))
                    for i, r in enumerate(refs)}
    except np.linalg.LinAlgError:
        aj.sigma = {}
    aj.descartadas = [usadas[i] for i in range(len(y)) if not vive[i]]
    aj.descartadas_por_ref = {
        r: int(sum(1 for i in range(len(y))
                   if not vive[i] and A[i, k] > 0))
        for k, r in enumerate(refs)}

    # La identificabilidad se juzga con las filas que quedaron, no con todas.
    A_util = A[vive]
    for i, r in enumerate(refs):
        aj.cuenta_util = getattr(aj, "cuenta_util", {})
        aj.cuenta_util[r] = int((A_util[:, i] > 0).sum())

    N = _nucleo(A_util)
    buenas = A[vive & ~cota]
    for i, r in enumerate(refs):
        aj.cuenta[r] = int((A_util[:, i] > 0).sum())
        # Cuantas de esas lecturas estan realmente asentadas. Un componente
        # sostenido por una sola no tiene un valor medido: tiene un reparto.
        aj.asentadas[r] = int((buenas[:, i] > 0).sum()) if buenas.size else 0
        if aj.cuenta[r] == 0:
            aj.sin_datos.append(r)
            aj.identificable[r] = False
            continue
        aj.identificable[r] = bool(N.shape[1] == 0
                                   or np.allclose(N[i], 0, atol=1e-8))
        aj.estimado[r] = float(x[i])

    # Lo que solo se conoce como suma: los resistores que el espacio nulo
    # mezcla entre si (R7 y RV1 nunca se miden por separado, por ejemplo).
    pendientes = [r for r in refs if aj.cuenta[r] and not aj.identificable[r]]
    while pendientes:
        r = pendientes.pop(0)
        i = idx[r]
        grupo = [r]
        for otro in list(pendientes):
            j = idx[otro]
            if np.any(np.abs(N[j]) > 1e-8) and np.any(np.abs(N[i]) > 1e-8):
                grupo.append(otro)
                pendientes.remove(otro)
        aj.grupos.append((grupo,
                          sum(x[idx[g]] for g in grupo),
                          sum(red.valor(g) for g in grupo)))

    pred = A @ x
    for m, p, v in zip(usadas, pred, y):
        # p puede dar 0 si el ajuste mando ese camino a cero: ahi el desvio
        # relativo no existe y se marca como infinito a proposito.
        rel = INF if p <= 0 else (v - p) / p
        aj.residuo.append((m, float(p), rel))
    return aj


def componentes_verificados(mediciones, tol=TOL_DIAG):
    """Resistores que quedan probados por una fila en rango de un solo salto."""
    _raiz, _nodos, aristas = red.construir()
    ok = set()
    for m in mediciones:
        if m.final is None or m.final == INF:
            continue
        if not coincide(m.esperado, m.final, tol):
            continue
        ruta = red.camino(aristas, red.PIN_A_NODO[m.sa], red.PIN_A_NODO[m.sb])
        if len(ruta) == 1:
            ok.add(ruta[0])
    return ok


# ---------------------------------------------------------------------------
# Autotest: sin placa medida no hay forma de saber si esto anda.
# ---------------------------------------------------------------------------
def _sintetizar(mutar, ruido=0.0, solo=None):
    """Mediciones que daria una placa con esa falla, para probar el buscador."""
    import random
    rng = random.Random(7)
    aristas, pinmap = mutar(*red_base())
    r = Red(aristas, pinmap)
    rt = Red(list(aristas) + [("C1_carga", "N_C1", "BPm", 0.0)], pinmap)
    sano = Red(*red_base())

    med = []
    for (pa, sa), (pb, sb) in itertools.combinations(red.PINES, 2):
        if solo is not None and not solo(sa, sb):
            continue
        v = r.entre_senales(sa, sb)
        vt = rt.entre_senales(sa, sb)
        if v != INF and ruido:
            v *= 1 + rng.uniform(-ruido, ruido)
        if vt != INF and ruido:
            vt *= 1 + rng.uniform(-ruido, ruido)
        esp = sano.entre_senales(sa, sb)
        par = "%s - %s" % (red.etiqueta(pa, sa), red.etiqueta(pb, sb))
        med.append(Medicion(par, sa, sb, v, vt, 120, vt != esp, esp))
    return med


def autotest(verboso=False):
    casos = [
        ("R5 abierto", _cambiar("R5", INF), ("R5",)),
        ("R6 abierto", _cambiar("R6", INF), ("R6",)),
        ("R8 en corto", _cambiar("R8", 0.0), ("R8",)),
        ("R13 abierto", _cambiar("R13", INF), ("R13",)),
        ("R10 de 15k en vez de 150k", _cambiar("R10", 15e3), ("R10",)),
        ("R2 de 4.7k en vez de 47k", _cambiar("R2", 4.7e3), ("R2",)),
        ("puente Vref_BP - Vref_LP", _puentear("BP_REF", "LP_REF"), ()),
        ("C6 en corto (Vref a masa)", _puentear("VREF", "GND"), ("C6",)),
        ("pin SUMo sin soldar", _aislar_pin("SUMo"), ()),
        ("sin falla", _sin_cambios, ()),
    ]
    fallas = 0
    for nombre, mutar, refs in casos:
        med = _sintetizar(mutar, ruido=0.008)
        sana = Hipotesis("SANA", "", [], _sin_cambios, "sana")
        ev_sana = evaluar(sana, med, set())
        malas = {m.par for m, _ in ev_sana.fallan}

        if not refs and nombre == "sin falla":
            bien = not ev_sana.fallan
            print("   %-34s %s" % (nombre, "OK" if bien else "MAL: hay filas"
                                   " fuera de rango sin haber falla"))
            fallas += 0 if bien else 1
            continue

        evs = []
        for h in hipotesis_discretas():
            if h.familia == "sana":
                continue
            ev = evaluar(h, med, malas)
            if not ev.fallan and not ev.fallan_t:
                evs.append(ev)
        ajustes = []
        for ref in [r for r, _, _ in red.RESISTORES]:
            v, ev = ajustar_valor(ref, med)
            if not ev.fallan and not ev.fallan_t \
                    and abs(v / red.valor(ref) - 1) > 0.02:
                ajustes.append((ref, v, ev))

        nombres = [e.h.texto for e in evs] + \
                  ["%s vale %s" % (r, red.fmt_k(v)) for r, v, _ in ajustes]
        acierta = bool(nombres) and (
            not refs or any(any(x in n for x in refs) for n in nombres))
        print("   %-34s %s   (%d hipotesis compatibles)"
              % (nombre, "OK" if acierta else "MAL", len(nombres)))
        if verboso or not acierta:
            for n in nombres[:6]:
                print("        %s" % n)
        fallas += 0 if acierta else 1
    print()
    print("   %d/%d casos" % (len(casos) - fallas, len(casos)))
    return fallas


# ---------------------------------------------------------------------------
# Volcado del diagnostico dentro de la misma planilla
# ---------------------------------------------------------------------------
def pendientes_de_remedir(aj, mediciones, tol=TOL_DIAG):
    """Que hay que volver a medir y por que. Ordenado por lo que mas aporta."""
    _raiz, _nodos, aristas = red.construir()
    por_par = {}

    def anotar(m, motivo, como):
        if getattr(m, "transitorio", False):
            como = ("descargar C1 (puentear BPm con SEo unos segundos); "
                    "lectura 1 = puntas como siempre, lectura 2 = puntas "
                    "invertidas. Si las dos coinciden el componente esta "
                    "realmente asi; si difieren, era la carga de C1")
        elif {getattr(m, "sa", ""), getattr(m, "sb", "")} == {"INp", "INn"}:
            como = ("con J4 desconectado; tiene que dar R2+R3, o sea la suma "
                    "exacta de INp-Vref mas INn-Vref")
        if m.par not in por_par:
            por_par[m.par] = [m, motivo, como]

    descartadas = {m.par for m in aj.descartadas}
    for m, pred, rel in sorted(aj.residuo, key=lambda t: -abs(t[2])):
        if m.par in descartadas or abs(rel) > max(4 * aj.escala, 0.04):
            anotar(m, "contradice al resto de las lecturas (el ajuste "
                      "coherente da %s)" % fmt_r(pred),
                   "tres pasadas, una por columna, levantando y volviendo "
                   "a apoyar las puntas entre cada una")

    # Todo lo que toca a un componente sin valor confiable.
    dudosos = [r for r in aj.refs
               if r in aj.estimado
               and aj.descartadas_por_ref.get(r, 0) >= aj.cuenta[r]]
    for m in mediciones:
        if m.final is None:
            continue
        ruta = red.camino(aristas, red.PIN_A_NODO[m.sa], red.PIN_A_NODO[m.sb])
        tocados = [r for r in ruta if r in dudosos]
        if tocados:
            anotar(m, "%s quedo sin valor confiable" % ", ".join(tocados),
                   "tres pasadas; si vuelve a saltar, es junta fria en el pin")

    # Un componente sospechoso cuyo camino pasa por C1 no se puede juzgar sin
    # descartar antes la carga del electrolitico: es la medida que decide si
    # el componente esta mal o si lo que estaba mal era el procedimiento.
    sospechosos = [r for r in aj.refs
                   if r in aj.estimado and aj.identificable.get(r)
                   and abs(aj.estimado[r] / red.valor(r) - 1)
                   > max(3 * aj.escala, 0.03)]
    for m in mediciones:
        if m.final is None:
            continue
        ruta = red.camino(aristas, red.PIN_A_NODO[m.sa], red.PIN_A_NODO[m.sb])
        tocados = [r for r in ruta if r in sospechosos]
        if not tocados:
            continue
        if m.transitorio:
            anotar(m, "decide si %s esta mal o si lo que falseaba era la "
                      "carga de C1" % ", ".join(tocados), "")
        elif len(ruta) <= 2:
            # Caminos cortos: son los que fijan el valor del componente. Van
            # todos juntos y de corrido, porque lo que hay que comprobar es
            # que la suma cierre, no cada uno por separado.
            anotar(m, "el valor de %s no cierra; este camino es de los que "
                      "mas lo definen" % ", ".join(tocados),
                   "medir toda la tanda de corrido, sin levantar las puntas "
                   "entre filas: lo que se comprueba es que las sumas cierren")

    # Una lectura que no llego a meseta, o que salto entre pasadas, no es una
    # medicion: hay que rehacerla aunque el ajuste no la marque, porque el
    # ajuste directamente no la uso.
    for m in mediciones:
        clase = _clasificar(getattr(m, "repetidas", []))
        if clase == "subiendo":
            anotar(m, "seguia subiendo al anotarla: %s es solo una cota "
                      "inferior" % fmt_r(m.repetidas[-1]),
                   "esperar hasta que el numero deje de moverse, aunque sean "
                   "5 min; o medir el componente en sus pads y evitar el "
                   "transitorio")
        elif clase in ("intermitente", "contacto"):
            anotar(m, "las pasadas no coincidieron entre si (%s)"
                   % ", ".join(fmt_r(v) for v in m.repetidas),
                   "limpiar el pad y repetir; si sigue saltando es junta fria")

    filas = list(por_par.values())

    # --- medidas nuevas que el diagnostico pide por su cuenta -------------
    class _Par:
        def __init__(self, par):
            self.par = par

    etiq = {s: red.etiqueta(pin, s) for pin, s in red.PINES}

    # Cada senal que aparece en varias lecturas contradictorias es sospechosa
    # de junta fria o de corto: se la busca contra masa y contra alimentacion.
    cuenta = {}
    for m, _m2, _c in filas:
        for s in (getattr(m, "sa", None), getattr(m, "sb", None)):
            if s:
                cuenta[s] = cuenta.get(s, 0) + 1
    for senal, n in sorted(cuenta.items(), key=lambda t: -t[1]):
        if n < 2 or senal in ("GND", "+5V"):
            continue
        for masa in ("GND", "+5V"):
            filas.append((
                _Par("%s - %s" % (etiq[senal], masa)),
                "%s aparece en %d lecturas contradictorias: descartar que "
                "este en corto contra %s" % (senal, n, masa),
                "tiene que dar ABIERTO (anotar OL). Cualquier numero es corto"))

    # Un resistor sin valor confiable se cierra midiendolo en sus propios
    # pads: ahi no hay red que interpretar.
    for r in aj.refs:
        malo = (r in aj.estimado and aj.identificable.get(r)
                and (aj.descartadas_por_ref.get(r, 0) >= aj.cuenta.get(r, 0)
                     or abs(aj.estimado[r] / red.valor(r) - 1)
                     > max(3 * aj.escala, 0.03)))
        if not malo:
            continue
        na, nb = red.nodo_de(r)
        pines = set(red.PIN_A_NODO.values())
        if na in pines and nb in pines:
            # Los pads son esos mismos dos pines: apoyar ahi no cambia nada,
            # el transitorio es el mismo. Decirlo, en vez de mandar a medir
            # algo que ya se midio.
            como = ("los pads de %s son esos mismos dos pines, asi que no "
                    "evita el transitorio: o se espera a la meseta, o se "
                    "abre el lazo de C1 levantando una pata de R4" % r)
        else:
            como = ("la punta llega a un nodo que ningun pin alcanza, y no "
                    "hay capacitor en ese lazo: la lectura es inmediata")
        filas.append((
            _Par("pads de %s" % r),
            "el valor reconstruido no cierra (nominal %s)"
            % red.fmt_k(red.valor(r)), como))

    # Todo el transitorio de la placa sale de un solo lazo: SEo-R4-C1-BPm.
    # Abrirlo vuelve inmediatas las diez filas afectadas, y de paso deja
    # medir R4, que desde los pines es invisible porque C1 lo bloquea.
    if any(getattr(m, "transitorio", False) for m, _mo, _c in filas):
        filas.insert(0, (
            _Par("levantar una pata de R4"),
            "las 10 filas con transitorio salen todas del lazo SEo-R4-C1-BPm;"
            " abrirlo las vuelve instantaneas y ademas destapa R4",
            "desoldar una sola pata de R4 (43k), medir el bloque entero de "
            "corrido, medir R4 en sus pads, y volver a soldarla"))

    # Las de pads primero: son las unicas que no obligan a esperar minutos.
    def prioridad(t):
        par = str(getattr(t[0], "par", ""))
        if par.startswith("levantar"):
            return 0
        return 1 if par.startswith("pads de ") else 2
    filas.sort(key=prioridad)
    return filas


_AMARILLO = "FFF2CC"


def escribir_en_planilla(ruta, texto, aj, pendientes):
    """Agrega las hojas 'Diagnostico' y 'Remedir' sin tocar lo anotado."""
    from openpyxl import load_workbook
    from openpyxl.styles import Alignment, Font, PatternFill
    from openpyxl.utils import get_column_letter

    wb = load_workbook(ruta)

    # La hoja se rehace entera en cada corrida, asi que hay que rescatar lo
    # anotado ANTES de borrarla. Y no alcanza con reponer las filas que
    # siguen en la lista: una fila ya resuelta sale de los pendientes, y si
    # se pierde su lectura el diagnostico retrocede.
    previo = {}
    for hoja in ("Medido", "Remedir"):
        if hoja not in wb.sheetnames:
            continue
        for f in wb[hoja].iter_rows(min_row=3, values_only=True):
            if len(f) < 9 or not f[1]:
                continue
            datos = [f[5], f[6], f[7], f[8]]
            if any(v not in (None, "") for v in datos):
                previo[str(f[1]).strip()] = datos

    for nombre in ("Diagnostico", "Remedir", "Medido"):
        if nombre in wb.sheetnames:
            del wb[nombre]

    ws = wb.create_sheet("Diagnostico")
    ws.column_dimensions["A"].width = 120
    ws.sheet_properties.tabColor = "C00000"
    for i, linea in enumerate(texto.split("\n"), start=1):
        c = ws.cell(i, 1, linea)
        c.font = Font(name="Consolas", size=10,
                      bold=linea[:1].isalpha() and linea == linea.upper()
                      and len(linea) > 3)

    ws = _hoja_tabla(wb, "Remedir", "FFC000",
                     "Completar SOLO las celdas amarillas, en kohm, o OL si no "
                "cierra. Aca queda SOLO lo que falta: apenas se completa una "
                "fila y el diagnostico la usa, pasa a la hoja Medido. Tres "
                "lecturas por fila: donde dice 'tres pasadas' van tres "
                "distintas moviendo las puntas, y donde dice polaridad van "
                "la directa y la invertida. Alcanza con la primera si no "
                "dice nada. Despues correr: python diagnostico.py --excel")

    def poner(hoja, n, par, objetivo, motivo, como):
        ws = hoja
        r = ws.max_row + 1
        ws.cell(r, 1, n)
        ws.cell(r, 2, par)
        ws.cell(r, 3, objetivo)
        ws.cell(r, 4, motivo)
        ws.cell(r, 5, como)
        for col in (6, 7, 8, 9):
            ws.cell(r, col).fill = PatternFill("solid", fgColor=_AMARILLO)
        for i, v in enumerate(previo.pop(par, [None] * 4)):
            if v not in (None, ""):
                ws.cell(r, 6 + i, v)
        for col in (4, 5):
            ws.cell(r, col).alignment = Alignment(wrap_text=True,
                                                  vertical="top")
        ws.row_dimensions[r].height = 30

    hechas = wb.create_sheet("Medido")
    _encabezado_tabla(hechas, "92D050",
                      "Registro de lo ya medido. No hace falta tocar nada "
                      "aca: el diagnostico lo sigue leyendo, y esta separado "
                      "de Remedir para no mezclar lo hecho con lo que falta. "
                      "Si una fila hay que rehacerla, borrale las lecturas y "
                      "volve a correr diagnostico.py --excel.")

    quedan = 0
    for m, motivo, como in pendientes:
        par = m.par if hasattr(m, "par") else str(m)
        objetivo = getattr(m, "objetivo", "") or _objetivo(m)
        quedan += 1
        poner(ws, quedan, par, objetivo, motivo, como)

    # Lo medido que ya salio de la lista de pendientes: tambien se conserva.
    for par in list(previo):
        poner(hechas, hechas.max_row - 1, par, "",
              "ya resuelto, fuera de la lista de pendientes", "")
    wb.save(ruta)
    return quedan


def _encabezado_tabla(ws, color, texto):
    from openpyxl.styles import Alignment, Font, PatternFill
    from openpyxl.utils import get_column_letter
    ws.sheet_properties.tabColor = color
    ws["A1"] = texto
    ws["A1"].font = Font(bold=True, size=11, color="1F3864")
    ws["A1"].alignment = Alignment(wrap_text=True, vertical="top")
    ws.merge_cells("A1:I1")
    ws.row_dimensions[1].height = 46
    titulos = ["#", "que medir", "tiene que dar", "por que", "como",
               "lectura 1", "lectura 2", "lectura 3", "notas"]
    anchos = [4, 34, 14, 46, 50, 11, 11, 11, 28]
    for i, (t, a) in enumerate(zip(titulos, anchos), start=1):
        c = ws.cell(2, i, t)
        c.fill = PatternFill("solid", fgColor="1F3864")
        c.font = Font(bold=True, color="FFFFFF")
        c.alignment = Alignment(horizontal="center", wrap_text=True)
        ws.column_dimensions[get_column_letter(i)].width = a
    ws.freeze_panes = "A3"
    return ws


def _hoja_tabla(wb, nombre, color, texto):
    return _encabezado_tabla(wb.create_sheet(nombre), color, texto)


def _objetivo(m):
    """Que tiene que dar la fila, para no volver a la planilla a buscarlo."""
    par = m.par if hasattr(m, "par") else str(m)
    if par.startswith("pads de "):
        ref = par[len("pads de "):]
        try:
            return red.fmt_k(red.valor(ref))
        except KeyError:
            return ""
    # Se calcula del modelo vigente, no de la columna de la planilla: si se
    # corrigio un valor en VALORES, la planilla puede estar sin regenerar y
    # entonces diria un objetivo viejo.
    sa, sb = getattr(m, "sa", None), getattr(m, "sb", None)
    if sa in red.PIN_A_NODO and sb in red.PIN_A_NODO:
        v = Red(*red_base()).entre_senales(sa, sb)
        return "abierto (OL)" if v == INF else red.fmt_k(v)
    return "abierto (OL)"


def tabla_transitorios():
    """De que pares hay que desconfiar, y cuanto hay que esperarles.

    Es una propiedad de la red, no de lo medido: se compara la lectura con
    C1 en corto (t=0) contra la final. El salto dice cuanto miente una
    lectura apurada; tau dice cuanto tarda en dejar de mentir.
    """
    ar, pm = red_base()
    fin, ini = Red(ar, pm), Red(list(ar) + [("c", "N_C1", "BPm", 0.0)], pm)
    R = fin.entre_nodos("N_C1", "BPm")      # lo que ve C1 entre sus patas
    C = red.faradios(red.vc("C1"))
    tau = R * C

    print()
    print("TRANSITORIO DE C1")
    print("   C1 se carga a traves de todo el lazo: %s en serie con sus"
          % red.fmt_k(R))
    print("   %s. Eso da tau = %.0f s, o sea %.1f min para llegar al 99 %% y"
          % (red.vc("C1"), tau, 4.6 * tau / 60))
    print("   %.1f min para el 99.9 %%. No hay atajo: es el lazo entero."
          % (6.9 * tau / 60))
    print()
    print("   %-34s %10s %10s %8s  %s"
          % ("par", "en t=0", "final", "salto", "confianza"))
    print("   " + "-" * 78)
    filas = []
    for (pa, sa), (pb, sb) in itertools.combinations(red.PINES, 2):
        a, b = ini.entre_senales(sa, sb), fin.entre_senales(sa, sb)
        if b == INF or a == b:
            continue
        filas.append((abs(b - a) / b, red.etiqueta(pa, sa), red.etiqueta(pb, sb),
                      a, b))
    for salto, ea, eb, a, b in sorted(filas, reverse=True):
        juicio = ("inservible sin esperar" if salto > 0.30 else
                  "hay que esperar" if salto > 0.05 else
                  "casi no la toca")
        print("   %-34s %10s %10s %7.0f%%  %s"
              % ("%s - %s" % (ea, eb), fmt_r(a), fmt_r(b), 100 * salto, juicio))
    print()
    print("   Los %d pares que no figuran no tienen transitorio ninguno."
          % (len(list(itertools.combinations(red.PINES, 2))) - len(filas)))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-i", "--entrada",
                    help="planilla a leer (.xlsx o .csv); por defecto la de "
                         "outputs/debug_analogico")
    ap.add_argument("--portadora", action="store_true",
                    help="la placa es la JitX, no la del TopDesign")
    ap.add_argument("--tol", type=float, default=TOL_DIAG * 100,
                    help="tolerancia de comparacion en %%%% (default %g)"
                         % (TOL_DIAG * 100))
    ap.add_argument("--md", action="store_true",
                    help="ademas del informe en pantalla, escribirlo a disco")
    ap.add_argument("--excel", action="store_true",
                    help="escribir las hojas Diagnostico y Remedir dentro de "
                         "la misma planilla (no toca lo anotado)")
    ap.add_argument("--transitorios", action="store_true",
                    help="de que pares hay que desconfiar y cuanto esperarles")
    ap.add_argument("--autotest", action="store_true",
                    help="probar el diagnostico con fallas inventadas")
    ap.add_argument("-v", "--verboso", action="store_true")
    args = ap.parse_args()

    if args.portadora:
        red.aplicar_portadora()

    if args.transitorios:
        tabla_transitorios()
        return 0

    if args.autotest:
        print("\nAUTOTEST: se simula una placa con una falla conocida y se")
        print("comprueba que el diagnostico la encuentre.\n")
        return 1 if autotest(args.verboso) else 0

    if args.entrada:
        ruta = Path(args.entrada)
    else:
        ruta = red.dir_salida("debug_analogico") / "medidas_analogicas.xlsx"
    if not ruta.exists():
        print("no existe %s; correr primero red_analogica.py" % ruta,
              file=sys.stderr)
        return 2

    med, medc, directas = (leer_csv(ruta) if ruta.suffix.lower() == ".csv"
                           else leer_xlsx(ruta))
    texto = informe(med, medc, tol=args.tol / 100.0, directas=directas)
    print()
    print(texto)
    print()

    if args.md:
        destino = red.dir_salida("debug_analogico") / "diagnostico.txt"
        destino.write_text(texto + "\n", encoding="utf-8")
        print("informe escrito en %s" % destino, file=sys.stderr)

    if args.excel:
        if ruta.suffix.lower() != ".xlsx":
            print("--excel necesita la planilla .xlsx", file=sys.stderr)
            return 2
        aj = ajustar_red(med, directas=directas)
        n = escribir_en_planilla(ruta, texto, aj,
                                 pendientes_de_remedir(aj, med))
        print("hojas Diagnostico y Remedir escritas en %s (%d filas a "
              "remedir)" % (ruta, n), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
