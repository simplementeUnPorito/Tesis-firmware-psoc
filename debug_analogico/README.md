# Verificación con óhmetro de la parte analógica

`red_analogica.py` describe la red externa del PSoC y calcula la resistencia
esperada entre todos los pares de pines analógicos, incluyendo GND y +5 V.

```
python red_analogica.py --tabla       # planilla de medicion en pantalla
python red_analogica.py --red         # dibujo de la red modelada
python red_analogica.py               # escribe medidas_analogicas.csv
python red_analogica.py --portadora   # cualquiera de los tres, para la placa JitX
```

Por defecto modela la topología del TopDesign, que es la que está construida:
las cuatro ramas de referencia son de 15 k y van todas al mismo nodo `Vref`
(P3[6]).

`--tabla` imprime dos grupos y ningún par abierto:

- **Resistencia**, 34 filas, con el rango al 1 %. Un `*` marca los doce
  chequeos originales, por si querés hacer sólo esos. Un `~` marca las cuatro
  filas que pasan por el trimmer RV1, donde el centro real es el que tenga
  puesto el cursor. La columna `arranca` es lo que vas a leer al apoyar las
  puntas antes de que se cargue el 680 µF.
- **Capacitancia**, 9 filas, con el rango al 20 %. La columna `en paralelo`
  dice si hay una resistencia que arruina la lectura: seis pares son limpios
  (los capacitores contra masa) y tres son dudosos.

`--red` dibuja la topología con los operacionales y después lista los nodos con
su pin y el inventario de componentes con sus dos extremos. Es lo que se
contrasta contra el TopDesign para verificar que el modelo esté bien. Cada
componente se sustituye como un token de ancho fijo, así que el dibujo no se
desalinea al cambiar un valor, y hay un chequeo que avisa si algún componente
del modelo no aparece dibujado.

## La planilla se anota sola y no se pisa

Sin flags escribe `outputs/debug_analogico/medidas_analogicas.xlsx`, sin
importar desde qué directorio lo corras: el helper `scripts/shared/rutas.py`
resuelve la raíz del superproyecto. Con `--csv` sale el CSV plano.

El Excel tiene tres hojas:

- **Antes de medir** — cómo se usa y las tres trampas de medición.
- **Resistencia** — 153 pares. Un `*` marca los doce chequeos base.
- **Capacitancia** — 9 pares, cada uno con su unidad (pF, nF o µF).

Sólo se completan **las celdas amarillas**. La columna `ok` es una fórmula:
compara la medición contra el rango de tolerancia y se pinta verde si entra,
roja si no. En las filas que deben dar abierto, un número la pone en `CORTO?`.
No hay que comparar nada a mano.

Se anotan tres números por fila, no uno: **`1a lectura`**, **`espera s`** y
**`medido`**. Eso deja registrado el transitorio completo, así una medida rara
se puede reconstruir después sin volver a la mesa: si la primera lectura
coincide con `arranca kohm`, el circuito está bien y lo que faltó fue
paciencia; si **no** coincide, ahí sí hay algo distinto de lo que dice el
modelo. Por eso, cuando una medida cae fuera de rango pero se esperó menos de
60 s en una fila con transitorio, `ok` dice `ESPERAR` en ámbar en vez de
`FUERA` en rojo.

Las resistencias se cargan en kΩ y los capacitores en la unidad que dice su
fila, así no hay que escribir `0.000177`.

Cuando el multímetro no cierra va **`OL`**, no la celda vacía: vacío significa
"todavía no lo medí" y `OL` significa "lo medí y está abierto". `diagnostico.py`
necesita esa diferencia, y en las 119 filas que deben dar abierto es la única
manera de darlas por verificadas. Las fórmulas de `ok` ya lo contemplan: `OL`
en una fila abierta se pinta verde, y en una fila con rango nunca se pinta
verde (Excel toma cualquier texto como mayor que cualquier número).

**Regenerar no borra lo anotado.** Antes de sobrescribir, el script relee el
archivo y arrastra las celdas amarillas. Podés corregir un valor en `VALORES`,
regenerar, y conservar todo lo medido; te avisa cuántas filas preservó. Las
anotaciones se indexan por hoja + par, porque hay pares que aparecen en las
dos: `P3[2] BPm - P3[7] BPo` es una fila de resistencia (47 k) y otra de
capacitancia (177 pF).

`planilla.py` es el módulo que arma el Excel; no se ejecuta solo.

Usar fuente monoespaciada: los dos modos de pantalla se desarman si no.

## Qué se encontró midiendo

`HALLAZGOS_ANALOGICO.md` tiene la campaña del 2026-09-02 entera: los cinco
hallazgos, los cálculos, y **todas las mediciones crudas**. Están ahí y no en
la planilla porque `outputs/` está en `.gitignore`: el Excel no se versiona y
ese documento es el único registro durable.

Resumen: ningún componente fallado. Una junta fría en R11, dos errores del
modelo que ya se corrigieron (R2/R3 son de 51 k, y RV1 no existe: hay un fijo
de 680 Ω), y C1 con fuga de ~1 MΩ que resulta inocua a 10–50 Hz. Además, el
tiempo de asentamiento real de las lecturas in-circuit resultó ser de más de
8 minutos, no los 60 s que suponía este README: ver §5 de ese documento antes
de volver a medir.

## Cuando algo no da: `diagnostico.py`

```
python diagnostico.py            # lee la planilla y dice que componente es
python diagnostico.py --excel    # ademas escribe las hojas Diagnostico y Remedir
python diagnostico.py --md       # ademas deja el informe en outputs/
python diagnostico.py --autotest # se verifica solo, sin placa
```

Hace dos cosas distintas, en este orden.

**Primero reconstruye el valor de cada resistor.** La red es un bosque: entre
dos pines hay un solo camino, así que cada lectura es la *suma* del camino.
Con 34 lecturas y 15 resistores el sistema sobra, y sale el valor de cada
componente promediado sobre todas las filas que lo tocan y no sobre la única
que lo mide directo. El ajuste es NNLS —un resistor no puede dar negativo—
con descarte iterativo de lecturas groseras y después Huber, para que una fila
con 5 % de dispersión no arrastre a un componente que cierra al 0,3 %.

De ahí sale lo más útil: **las lecturas que se contradicen entre sí**. Ninguna
red pasiva puede dar A–B más B–C distinto de A–C; si eso pasa no hay falla de
circuito que lo explique, es contacto malo, punta movida o una lectura anotada
antes de que terminara el transitorio. Y sale la dispersión propia de los
datos, que es la vara real contra la cual juzgar un desvío: por debajo de eso
no hay nada que diagnosticar.

**Después busca la falla**, y ahí no tiene reglas escritas a mano del tipo
"si `INp`–`INn` da bajo entonces R2".
Toma el mismo modelo de red, **le inyecta una falla por vez** —un resistor
abierto, en corto o con otro valor; un pin sin soldar; un puente de estaño
entre dos pines cualesquiera; un capacitor en corto; una resistencia
desconocida en paralelo—, vuelve a resolver los 153 pares y se queda con las
hipótesis que reproducen lo que dice el multímetro. De ahí salen cuatro cosas
que una lista de reglas no da (si hay contradicciones sin resolver lo avisa y
no le des bola a esta parte: el buscador supone que todas las lecturas son
ciertas a la vez, y ahí no pueden serlo):

- **Distingue lo que no se puede distinguir.** `R5` abierto y el pin `BPm` sin
  soldar predicen exactamente lo mismo en los 153 pares, porque C1 bloquea la
  otra pata de `BPm` en continua. El informe las agrupa en vez de acusar a una
  de las dos y avisar de más.
- **Anda con la planilla a medio llenar**, que es como va a estar mientras
  medís. Usa las filas que haya.
- **Dice qué medir después**: cuando quedan varias hipótesis vivas, busca el
  par sin medir cuya lectura más las separa, y muestra qué daría bajo cada
  una. Con dos hipótesis en pie suele alcanzar una sola medida más.
- **Ajusta lo que no se conoce en vez de acusarlo.** RV1 no tiene un valor
  correcto —vale lo que tenga puesto el cursor—, así que lo despeja de
  `BPo`–`SUMm`; si esa fila se anotó a medio transitorio se declara el cursor
  desconocido en lugar de ajustar el modelo a una lectura mala.

Las tres trampas de la sección de abajo están cubiertas: el geófono enchufado
en J4 aparece como *"lo que se lee es la bobina, 375 ohm"* y no como `R2` y
`R3` en corto; las filas anotadas antes de que cargue C1 se listan aparte como
`FALTA ESPERAR` y no cuentan como falla; y las lecturas bajas contra masa las
explica como capacitor en corto o puente de estaño.

En capacitancia hace algo más fino donde hay dos en paralelo: si `BPm`–`BPo`
mide 27 pF en vez de 177 pF, dice que *es justo la suma sin C17*.

`--excel` deja dentro de la misma planilla dos hojas nuevas: **Diagnostico**,
con el informe completo, y **Remedir**, con la lista concreta de lo que hay que
volver a medir —cada fila con el motivo, cómo hacerla, y la celda amarilla para
anotar—. Incluye los chequeos de corto contra GND y +5 V de los pines que
aparecen en varias contradicciones, y las medidas sobre los pads propios de los
componentes que no cierran. No toca ninguna celda anotada; se pierden al
regenerar con `red_analogica.py`, que es lo correcto.

Son dos hojas y no una: en **Remedir** queda sólo lo que falta, y apenas una
fila se completa pasa a **Medido**, que es el registro de lo hecho. Así la
hoja de la mesa no se mezcla con lo ya resuelto. Las dos se leen igual, y
`red_analogica.py` ahora las preserva al regenerar la planilla (antes las
borraba, porque reconstruye el libro entero).

Esa hoja **se lee de vuelta**: es la tanda nueva, hecha a propósito, así que
pisa a la original. Tres columnas de lectura por fila, para las que hay que
repetir o invertir polaridad, y ahí el orden importa: se usa **la meseta del
final**, no la mediana. Un transitorio capacitivo siempre *sube* —el capacitor
arranca como un corto y se va abriendo—, así que una lectura que arranca alta
y después baja no la hace ningún capacitor: es resistencia de contacto, y se
reporta como tal. Si las dos últimas no coinciden no hubo meseta y el contacto
es intermitente. Las filas `pads de Rx` entran al ajuste
como una ecuación de un solo término: es la medida más fuerte que hay, porque
no depende del modelo. El ciclo es completar la hoja y volver a correr
`python diagnostico.py --excel`, que rehace el diagnóstico y la lista.

`--autotest` es la única forma de saber que esto funciona sin una placa
fallada delante: simula diez placas con una falla conocida cada una —incluida
una sana— y verifica que el diagnóstico la encuentre.

## Cómo usarlo

Los valores están todos en el diccionario `VALORES` al principio del script.
Si en la implementación cambió una resistencia, se corrige ahí y se regenera:
un cambio en `R6`, por ejemplo, mueve cinco filas a la vez. Las columnas
`medido_ohm`, `ok` y `esperado_corregido` quedan vacías para completar a mano
durante la medición.

`RV1_FRACCION` es la posición del trimmer guardada en el TopDesign (31.7 %),
no una garantía física. Conviene medir `BPo`–`SUMm`, restarle `R7` y poner la
fracción real antes de tomar en serio las cuatro filas que dependen de RV1.

## Tres cosas que hacen leer mal

**El geófono queda en paralelo.** J4 va directo entre `INp` e `INn`, en
paralelo con los 94 k. Con el geófono enchufado se lee la resistencia de
bobina, no 94 k. Vale la pena medir las dos veces: con geófono ≈ R_bobina
valida conector y cable; sin geófono debe dar 94 k.

**El electrolítico de 680 µF.** `C1` ofrece un camino paralelo que conduce
mientras se carga con la propia corriente del óhmetro, y se abre después.
τ ≈ 17 s, así que la lectura *trepa* durante uno o dos minutos. La columna
`lectura_inicial` del CSV dice desde dónde arranca cada par afectado. Anotar
el primer número que muestra el multímetro hace aparecer tres fallas
inexistentes.

**Todo lo que va contra masa da abierto.** Los cuatro `Vref_XX`, `Vref` y el
`AMuxCapacitor` sólo llegan a Vss por capacitores (1 µF + 100 nF, y 100 nF en
el AMux). Cualquier lectura baja contra GND es un corto, no una rama de
polarización. Vale también para +5 V.

## Qué placa tengo delante

El default es el TopDesign porque es lo que está armado: la portadora JitX
tiene el `.kicad_pcb` vacío, no se fabricó. Si algún día se fabrica, las
ramas de referencia son la única diferencia que se ve con el óhmetro (ver
`../TOPDESIGN_TO_KICAD.md`, §Divergencias), y una sola medida las distingue:

| `Vref_PGA` (P3[0]) contra `Vref` (P3[6]) | Placa | Flag |
|---|---|---|
| 15 k | la actual, según el TopDesign | (default) |
| abierto | portadora JitX (30 k hacia `VREF_2V048`) | `--portadora` |

## Lo que esta tabla no cubre

Ningún capacitor: 680 µF, 27 pF, 150 pF, 15 nF, 47 nF, 3.3 nF, los 1 µF/100 nF
de cada rama y el 100 nF del AMux. Tampoco `R4` (43 k), que queda invisible
desde los pines porque `C1` lo bloquea en continua — hay que medirlo en sus
propios pads. Todo eso lo cierra un barrido de respuesta en frecuencia con la
placa encendida, no el óhmetro.
