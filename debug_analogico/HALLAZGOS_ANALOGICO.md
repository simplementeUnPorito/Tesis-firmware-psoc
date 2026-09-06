# Verificación con óhmetro de la parte analógica — hallazgos

Campaña de medición del 2026-09-02 sobre la placa construida (topología del
TopDesign, no la portadora JitX). Instrumento: multímetro ET-1505.

Las mediciones crudas están acá porque `outputs/` está en `.gitignore`: la
planilla `outputs/debug_analogico/medidas_analogicas.xlsx` **no se versiona**
y este documento es el único registro durable.

Herramientas: `red_analogica.py` (modelo de la red y planilla), `planilla.py`
(Excel), `diagnostico.py` (análisis). Ver `README.md`.

## Resumen

**No apareció ningún componente fallado.** Lo que apareció fue:

| # | Hallazgo | Naturaleza | Acción |
|---|---|---|---|
| 1 | Junta fría en el pad de R11 del lado de `Vref` | falla física real | **resoldar** |
| 2 | R2 y R3 son de 51 kΩ, no de 47 kΩ | error del modelo | corregido en `VALORES` |
| 3 | RV1 no existe: hay un fijo de 680 Ω | error del modelo | corregido (`RV1_FIJO`) |
| 4 | C1 (680 µF) tiene fuga, ~1 MΩ equivalente | real, **inocua** | ninguna, ver §4 |
| 5 | Las lecturas in-circuit tardan >8 min en asentar | método | ver §5 |

Los cinco valores medidos que no cerraban se explican con (2), (3) y (4)
juntos, sin necesidad de que ningún resistor esté fuera de tolerancia.

## 1. Junta fría en R11

Tres pasadas sobre `P3[6] Vref – P3[0] Vref_PGA`: **440 k / 14,91 k / 14,7 k**.

Ningún componente hace eso. Y el signo lo delata: un transitorio capacitivo
sólo puede leer *de menos* —el capacitor arranca como un corto y se va
abriendo—, así que una lectura que empieza **alta** y después baja no es un
capacitor, es resistencia de contacto. Inspección visual posterior: el pad de
R11 del lado que va a `Vref` estaba abierto.

R11 en sí está sana: reconstruida en 14,76 kΩ sobre seis lecturas.

## 2. R2 y R3 son de 51 kΩ

El diseño pedía 50 k. `VALORES` decía 47 k con la nota *"montada 47k: no
había de 50k"*. Es al revés: son de **51 kΩ** (E24), confirmado por el cuerpo
del componente y por ocho lecturas coherentes entre sí.

La evidencia, con R11–R14 ≈ 14,9 k medidos:

| medición | da | implica |
|---|---|---|
| `INp–Vref_PGA` / `_BP` / `_ADDER` / `_LP` | 65,5 k | R2 = 65,5 − 14,9 = **50,6 k** |
| `INn–Vref_*` (las cuatro) | 65,7 k | R3 = **50,8 k** |
| `INp–INn` | 101,5 k | R2+R3 = **101,4 k** ✓ |

Las dos lecturas directas `INp–Vref` = 47,17 e `INn–Vref` = 47,47 quedan como
la única inconsistencia abierta; se tomaron con 1 min de espera, y en esta
placa eso no alcanza (§5).

**Impacto en el circuito: ninguno.** R2 y R3 sólo polarizan el geófono contra
`Vref`; quedan en paralelo con la bobina, y 102 k contra los cientos de ohm de
la bobina no aportan amortiguamiento apreciable. Un 8 % de diferencia mueve el
ruido térmico de entrada un 4 % en tensión. Es despreciable.

## 3. RV1 no existe

En la placa **no hay trimmer**: no se consiguió el preset y va un resistor
fijo de 680 Ω ±5 % en su lugar. El script lo modelaba como un potenciómetro
de 2 k con el cursor al 31,7 %.

Esto importaba más de lo que parece: `diagnostico.py` venía **despejando la
posición del cursor** de la medición `BPo–SUMm` y ajustando el modelo a ella.
Si no hay cursor, ese ajuste es un agujero por donde se cuela cualquier
problema real de esa rama. Corregido: con `RV1_FIJO` puesto, no se ajusta nada
y se dice explícito.

Consecuencia de diseño: **la ganancia del sumador quedó fija.** 680 Ω sobre
2 k es el 34 %, cerca del 31,7 % que tenía el TopDesign, así que el punto es
el previsto — pero se perdió el ajuste.

Medido: `BPo–SUMm` converge en **7,35 k**. Rango esperado con R7 = 6,8 k y los
680 Ω al 5 %: 7,38–7,58 k. Con la fuga de C1 (§4) el modelo predice 7,43 k.
Cierra.

## 4. C1 tiene fuga, y no importa

Cinco lecturas de la rama del pasabanda-sumador convergían todas un poco por
debajo del nominal. Una sola causa las explica a las cinco: una resistencia
de fuga en paralelo con C1, que **no desaparece al cargarse el capacitor**.

| medición | medido | sin fuga | con fuga de 1 MΩ |
|---|---|---|---|
| `BPm–BPo` | 45,00 k | 47,00 | **45,00** |
| `SEo–BPm` | 58,47 k | 61,28 | **57,88** |
| `BPo–SUMm` | 7,35 k | 7,48 | **7,43** |
| `SEo–SUMm` | ≥6,65 k | 6,80 | **6,76** |
| `SUMm–SUMo` | 26,66 k | 27,00 | **27,00** |

El ajuste por mínimos cuadrados sobre esas cinco da **1,23 MΩ** con cero
filas sin explicar; despejando sólo de `BPm–BPo` da 960 kΩ. O sea ~1 MΩ, que
a la tensión de prueba del óhmetro (medio voltio) son unos 0,5 µA de fuga:
un electrolítico de 680 µF perfectamente normal, no uno degradado.

### Por qué no hay que cambiarlo

La fuga queda en paralelo con C1, que está en serie con R4 = 43 k a la
entrada del pasabanda inversor (realimentación R5 = 47 k). La transferencia
de la etapa, con y sin fuga:

```
 f [Hz]     con fuga     sin fuga     error
   0.01      0.95079      0.96002   -0.962%
    0.1      1.09127      1.09141   -0.013%
      1      1.09301      1.09301   -0.000%
     10      1.09302      1.09302   -0.000%
     50      1.09302      1.09302   -0.000%
```

- polo introducido por la fuga: **0,234 mHz**
- cero (el corner útil, R4·C1): **5,68 mHz**
- banda de trabajo real: **10–50 Hz**

A 10 Hz la impedancia de C1 son **23,4 Ω** contra los 43 k de R4. Lo que se
le ponga en paralelo a algo de 23 Ω no cambia nada; para que 1 MΩ pese, C1
tendría que presentar una impedancia comparable, y eso recién pasa por debajo
de 0,23 mHz. Se está midiendo **1700 veces más arriba** del punto donde la
fuga empieza a notarse.

Lo único que hace es dar una ganancia de continua de 0,045 en vez de 0: si
`SEo` y la referencia del pasabanda difieren en 50 mV, aparecen 2,3 mV de
offset a la salida.

Además, el error de la estimación va en la dirección benigna: las lecturas de
las que sale ~1 MΩ podrían estar todavía un poco cortas, y si lo están la
fuga real es *menor* (más resistencia). La conclusión aguanta.

**Pendiente menor, por curiosidad:** limpiar con alcohol isopropílico
alrededor de C1 y repetir `BPm–BPo`. Si sube a 47 k era flux entre pistas; si
se queda en 45 k es el capacitor. A 10 Hz da igual en los dos casos.

## 5. El transitorio: por qué casi nada se puede medir rápido

Todo el transitorio de la placa sale de **un solo lazo**:
`SEo → R4 → C1 → BPm → R5 → BPo → R7+RV1 → SUMm → R6 → SEo`.

C1 no se carga a través de R4 sino a través del lazo entero:

```
R que ve C1 entre sus patas: 104.3 k  ->  tau = 71 s
   99% (4.6 tau) = 5.4 min   |   99.9% (6.9 tau) = 8.2 min
```

El README decía τ ≈ 17 s. Es **71 s**, cuatro veces más.

Salto de cada par entre t=0 (C1 descargado) y el valor final —o sea, cuánto
miente una lectura apurada. Se reproduce con `python diagnostico.py --transitorios`:

| par | en t=0 | final | salto | |
|---|---|---|---|---|
| `SEo – BPm` | 25,27 k | 61,28 k | **59 %** | inservible sin esperar |
| `BPm – SUMm` | 26,02 k | 54,48 k | **52 %** | inservible sin esperar |
| `BPm – BPo` | 25,82 k | 47,00 k | **45 %** | inservible sin esperar |
| `BPm – SUMo` | 53,02 k | 81,48 k | **35 %** | inservible sin esperar |
| `SEo – BPo` | 12,33 k | 14,28 k | 14 % | hay que esperar |
| `BPo – SUMm` | 6,94 k | 7,48 k | 7 % | hay que esperar |
| `SEo – SUMm` | 6,36 k | 6,80 k | 7 % | hay que esperar |
| `BPo – SUMo` | 33,94 k | 34,48 k | 2 % | casi no la toca |
| `SEo – SUMo` | 33,36 k | 33,80 k | 1 % | casi no la toca |
| `SUMm – SUMo` | 27,00 k | 27,00 k | 0 % | casi no la toca |

Los otros 143 pares no tienen transitorio ninguno.

**El lazo atraviesa el sumador, no se queda en el pasabanda.** `SEo–SUMm` y
`BPo–SUMm` están adentro; parecen estables porque el camino paralelo
(43+47+7,5 = 97,5 k) es enorme comparado con los 6,8 k que se miden, pero
mienten un 7 % — justo el orden de todo lo que hubo que perseguir. Las que
son genuinamente inmunes son `SUMm–SUMo` y **toda la rama del pasabajos**.

### El tiempo real es peor que τ

Observado en la mesa: dejando el óhmetro puesto, **la lectura sigue subiendo**
mucho después de los 8 min que predice una exponencial simple. Eso es
absorción dieléctrica (*soakage*): un electrolítico no tiene una constante de
tiempo sino una distribución, y la cola puede durar decenas de minutos.

Consecuencia práctica: **ninguna lectura in-circuit de ese lazo es una
medición; son cotas inferiores.** `diagnostico.py` las trata así — se anotan
con `>` en la planilla, no entran al ajuste como igualdad, y se contrastan
contra el nominal en la sección `COTAS CONTRA EL VALOR DE DISEÑO`. Las seis
disponibles son todas compatibles con el diseño.

Para medir de verdad esa rama hay que **levantar una pata de R4**: abre el
lazo, los diez pares pasan a 0 % de salto, y de paso deja medir R4, que desde
los pines es invisible porque C1 lo bloquea en continua. No se hizo.

## 6. Valor reconstruido de cada resistor

La red es un bosque: entre dos pines hay un solo camino, así que cada lectura
es la **suma** del camino. Con 34 lecturas y 15 resistores el sistema sobra, y
cada valor sale promediado sobre todas las filas que lo tocan. El ajuste es
NNLS —un resistor no puede dar negativo— con descarte iterativo de lecturas
groseras, después Huber, y las cotas entran con peso reducido.

| ref | nominal | reconstruido | desvío | error | lecturas | asentadas |
|---|---|---|---|---|---|---|
| R2 | 51,00k | 50,38k | −1,2 % | ±0,3 % | 6 | 6 |
| R3 | 51,00k | 50,65k | −0,7 % | ±0,3 % | 6 | 6 |
| R5 | 47,00k | 44,41k | −5,5 % | ±0,4 % | 3 | 2 |
| R6 | 6,80k | 6,93k | +1,9 % | ±2,1 % | 4 | 1 |
| R8 | 27,00k | 26,72k | −1,0 % | ±0,4 % | 3 | 1 |
| R9 | 12,00k | 12,24k | +2,0 % | ±5,1 % | 2 | 2 |
| R10 | 150,0k | 147,8k | −1,5 % | ±0,4 % | 2 | 2 |
| R11 | 15,00k | 14,76k | −1,6 % | ±0,4 % | 6 | 6 |
| R12 | 15,00k | 14,92k | −0,5 % | ±0,4 % | 6 | 6 |
| R13 | 15,00k | 14,87k | −0,9 % | ±0,4 % | 6 | 6 |
| R14 | 15,00k | 14,78k | −1,5 % | ±0,4 % | 6 | 6 |
| R15 | 30,00k | 29,04k | −3,2 % | ±2,0 % | 2 | 2 |
| R7+RV1 | 7,48k | 7,42k | −0,8 % | (grupo) | 4 | |
| R4 | 43,0k | — | | | 0 | invisible desde los pines |

Dispersión típica de las lecturas coherentes entre sí: **0,50 %**. Esa es la
vara real; por debajo de eso no hay nada que diagnosticar.

**La columna `error` importa.** R9 y R15 salen de restar números grandes
—R15 = (`PGAo–LPm` + `PGAo–LPo` − `LPm–LPo`)/2—, así que un 1,5 % en cada
lectura se amplifica a varios por ciento en el resultado. R9 llegó a marcar
−5,3 % con las lecturas viejas y +2,0 % con las nuevas: la diferencia fue un
único par que cambió 1,9 k. Con ±5,1 % de barra, R9 es compatible con 12 k sin
discusión, y R15 con 30 k. **Sin la barra de error, ese ruido se lee como un
componente fuera de tolerancia** — y así se persiguió a R9 un buen rato.

El −5,5 % de R5 es el efecto de la fuga de C1 (§4), no un componente malo.

## 7. Capacitancia: sin datos utilizables

El capacímetro disponible no llega: marca `APO` / fuera de rango en pF y nF, y
en µF da valores no creíbles (32 µF donde van 1,1 µF; 10–13 µF en las tres
ramas de referencia, que son idénticas entre sí). **No se concluye nada de los
capacitores por esta vía**, y ninguno se dio por malo.

Lo único confirmado es por marcado: **C3 = 15 nF**, código `153`.

C1 (680 µF) y C4 (47 nF) tienen además una pata en un nodo interno: no hay dos
pines entre los cuales medirlos. Todo esto lo cierra un barrido de respuesta en
frecuencia con la placa encendida, no el óhmetro.

## 8. Lo que quedó sin hacer

1. **Resoldar el pad de R11** (§1). Es la única acción correctiva pendiente.
2. **Levantar una pata de R4** y medir la rama del pasabanda de corrido (§5).
   Cierra R4, R5 y R6, que hoy sólo tienen cotas.
3. **Repetir `INp–Vref` e `INn–Vref` con 5+ min de espera** (§2). Es la única
   contradicción abierta.
4. **Los 119 pares que deben dar abierto**, contra GND y +5 V incluidos. No se
   midió ninguno. Son las únicas medidas que detectan puentes de estaño, y son
   instantáneas.
5. **Barrido en frecuencia con la placa encendida** para todo lo capacitivo.

## 9. Anomalía sin cerrar

`BPm–SUMo` = **87,5 k** contra 81,48 k de diseño. Es la única lectura que
*supera* el nominal, y un transitorio capacitivo no puede hacer eso: sólo lee
de menos. La explicación más probable es C1 cargado de la medición anterior
inyectando tensión contra el óhmetro. Se resuelve descargando C1 (puentear
`BPm` con `SEo` unos segundos) y midiendo en las dos polaridades: si el
resultado cambia al invertir las puntas, era la carga.

## 10. Mediciones crudas

Todo lo que se anotó, tal cual. La planilla de la que salen no se
versiona (`outputs/` está en `.gitignore`), así que esta es la copia
durable.

### Tanda 1 — planilla original, espera corta (2–140 s)

Todas estas quedaron bajo sospecha al descubrirse el tiempo de
asentamiento real (§5). Las del lazo de C1 son cotas inferiores.

| par | esperado k | 1a lectura | espera | medido k | nota |
|---|---|---|---|---|---|
| P1[2] INp - P1[7] INn | 102 | 90 | 10 s | **100.3** |  |
| P1[2] INp - P3[6] Vref | 51 | 38 | 10 s | **47.47** |  |
| P1[2] INp - P3[0] Vref_PGA | 66 | 62 | 15 s | **489** |  |
| P1[2] INp - P3[1] Vref_BP | 66 | 22 | 10 s | **60.52** |  |
| P1[2] INp - P0[6] Vref_ADDER | 66 | 65.6 | 2 s | **65.6** |  |
| P1[2] INp - P0[7] Vref_LP | 66 | 62.4 | 5 s | **65.5** |  |
| P1[7] INn - P3[6] Vref | 51 | 37 | 5 s | **47.56** |  |
| P1[7] INn - P3[0] Vref_PGA | 66 | 7000 | 5 s | **8560** |  |
| P1[7] INn - P3[1] Vref_BP | 66 | 29 | 5 s | **60.79** |  |
| P1[7] INn - P0[6] Vref_ADDER | 66 | 62.7 | 5 s | **65.9** |  |
| P1[7] INn - P0[7] Vref_LP | 66 | 62.7 | 5 s | **65.9** |  |
| P3[6] Vref - P3[0] Vref_PGA | 15 | 15 | 10 s | **431.3** |  |
| P3[6] Vref - P3[1] Vref_BP | 15 | 11.49 | 5 s | **14.96** |  |
| P3[6] Vref - P0[6] Vref_ADDER | 15 | 12.72 | 5 s | **14.88** |  |
| P3[6] Vref - P0[7] Vref_LP | 15 | 12.07 | 5 s | **14.79** |  |
| P2[7] SEo - P3[2] BPm | 61.28 | 25 | 90 s | **55.12** |  |
| P2[7] SEo - P3[7] BPo | 14.28 | 14.89 | 60 s | **14.37** |  |
| P2[7] SEo - P0[5] SUMm | 6.8 | 6.98 | 70 s | **6.86** |  |
| P2[7] SEo - P0[0] SUMo | 33.8 | 33 | 80 s | **33.7** |  |
| P3[2] BPm - P3[7] BPo | 47 | 22.8 | 140 s | **41.21** |  |
| P3[2] BPm - P0[5] SUMm | 54.48 | 38.48 | 110 s | **47.3** |  |
| P3[2] BPm - P0[0] SUMo | 81.48 | 600 | 120 s | **85** |  |
| P3[7] BPo - P0[5] SUMm | 7.48 | 7.23 | 60 s | **7.34** |  |
| P3[7] BPo - P0[0] SUMo | 34.48 | 33 | 60 s | **33.7** |  |
| P0[5] SUMm - P0[0] SUMo | 27 | 25 | 20 s | **26.36** |  |
| P2[6] PGAo - P0[3] LPm | 42 | 40 | 10 s | **41.23** |  |
| P2[6] PGAo - P0[1] LPo | 180 | 175 | 10 s | **176.6** |  |
| P0[3] LPm - P0[1] LPo | 162 | 120 | 10 s | **158.1** |  |
| P3[0] Vref_PGA - P3[1] Vref_BP | 30 | 50 | 10 s | **2312** |  |
| P3[0] Vref_PGA - P0[6] Vref_ADDER | 30 | 15 | 10 s | **14.25** |  |
| P3[0] Vref_PGA - P0[7] Vref_LP | 30 | 20 | 10 s | **12.86** |  |
| P3[1] Vref_BP - P0[6] Vref_ADDER | 30 | 22 | 10 s | **29.77** |  |
| P3[1] Vref_BP - P0[7] Vref_LP | 30 | 18 | 10 s | **29.6** |  |
| P0[6] Vref_ADDER - P0[7] Vref_LP | 30 | 22 | 10 s | **29.61** |  |

### Tanda 2 — repeticiones con espera larga

`>x` significa «al menos x, seguía subiendo». Las lecturas van en el
orden en que se tomaron.

| par | tiene que dar | lect. 1 | lect. 2 | lect. 3 | nota |
|---|---|---|---|---|---|
| P1[2] INp - P1[7] INn |  | 22 | 101.8 | 101.2 | idem |
| P1[2] INp - P3[0] Vref_PGA |  | 65.3 | 65.7 | 65.6 |  |
| P1[2] INp - P3[1] Vref_BP |  | 65.6 | 65.6 | 65.2 | hay que esperar 1min para que cargue |
| P1[7] INn - P3[0] Vref_PGA |  | 65.5 | 65.7 | 65.7 |  |
| P1[7] INn - P3[1] Vref_BP |  | 62 | 65.5 | 66 | el primero es instantaneo, los otros rotando las puntas y esperando |
| P3[6] Vref - P3[0] Vref_PGA |  | 440 | 14.91 | 14.7 | ya pille, esta abierto el circuito en el pin de la resistencia uno de los lados, el que va a Vref |
| P3[0] Vref_PGA - P3[1] Vref_BP |  | 29.44 | 29.62 | 29.62 |  |
| P3[0] Vref_PGA - P0[7] Vref_LP |  | 27 | 29.47 | 29.47 | primera puse el valor instantaneo, los otros tuve que esperar como 30s |
| P3[0] Vref_PGA - P0[6] Vref_ADDER |  | 22 | 29.48 | 29.5 | idem |
| P0[3] LPm - P0[1] LPo |  | 160 |  |  | estable |
| P0[5] SUMm - P0[0] SUMo |  | 26.66 |  |  | clavado, sin transitorio |
| P3[2] BPm - P0[0] SUMo | 81.48k | 51.76 | 87.3 | 87.7 | primera puse el valor instantaneo, los otros tuve que esperar como 2min |
| P1[2] INp - P3[6] Vref | 51.00k | 22 | 47.1 | 47.24 | idem |
| P1[7] INn - P3[6] Vref | 51.00k | 12 | 47.41 | 47.53 | lectura 1 es primero, lec2 y 3 cargado 1min |
| P2[7] SEo - P3[2] BPm | 61.28k | 26 | 58 | 58.47 | estabilizo en 58.47 a los 6 min (P2[7] contra P3[2]); a los 3 min iba en 58 |
| P2[7] SEo - P0[5] SUMm | 6.80k | >6.6 | >6.65 |  | a 1 min iba 6.6 y seguia subiendo; la solte en 6.65 |
| P3[2] BPm - P3[7] BPo | 47.00k | 32 | 44.6 | 45 | se planto en 45k; arranco en 32k (C1 venia cargado en parte) |
| P2[6] PGAo - P0[3] LPm | 42.00k | 41.28 |  |  | estable |
| P2[6] PGAo - P0[1] LPo | 180.0k | 176.8 |  |  | estable |
| P3[7] BPo - P0[5] SUMm | 7.48k | 6 | >7.2 |  | arranca en 6k, va por 7.2 y sube muy lento |

### Capacitancia

| par | componentes | esperado | medido | nota |
|---|---|---|---|---|
| P3[6] Vref - GND | C6 + C7 | 1.1 uF | 32 | microfaradios |
| P3[2] BPm - P3[7] BPo | C2 + C17 | 177 pF | no | no sale la medicion sale APO sale por encima de rango micro |
| P0[5] SUMm - P0[0] SUMo | C3 | 15 nF | no | no sale la medicion sale APO sale por encima de rango micro |
| P0[3] LPm - P0[1] LPo | C5 | 3.3 nF | no | sale por encima de rango nano |
| P3[0] Vref_PGA - GND | C8 + C9 | 1.1 uF | 10.16 | esta bien usé todo de 10 hz generarico con el de 100nF en paralelo eso no importa tanto que sea exacto |
| P3[1] Vref_BP - GND | C10 + C11 | 1.1 uF | 12.14 | lo mismo |
| P0[6] Vref_ADDER - GND | C12 + C13 | 1.1 uF | 12.06 | lo mismo |
| P0[7] Vref_LP - GND | C14 + C15 | 1.1 uF | no | no lee nada pone .y en nano |
| P15[3] AMuxCapacitor - GND | C16 | 100 nF | 0.03 | pone en nano |

---

Para reproducir el análisis: `python diagnostico.py` sobre la planilla,
`--transitorios` para la tabla de §5, `--autotest` para verificar la
herramienta contra fallas simuladas (10/10).
