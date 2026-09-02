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

**Regenerar no borra lo anotado.** Antes de sobrescribir, el script relee el
archivo y arrastra las celdas amarillas. Podés corregir un valor en `VALORES`,
regenerar, y conservar todo lo medido; te avisa cuántas filas preservó. Las
anotaciones se indexan por hoja + par, porque hay pares que aparecen en las
dos: `P3[2] BPm - P3[7] BPo` es una fila de resistencia (47 k) y otra de
capacitancia (177 pF).

`planilla.py` es el módulo que arma el Excel; no se ejecuta solo.

Usar fuente monoespaciada: los dos modos de pantalla se desarman si no.

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
paralelo con los 100 k. Con el geófono enchufado se lee la resistencia de
bobina, no 100 k. Vale la pena medir las dos veces: con geófono ≈ R_bobina
valida conector y cable; sin geófono debe dar 100 k.

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
