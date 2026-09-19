# Control de calibracion unificado

Este es el unico proyecto PSoC activo. El mismo binario Release contiene la
captura, el aprendizaje lento y el PI permanente; Debug activa las pruebas con
`PSOC_TEST=1`. Los antiguos proyectos `AcondicionamientoAnalogicoTest` y
`AcondicionamientoAnalogicoField5Test.cydsn` fueron absorbidos y retirados.

## Secuencia de operacion

1. Al encender se validan dos registros EEPROM alternados (version y CRC).
2. Si no existe perfil compatible, el estado queda `FALTA_APRENDER`; no se
   aprende automaticamente.
3. `ctl learn` autoriza una medicion lenta de IDAC0/IDAC1 en las ganancias
   nominales. Solo se guarda un resultado estable y valido.
4. Fuera de captura, IDAC2/IDAC3 mantienen LPo con PI entero. Cada ~1,5 s se
   recorren SEo, BPo, OPA_SUMo, SUMo y LPo para diagnostico.
5. Al armar captura se congelan los cuatro IDAC, se detienen PI, aprendizaje,
   barrido y telemetria, se desconecta el capacitor y se carga
   `FIR_adquisition`. Despues del volcado se restaura `FIR_calibration` y el
   control continua.

El sexto canal del AMux no es una senal: conecta el capacitor de 100 nF durante
calibracion cuando el parametro 3 vale uno.

## Configuracion inicial

Las unidades del protocolo son enteras: microvoltios, milisegundos y codigos
IDAC. Los valores principales de fabrica son PGA x4 (codigo 2), PGAout x24
(codigo 5), promedio de 54 muestras filtradas, capacitor habilitado, periodo PI
de 3000 ms, tau de 9000 ms, Kp 1/4, barrido cada 1000 ms, reporte cada 5000 ms
y captura por LPo (canal 4). La lista completa y sus limites vive en
`control_config.h/.c`; no hay parametros operativos escondidos en `#define`.

### Ley del lazo rapido (v4, 2026-09-16)

Todas las tensiones estan en el dominio del ADC de control (`ctl_dc`), el unico
que el lazo observa; no son tensiones fisicas del tap. Medido en geo-01:

| magnitud | valor |
|---|---|
| ventana valida de lectura | -95..+98 mV |
| ruido de LPo / SUMo | +-3 / +-6 mV |
| IDAC3 sobre LPo | ~+0,25 mV/codigo (todo su recorrido: +-64 mV) |
| IDAC2 sobre LPo | ~-80 mV inmediato, ~-150 mV asentado |

Prioridades, en orden:

1. **Guarda de SUMo**: solo si SUMo esta en riel o pasa de +-60 mV, IDAC2 da un
   codigo y espera 45 s. La banda vieja de +-5 mV estaba dentro del ruido y
   hacia oscilar IDAC2 para siempre, tirando LPo de lado a lado cada 45 s.
2. **Rescate de LPo**: en riel, IDAC3 camina por signo de a 100 codigos cada
   10 s; si esta en tope, IDAC2 toma un codigo.
3. **Histeresis**: con |LPo| <= 6 mV el lazo se congela y no vuelve a actuar
   hasta que LPo sale de +-20 mV. La banda muerta vieja (100 mV) era mas ancha
   que la ventana valida, asi que el PI no corria nunca.
4. **Descarga a IDAC2**: solo con IDAC3 >= 240 codigos, el error empujandolo
   mas afuera y un error predicho menor. Un codigo de IDAC2 es mayor que todo
   IDAC3: sin esa prediccion, LPo en el hueco entre dos codigos de IDAC2 hacia
   bailar a IDAC2.
5. **PI** hacia cero con acumulador fraccional.

**Modo estable.** Tras 60 s congelado, el lazo solo despierta con 3 lecturas
seguidas (9 s) fuera de +-35 mV; un riel lo despierta siempre. Ademas el barrido
de diagnostico pasa de ~2 s a 20 s y el reporte de 5 s a 30 s: en el
osciloscopio (2026-09-16 16:34) los ticks remanentes eran el barrido del AMux
(espiga de 4 ms cada 2 s en SUMo/LPo) y la rafaga I2C del reporte (LPo +1,5 V
40 ms y cola de 0,3 s cada 5 s), no movimientos del PI. La bandera "en banda"
es ahora el veredicto del lazo (congelado), asi que solo cambia al congelarse
o despertar.

Medicion: mediana de 5 promedios por canal (la cadena muestra pozos de 40 ms y
-30 mV cada ~2 s) y 800 ms de lecturas descartadas despues de cada envio de
telemetria, porque cada rafaga I2C hacia el ESP golpea LPo +47/-54 mV durante
~0,5 s (traza del PSoC). `ctl get 255` vuelca las ultimas 127 lecturas crudas
de LPo con su tiempo para diagnosticar sin osciloscopio.

Retocar parametros del lazo rapido ya no invalida el perfil aprendido: el
perfil son IDAC0/IDAC1 y solo depende de ganancias, capacitor, pendientes
lentas y consigna del sumador. Las imagenes EEPROM v3 se migran al arrancar
conservando el perfil y tomando el lazo rapido de los valores de fabrica.

`control_config_valid()` rechaza rangos, canales, pendientes, tiempos y margenes
inconsistentes. Una modificacion incompatible invalida el perfil aprendido. El
guardado es explicito; el PI nunca escribe EEPROM durante sus iteraciones.

## Comandos desde el USB del esclavo

Los comandos son transportados por el ESP32 hacia el PSoC:

- `ctl report`: estado, configuracion, perfil, IDAC y DC de los cinco taps.
- `ctl get N` / `ctl set N VALOR`: leer o preparar un parametro.
- `ctl apply`, `ctl save`, `ctl defaults`: aplicar, persistir o restaurar.
- `ctl learn`: autorizar aprendizaje lento.
- `ctl pause` / `ctl resume`: congelar o reanudar control.
- `ctl channel N`: elegir SEo=0, BPo=1, OPA_SUMo=2, SUMo=3 o LPo=4.

Los cambios se aplican completos entre ciclos y se rechazan durante armado o
adquisicion. La captura no se bloquea por perfil ausente o banda incorrecta:
esas condiciones viajan como advertencia y metadatos para que decida el maestro.

## Verificacion

- Host: `gcc tests/control_test.c AcondicionamientoAnalogico.cydsn/control_config.c`.
- PSoC normal: `program_psoc.ps1 -Configuration Release`.
- PSoC pruebas: `program_psoc.ps1 -Configuration Debug -SelfTest`.
- ESP: `pio run -e slave2 -e slaveTest` desde el proyecto `slave`.

La captura espera el evento real `ARMED` antes de producir SYNC; nunca debe
inferir que el PSoC esta listo a partir de una demora fija.

## Lazo: hallazgos del 2026-09-17 (geo-01)

Toda la tarde el lazo rebotó entre los dos rieles de LPo y la cadena quedaba
saturada. Las causas, en orden de importancia:

1. **La escala de IDAC3 estaba mal por un factor grande.** El firmware usaba
   `FINE_SLOPE_UV` = 250 µV/código, medido con la etapa contra el riel. Medido
   con el lazo pausado y esperando 90 s por punto, alrededor del centro son
   **~15 mV/código**: IDAC3 0 → LPo 947 mV, 3 → 1008, 7 → 1052, 15 → 1083,
   31 y más → 1112 (riel). La ventana válida entera (±95 mV) son unos **13
   códigos**. Con pasos de 25 o 100 códigos, cualquier rescate cruzaba de un
   riel al otro y no aterrizaba nunca.
2. **La pendiente no es única**: cambia con el punto de trabajo y con la
   historia (la cola de ~34 s del acople de 680 µF). Estimándola de
   transiciones del propio lazo salían 0,8, 2,5 y 5,8 mV/código. Por eso el
   rescate ahora es una **bisección de signo**: camina en el sentido del signo
   y parte el paso al medio cada vez que cruza, sin depender de la pendiente.
   Probado en `tests/control_test.c` de 0,5 a 8 mV/código: entra en 2 a 17 pasos.
3. **El lazo corregía sobre su propio transitorio**: `TAU_MS` valía 9 s cuando
   la planta tiene ~34 s.
4. **Las bandas eran más angostas que la perturbación**: cada ráfaga de
   telemetría I2C mueve LPo decenas de mV, y con HOLD de 6 mV y DEADBAND de
   20 mV el lazo corregía el golpe y no la señal. Ahora HOLD 25 mV,
   DEADBAND 45 mV, banda de modo estable 60 mV con 5 lecturas seguidas y
   QUIET 1,5 s.
5. **Nada lo devolvía al centro**: congelado dentro de la banda, la deriva lo
   dejaba pegado a un borde. Se agregó el **recentrado lento** (`CP_RECENTER_UV`
   y `CP_RECENTER_MS`, v5 de la imagen NV): si lleva 5 min congelado con |LPo|
   pasado 15 mV, mueve UN código hacia el centro y se vuelve a congelar.
6. **Guardado automático del punto de trabajo**: `ctl_autosave()` en
   `control_service()` persiste los códigos cuando el lazo está en banda,
   congelado en modo estable, los códigos cambiaron y pasaron 10 minutos del
   último guardado. Así un arranque en frío parte del último punto bueno.

**Trampa de laboratorio, ya cara dos veces**: medir la pendiente con la etapa
en el riel subestima muchísimo. Centrar primero (bisección con el lazo
pausado), medir después.

**Falla de hardware del mismo día**: al re-sintetizar el TopDesign, el fitter
movió `I2Cp SDA` de P2[1] a P2[2] y el SPI de la SD. El PSoC quedó hablando
contra pines sin conectar: mudo, y la cadena sin lazo, clavada al riel. Los
pines correctos están en el esquemático y en `BUILD_PROGRAM_PSOC.md`; hay que
fijarlos (Lock) en el `.cydwr` después de cada re-síntesis.

**A vigilar del guardado automático**: `ctl_store()` escribe 16 filas de EEPROM
y eso bloquea el lazo principal unos cientos de ms, además de ser un consumo
extra. Solo corre con el PSoC en IDLE, en banda y congelado, y como mucho cada
10 minutos, pero si aparece un golpe en LPo con esa cadencia, el sospechoso es
ese. La telemetría emite la clave 0x109 con el número de guardados.

## Quién patea a LPo: radio vs SD (medido 2026-09-17, noche)

Dos experimentos con el lazo corriendo y sin capturar.

**Grueso** (lecturas de telemetría cada 10 s, 5 min por condición): reposo
−13/+98 mV con una lectura inválida; radio ESP-NOW a 4 Hz −8/+7 mV; `sdtest`
repetido −26/+44 mV. A esta resolución los golpes no se ven: lo que domina es
la deriva lenta.

**Fino** (traza del PSoC, 127 muestras de ~20 ms, `ctl get 255`, provocando el
evento justo antes):

| Condición | Desvío máximo de LPo | Sentido |
|---|---|---|
| Reposo | 5 a 11 mV | ruido de fondo |
| Ráfaga ESP-NOW de 1 s | 44 a 62 mV | siempre negativo |
| `sdtest` (escritura FatFs) | 69 a 76 mV | siempre positivo |

Conclusión: **los dos acoplan, con signos opuestos y tamaños parecidos**, entre
5 y 15 veces el ruido de reposo. Solos no cruzan la ventana de ±95 mV, pero
sumados a la deriva sí, y de ahí las idas al riel sin causa aparente. La
mediana de 5 y la ventana de silencio del lazo los filtran en régimen; lo que
no filtran es la deriva.

Para bajarlos hace falta trabajo de placa: desacoplar la alimentación del ESP
(los picos de corriente de TX) y la de la SD, y revisar por dónde vuelven esas
corrientes a masa.

## La meseta de LPo en las capturas (2026-09-18)

Síntoma: en `muestra_20260918_091907` la señal sube, se queda ~1,5 s en
1,1121 V y baja. Medido hoy sobre la placa, por COM8 (sin maestro):

| evidencia | valor |
|---|---|
| meseta en la captura | 1,1120 … 1,1122 V absolutos (p2p 0,1 mV) |
| ruido fuera de la meseta | ±15 mV p2p |
| `ctl_dc[4]` (LPo) durante 120 s | +111,66 … +111,81 mV, **constante**, `valid=0` |
| resto de los taps en esas 2 min | SEo −12, BPo −10, OPA −8, SUMo +12 mV, todos `valid=1` |
| techo de validez | `CP_VALID_HIGH_UV` 122.699 − `CP_MARGIN_UV` 25.000 = **+97,7 mV** |
| rescate | `CP_RESCUE_STEP`=2 códigos, `CP_RESCUE_MS`=60.000 ms, `CP_FINE_SLOPE_UV`=15.000 |

Lecturas:

1. La meseta **no es señal**: el ruido se derrumba 100× (±0,1 mV contra ±15 mV).
   Es el criterio que ya habíamos fijado — una etapa enganchada está quieta.
2. La meseta es exactamente el valor en el que queda clavada la propia medición
   de LPo del lazo (+111,7 mV), o sea que la captura y el lazo ven lo mismo por
   el mismo ADC. LPo queda estacionado apenas por encima de su techo de validez
   (+97,7 mV) y por eso el nodo repite `[CAL] SALI_DE_BANDA` y la web muestra
   el punto rojo: el veredicto es correcto.
3. **La bajada que se ve en la captura es el rescate, no la señal.** A 2 códigos
   de 15 mV cada 60 s el lazo devuelve 30 mV por minuto: una excursión de
   ~110 mV tarda tres o cuatro minutos en volver. Dentro de una captura de 3 s
   el lazo está congelado (`main.c:3567` saltea `control_service()` en
   ARMED/SAMPLING/dump), así que lo que se graba es la cola de una excursión que
   empezó antes de apretar START.

Pendiente (no medido todavía): **qué patea a LPo por encima del techo**. Un solo
dato apunta a la propia secuencia de captura — antes de un `rawcap 26` LPo estaba
en −20 mV y justo después en +112 mV. Falta repetirlo con telemetría a los dos
lados del ARM para poder afirmarlo.

Aviso de método: medir la autoridad de un IDAC con `idac 3 <código>` mientras el
lazo corre **no sirve**: `control_pi_step()` reescribe IDAC2/IDAC3 cada
`CP_PERIOD_MS` y pisa el valor manual en menos de un período. Hay que pausar el
control primero. Me llevó a leer una autoridad falsamente nula.

## El vernier no se descargaba en banda (2026-09-18)

Síntoma: «el nodo nuevo no calibra, satura como loco». Pero medido, el nodo
**estaba en banda**: `band=1`, `valid=11111`, LPo 16-23 mV durante 100 s. Lo que
no estaba sano era el actuador: **IDAC3 = 248 con `CP_FINE_MID` = 240**.

El descargue al IDAC grueso (paso 4 de `control_pi_step()`) vive *después* del
bloque de histéresis, y ese bloque hace `return` mientras el lazo está congelado
en banda. Entonces un nodo que entra en banda con el vernier contra su tope se
queda ahí: sostiene el punto de trabajo, pero le quedan ~7 códigos de autoridad
para un lado y 500 para el otro. La primera perturbación que pida más de eso lo
manda al riel, y desde el riel el rescate camina 2 códigos cada 60 s.

Arreglo: descargar el vernier **estando en banda**, sin despertar el lazo, y solo
si el movimiento bumpless predice quedarse dentro de `CP_HOLD_UV`; si no, se deja
como está, que no es peor que hoy. Dos casos nuevos en `tests/control_test.c`
(descarga cuando está pasado el umbral, no toca nada cuando está sano).

Verificado en placa tras regrabar: arranca en +113 mV, el lazo lo baja y **entra
en banda a los 177 s**; después queda con LPo = 8 mV, SUMo = 3 mV, IDAC3 = 232,
`valid=11111`, quieto durante 90 s. Captura de 60 lotes: `fill=60/60`,
`bOK=60 bBad=0`.

**La lección, que vale más que el parche:** un lazo *en banda* no es un lazo
sano. Hay que mirar dónde quedó parado el actuador, no solo la salida. La salida
estaba perfecta mientras el vernier agonizaba contra su tope.

Nota de método: `FIR_adquisition.h` tenía coeficientes nuevos pegados sin las
barras de continuación del macro y con una llave de más — el proyecto **no
compilaba** por eso, no por el control. Se repararon conservando los 128 taps
nuevos (verificado: 0..127 en orden).

## Forzar el lazo y recalibrar al cambiar de ganancia (2026-09-18)

Dos comandos nuevos, los dos solo en IDLE y con eco del valor pedido en el ack
(`0xEE` = rechazado):

| cmd | qué hace |
|---|---|
| `PSOC_CMD_FORCE_PI` (0xAB) | `p1` segundos de **forzado**: el lazo no se congela por histéresis y corrige a fondo. Es el mismo lazo permanente, sin freno. |
| `PSOC_CMD_VIEW_CHANNEL` (0xAC) | canal del AMux que se digitaliza (vista de diagnóstico). `0xFF` vuelve al normal. No se guarda en EEPROM. |

**Por qué el forzado no es la calibración legacy.** El botón viejo mandaba
`PSOC_CMD_CALIBRATE` (0xB5), que barre las referencias etapa por etapa y tarda
~15 s. Eso no es lo que hace falta cuando el punto de trabajo se corrió: el lazo
permanente ya sabe corregir, lo único que lo detiene es su propia histéresis.
`control_pi_force()` abre una ventana temporal en la que se saltea **solo** la
histéresis; la banda de validez, el rescate por bisección, el mid-ranging y el PI
siguen exactamente igual.

**Cambio de ganancia = recalibración automática.** `PGAgain_Set()` y
`PGAout_Set()` llaman ahora a `control_on_gain_change()` cuando el código
realmente cambia. Una ganancia no suma un offset: lo **multiplica**, así que lo
que estaba centrado queda contra el riel. Por eso ahí no alcanza con forzar:
además se borra el enfriamiento del actuador grueso (`ctl_pi.has_coarse=0`) para
que IDAC2 pueda moverse en el primer paso en vez de esperar los 45 s de
`CP_COARSE_MS`, se limpia el rescate y se tira la mediana vieja (son lecturas de
la ganancia anterior). La ventana es de 150 s ≈ 4,4 τ.

**Trampa que costó un diagnóstico:** el parser de tramas del PSoC tiene una
**lista blanca** de comandos (`main.c`, estado 1 del `rx_state`). Un comando que
no figura ahí se descarta **sin contestar**, así que el esclavo lo reporta como
`cfg ack timeout` y desde afuera parece que el PSoC lo ignoró. Hay que agregarlo
en tres lugares: la lista blanca, la rama de "ocupado calibrando" y el despacho.

## Barrido de ganancias: por qué el lazo no se recuperaba (2026-09-18)

Primera corrida, 200 s de observación por par, con el forzado automático ya
puesto pero con el rescate y el grueso a su ritmo normal:

| par | volvió a banda | LPo final | IDAC2 | IDAC3 | SUMo |
|---|---|---|---|---|---|
| x50/x1 | **nunca** | −263,7 mV | −9 | 225 | +103,3 |
| x8/x8 | **nunca** | −269,1 mV | −14 | 225 | +105,9 |
| x4/x24 | 57 s | +112,8 mV | −15 | 217 | −19,0 |
| x4/x8 | **nunca** | +112,9 mV | −15 | 211 | −19,3 |
| x16/x4 | 7 s | −273,9 mV | −19 | 215 | +101,0 |

Los −264/−274 mV y +113 mV **no son puntos de trabajo: son los dos rieles de la
medición** (el mismo par que ya estaba en el modelo de planta de
`tests/control_test.c`). Y el patrón es el mismo en todos los casos malos:
**IDAC3 entre 211 y 225 (su tope es 255) con IDAC2 casi sin moverse**. El vernier
agotado y el actuador grueso quieto.

Dos causas, las dos de ritmo, no de dirección:

1. **El rescate escalaba al grueso recién en el límite duro.** Con IDAC3 en 225
   seguía caminando de a `CP_RESCUE_STEP`=2 códigos cada `CP_RESCUE_MS`=60 s:
   llegar a 255 son 15 minutos antes de tocar IDAC2. Ahora escala en
   `CP_FINE_MID` (240).
2. **Los dos actuadores caminan demasiado lento para un cambio de ganancia.** La
   guardia de SUMo mueve un código cada `CP_COARSE_MS`=45 s, y con SUMo a
   ~100 mV fuera de su banda de 60 hacen falta una decena de códigos: siete
   minutos. Mientras esa guardia actúa hace `return`, así que el rescate de LPo
   ni corre. Ahora, **solo mientras se fuerza**, los dos usan
   `CTL_FORCED_STEP_MS` = 10 s; la bisección sigue partiendo el paso al medio en
   cada cambio de signo, así que ir más rápido no lo vuelve agresivo cerca del
   centro: deja de ser lento solo cuando está lejos.

**Cuidado con la columna "volvió a banda", que es mía y engaña.** El 7 s de
x16/x4 es un artefacto: a los 7 s LPo todavía marcaba +37 mV, que es la cola del
par anterior, y el script lo contó como banda antes de que la ganancia nueva
hiciera efecto. El único caso genuino es x4/x24 (−269 → −169 → +4 mV entre los
37 y los 67 s)… y de ahí siguió subiendo hasta clavarse en el riel de +113 mV.
O sea que cruzó la ventana entera de un riel al otro: "volvió a banda" y "quedó
bien" no son lo mismo, y el barrido hay que leerlo mirando la serie, no el
resumen. En la próxima corrida el criterio tiene que exigir además que se quede.

### Segunda corrida: el salto de riel a riel y de dónde sale

Con la escalada al grueso en `FINE_MID` y el paso de 10 s durante el forzado,
330 s por par:

| par | volvió a banda | LPo final | IDAC2 | IDAC3 | SUMo |
|---|---|---|---|---|---|
| x50/x1 | nunca | −264 (riel) | −32 | 225 | **+89,3** |
| x8/x8 | 107 s | +113,8 (riel) | −36 | 194 | −53,3 |
| x4/x24 | 312 s | +41,7 ✓ | −12 | 182 | −8,7 |
| x4/x8 | — | +26,9 ✓ | −12 | 165 | −4,2 |
| x16/x4 | nunca | −274 (riel) | −38 | 169 | **+100,7** |

La serie de x8/x8 es la que explica todo: **−269 mV → +6 mV a los 112 s → +113 mV
a los 127 s**, y clavado ahí los 200 s restantes. Eso no es deriva, es un
**salto**: cruzó la ventana entera en quince segundos.

De dónde sale el salto: el movimiento *bumpless* del grueso compensa con el
vernier usando `CP_COARSE_SLOPE_UV`, medido a **PGAout x24**. Pero **IDAC2
inyecta en el sumador, aguas arriba de PGAout**, así que su efecto real sobre LPo
escala con esa ganancia, mientras que IDAC3 actúa en el pasabajos, aguas abajo, y
no escala:

| PGAout | efecto real de un código de IDAC2 | lo que cree el lazo | patada neta |
|---|---|---|---|
| x24 | −150 mV | −150 mV | 0 |
| x8 | −50 mV | −150 mV | **+100 mV** |
| x1 | −6 mV | −150 mV | **+144 mV** |

Arreglo: `ctl_effective_config()` escala `CP_COARSE_SLOPE_UV` con la ganancia
vigente de PGAout antes de cada paso del PI. No toca la config guardada.

**Lo que el escalado NO arregla, y es un problema distinto:** x50/x1 y x16/x4 son
los dos de **PGA más alto**, y los dos tienen **SUMo en +89 y +101 mV**, fuera de
su banda de ±60. Ese offset es el de entrada multiplicado por el PGA, y aparece
en el sumador. El actuador que le corresponde no es IDAC2: medida su autoridad
sobre SUMo, **0,6 mV por código** (23 códigos movieron 14 mV), así que 100 mV
pedirían ~165 códigos. El actuador correcto es el **par lento IDAC0/IDAC1**, que
es lo que fija la fase de aprendizaje. Hoy un cambio de PGA invalida la
calibración (`g_last_calibration_ok = 0`) pero **no vuelve a aprender**.

Pendiente de decisión (no lo dejo andando solo porque tarda minutos): que un
cambio de PGA dispare `CONTROL_LEARN`. El de PGAout no lo necesita.

### El cambio de PGA necesita volver a aprender, y el paso del aprendizaje era inútil

Cuatro corridas del barrido dejaron el mismo cuadro para los pares de PGA alto
(x50/x1 y x16/x4, y también x8/x8 según de dónde venga): **SUMo queda en
+90…+106 mV, fuera de su ventana de medición de ±97,7**. Ahí la guardia de SUMo
de `control_pi_step()` se dispara y hace `return`, así que **el rescate de LPo
nunca corre**. El lazo queda trabado y no es cuestión de sintonía: probado con el
grueso rápido (cruzaba al riel opuesto) y con el grueso lento (no se movía
nunca).

El offset lo crea el PGA, que está **aguas arriba del sumador**, así que el
actuador que corresponde es el par lento IDAC0/IDAC1 — o sea el aprendizaje.
Hasta hoy un cambio de ganancia invalidaba la calibración
(`g_last_calibration_ok = 0`) pero **no volvía a aprender nunca**.

Ahora `control_on_gain_change(pga, pgaout, pga_changed)` distingue las dos
etapas: PGAout está aguas abajo y le alcanza con el forzado del PI; el PGA
dispara `CONTROL_LEARN`. Además la config sigue a la placa (`CP_PGA`/`CP_PGAOUT`),
que antes no pasaba: el perfil guardado mentía sobre con qué ganancia había sido
aprendido y un APPLY o un arranque en frío volvían a la vieja.

**Y el aprendizaje, solo, tampoco alcanzaba:** su paso era fijo en un código
(`CP_RESCUE_STEP/3`, mínimo 2, repartido entre los dos IDAC) y nunca crecía. Con
~100 mV que recorrer y un paso cada 87 s (`CP_SLOW_TAU_MS*2`), eso son horas.
Ahora el paso **se duplica** mientras no se vio el otro riel, y cuando el signo
se da vuelta entra la bisección que ya estaba escrita y no depende de ninguna
pendiente.

Medido en placa el 2026-09-18, cambiando a PGA x8 con el nodo en régimen:

| t | estado | SUMo | LPo | I0/I1 |
|---|---|---|---|---|
| 0 s | corriendo | +69,4 | −266,7 | 4/4 |
| 30 s | **APRENDIENDO** | +105,7 | −269,1 | 0/0 |
| 270 s | aprendiendo | +105,6 | −269,3 | 7/7 |
| 330 s | aprendiendo | +84,6 | −268,7 | 7/7 |
| 390 s | aprendiendo | +35,6 | −266,6 | 10/8 |
| 420 s | aprendiendo | **+8,7** | **−191,7** | 10/8 |
| 450 s | aprendiendo | −27,9 | +112,8 | 10/8 |
| 600 s | aprendiendo | −51,3 | +113,6 | 10/8 |

El bloqueo se rompió: el par lento movió SUMo **160 mV**, algo que antes no
lograba ni un milivolt. Pero **se pasó de largo** y quedó del otro lado, con LPo
contra el tope opuesto. Falta que la bisección aterrice; el aprendizaje seguía
corriendo al cierre de la medición.

**Trampa en la que volví a caer** (ya estaba anotada y la repetí): a los 150 s,
con un código puesto, SUMo no se había movido "ni el ruido" y estuve por concluir
que a x8 no había autoridad. Era falso por dos motivos a la vez — el paso era de
un código, o sea movimiento por debajo del ruido, **y** la cola lenta de 44 s no
había llegado. La autoridad existe y es de varios mV por código. Con esta planta,
cualquier afirmación de "no responde" hay que hacerla con un paso grande y
después de 2τ, nunca antes.

## El techo de ganancia es la RESOLUCIÓN del par lento (2026-09-19)

Medido a PGA x24 con traza de los cuatro IDAC: **SUMo solo toma dos valores,
+108 mV o −257 mV, nunca nada en el medio.** Las transiciones lo explican: pasar
de `I0/I1 = 9/7` a `10/7` —un solo código de IDAC0— tira SUMo de +107 a −257.

| magnitud | valor |
|---|---|
| efecto de UN código de IDAC0 sobre SUMo (PGA x24) | ~365 mV |
| ancho de la ventana de validez de SUMo | 195 mV (±97,7) |

**Sobre ese camino no hay dónde aterrizar**, y por eso a PGA alto el nodo
rebotaba entre los dos rieles hasta abortar: cada paso del par lento cruza la
ventana entera, así que la búsqueda a ciegas la saltea siempre.

**Corrección (2026-09-19, misma tarde): el punto sí existe.** Escribí primero
"no existe combinación entera", y es más fuerte de lo que los datos aguantan. En
la validación posterior **x24/x4 entró en banda en 90 s y terminó en +29,6 mV**,
bien adentro. Lo que la medición prueba es que *ese* par de códigos, movido en
*esa* dirección, salta la ventana; no que el espacio entero de 511×511
combinaciones no la contenga. Los dos hechos conviven porque el par lento tiene
dos grados de libertad y sus pesos son distintos: la suma es gruesa, la
**diferencia** es fina.

Lo que sí queda establecido es el mecanismo del techo: **resolución del actuador
contra ancho de ventana**, y a PGA alto la búsqueda a ciegas no sirve. De ahí
salen las dos palancas de firmware que hoy sostienen x16 y x24 (siembra desde el
par vecino y vernier diferencial). Para subirlo de verdad en hardware haría falta
un paso más fino en el par lento (resistencia serie mayor o fondo de escala menor
en IDAC0/IDAC1).

La única palanca que queda en firmware es el **vernier diferencial**: mover
IDAC0 e IDAC1 en sentidos OPUESTOS da la diferencia de sus pesos en vez de la
suma, varias veces más fina. Está implementado como último refinamiento del
aprendizaje, cuando el paso ya bajó a un código.

### Bloqueo permanente por SUMo (grave, arreglado)

Antes de esto el nodo no quedaba lento: quedaba **muerto**. Medidos 820 s sin que
se moviera un solo código, con SUMo fuera de ventana y LPo contra el riel. La
cadena: el aprendizaje aborta → SUMo queda inválido → la guardia de SUMo hace
`return` antes del rescate de LPo → ya nada puede moverse nunca más.

Dos arreglos: (a) la bisección que "no se mueve" ya no aborta —pasaba cuando el
cambio de signo lo producía la COLA y no un paso nuevo, así que los dos rieles
quedaban anotados con los mismos códigos—, y (b) una vigilancia: si SUMo lleva
`CTL_SUM_MUERTO_MS` (2 min) fuera de ventana, el nodo vuelve a aprender solo. Un
nodo que se muere callado en el campo es peor que uno lento.

## PGAout bajo deja al lazo rápido sin actuadores (2026-09-19)

Medido a **x50/x1**, que era el único par que nunca sostenía la banda. La traza
de LPo cuenta la historia entera:

| t (s) | 5 | 25 | 45 | 65 | 85 | … | 345 |
|---|---|---|---|---|---|---|---|
| LPo (mV) | −239 | −265 | **−58** | +96 | +113 | +113 | +113 |

Cruza la ventana entera en 20 s, se planta en el tope de medición y **se queda
ahí 280 s sin que se mueva un solo código**. No es lentitud: es parálisis.

La causa es geométrica. **IDAC2 está aguas ARRIBA de PGAout**, así que su
autoridad sobre LPo se divide por la ganancia de esa etapa: a x1 es 24 veces
menor que en el punto de diseño (x24), tanto que `ctl_effective_config()` le
clava la pendiente en el piso de 100 µV que exige `control_config_valid()`. Con
IDAC3 ya en su tope, **el lazo rápido se queda literalmente sin actuadores**.

| actuador | dónde inyecta | autoridad sobre LPo a PGAout x1 |
|---|---|---|
| IDAC3 | pasabajos, aguas **abajo** | no depende de PGAout — pero es el vernier |
| IDAC2 | sumador, aguas **arriba** | ÷24 respecto de x24: piso de la config |
| IDAC0/IDAC1 | PGA y pasabanda | ÷24 también, pero ×50 del PGA lo compensa |

El único que conserva autoridad es el **par lento**, y el par lento sólo se
mueve aprendiendo.

### Por qué la vigilancia de SUMo no lo salvaba

Porque SUMo estaba **bien**. El pasabajos amplifica ×16 desde el sumador y suma
su propio offset, así que el error puede nacer entero aguas abajo de SUMo: LPo
contra el riel con SUMo dentro de ventana es un estado perfectamente posible, y
la vigilancia de SUMo no se entera nunca.

Arreglo: una vigilancia **propia de LPo** (`CTL_LP_MUERTO_MS`, 90 s). Si LPo
lleva ese tiempo fuera de ventana **y** IDAC3 ya está pasado de `CP_FINE_MID`
—o sea que el lazo rápido agotó lo suyo y no es un rescate en curso—, el nodo
vuelve a aprender, que es lo único que mueve el par lento.

## La tabla de puntos por ganancia ahora sobrevive al reset (2026-09-19)

`ctl_punto[]` vivía sólo en RAM. Cada reset obligaba a re-aprender desde cero
todos los pares menos el último —el único que guarda el perfil—, y ese
re-aprendizaje **es** lo que hacía lento el arranque: minutos a PGA alto.

Ahora se persiste en un área propia de la EEPROM, **filas 48..52 (bytes
768..847)**, separada de los dos slots del perfil (filas 16..47) y con su propio
CRC. Sin doble buffer a propósito: si la imagen no valida se arranca con la
tabla vacía, que es exactamente el comportamiento anterior — perder la tabla
cuesta tiempo, nunca corrección.

| campo | bytes |
|---|---|
| cabecera `'G' 'P'` + versión + clase de hardware | 4 |
| 8 entradas × (1 byte de ganancias + 4 códigos int16) | 72 |
| CRC | 2 |

Se escribe con el **mismo criterio que el perfil** (en banda, congelado en modo
estable y con los códigos cambiados), enganchada al autosave, así que la tabla
se construye sola a medida que el nodo pasa por cada ganancia.

## Los dos bugs que dejaban a x50/x1 afuera (2026-09-19)

La traza completa de los cinco taps a x50/x1 (`lab/diag_par.py`, 420 s) mostró
algo que los veredictos escondían: **el nodo nunca salía de `LEARNING`**. El PI
no era lento, no llegaba a correr nunca. LPo estuvo válido 15 muestras de 210;
el resto lo pasó pegado a uno de los dos topes de medición.

### 1. El criterio de "la cadena se aquietó" medía entre dos vueltas del lazo

```c
int32 delta = ctl_dc[3] - ctl_learning_prev_sum;
ctl_learning_prev_sum = ctl_dc[3];
if (magnitude(delta) > v[CP_STABLE_UV]) return;
```

`ctl_learning()` corre en **cada vuelta del lazo principal**. Entre dos vueltas
SUMo no cambia, así que `delta` daba 0 y el criterio pasaba siempre: degeneraba
en "esperar un τ", justo la espera corta que se quería evitar.

Con los cuatro códigos quietos, SUMo seguía moviéndose **46 mV en 32 s** (~1,4
mV/s): cada paso del aprendizaje salía calculado sobre una foto en pleno
transitorio. Ahora el criterio es una **tasa** —menos de `CP_STABLE_UV` en
`CTL_LEARN_QUIETO_MS` (15 s)— y la ventana se reinicia en cada cambio de fase.

### 2. El objetivo de SUMo no centra LPo

Dos puntos limpios de la misma traza, con SUMo válido:

| SUMo | LPo |
|---|---|
| −25,7 mV | **+43,2 mV** |
| +19,1 mV | **−252,6 mV** |

De ahí: ganancia del pasabajos desde SUMo ≈ **−6,6**, y **LPo se anula con SUMo
≈ −19 mV, no en cero**. Los dos puntos dan el mismo cruce por cero, así que no
es ruido.

`CP_OPA_TARGET_UV` vale **0**. O sea que un aprendizaje perfecto deja LPo en
≈ +127 mV, **fuera de la ventana de ±97,7 por construcción**. A PGAout x24 el
lazo rápido lo tapa con IDAC2; a x1 IDAC2 casi no tiene autoridad (ver sección
anterior) y el nodo queda afuera para siempre.

Arreglo: el objetivo se **biseca solo**. El aprendizaje anota con qué SUMo quedó
LPo de cada lado (`ctl_lp_sum_pos` / `ctl_lp_sum_neg`) y apunta al punto medio,
repitiendo la fase 0. Sólo mira **signos**, como el resto del lazo, así que no
depende de la ganancia del pasabajos —que cambia con el punto y con la placa—.
Se emite como clave `0x10A` para poder seguirlo desde el banco.

Si el objetivo de fábrica ya deja a LPo adentro, esto **no se activa nunca**: los
pares que ya andaban no cambian de comportamiento.

### 3. La condición de cierre del aprendizaje era imposible en esta planta

Con los dos arreglos anteriores el nodo llegó a pisar un punto bueno y **no lo
aceptó**: a x50/x1, `I0/I1 = 12/0` dejó SUMo en −13,8 mV y LPo en **+16,5 mV**
—en banda— y la búsqueda siguió de largo hasta volver al riel.

La fase 2 exigía que SEo, BPo y OPA_SUMo se quedaran **quietos** dentro de
`CP_STABLE_UV` (10 mV) durante `CP_STABLE_MS`. Con la cola de ~44 s del acople de
680 µF, BPo se mueve más de 10 mV en 20 s con los cuatro códigos sin tocar, y a
PGA x50 eso va multiplicado: la condición no se cumple nunca.

Ahora la fase 2 exige **lo que el perfil tiene que lograr**, no la quietud de
unos taps intermedios: todos los taps frescos y válidos, y `|LPo| ≤
CP_DEADBAND_UV`, sostenido `CP_STABLE_MS`. Si LPo se va al riel después de dos τ
vuelve a la fase 1 —que es la que decide si corre el objetivo o rehace los
códigos—, no aborta.

Y al **vencer el plazo** el aprendizaje ya no falla si SUMo quedó medible: se
queda con lo alcanzado. Abortar restaura los códigos viejos, normalmente peores,
y desde que la fase 2 pide además LPo en banda el plazo puede vencer sobre un
punto perfectamente usable. Se emite razón `0x10` en la clave `0x108`.

### Pendiente abierta: el modelo del par lento está errado ~5× a PGA x50

`ctl_choose_slow_codes()` resuelve el par de códigos con un modelo lineal:
`CP_SLOW0_SLOPE_UV` (600 µV/código) y `CP_SLOW1_SLOPE_UV` (400), escalados por la
ganancia de **PGAout**. Medido el 2026-09-19 a x50/x1, entre `I0/I1 = 5/4` (SUMo
≈ +56 mV asentado) y `13/9` (SUMo ≤ −45): **~12,6 mV por código**, contra los
~2,4 que predice el modelo.

La sospecha natural es que IDAC0 inyecta en la etapa del PGA y por lo tanto su
efecto escala también con la ganancia del **PGA**, que el escalado no contempla.
**No está verificado**: el rescate mueve IDAC0 e IDAC1 siempre juntos, así que
con estas trazas no se puede atribuir el movimiento a uno de los dos, y hay una
medición previa que dice que IDAC0 no tiene autoridad de continua (el capacitor
de 680 µF lo bloquea). Para cerrarlo hace falta mover **uno solo** y esperar
varias τ.

Mientras tanto el error de modelo no es fatal porque lo que aterriza es la
bisección, que sólo mira signos; pero explica por qué la primera tirada de
`choose_slow_codes` se pasa de largo sistemáticamente a ganancia alta.

### El bracket hay que anotarlo siempre, no sólo cuando la fase 1 mira

Primera versión de la bisección del objetivo: no se activó **ni una vez**. El
bracket se anotaba dentro de la fase 1, y la fase 1 corre una vez por ciclo de
aprendizaje: el otro riel de LPo pasaba entre dos visitas, así que sólo quedaba
anotado **un lado** y el punto medio nunca se podía calcular.

Ahora el bracket se anota en cada vuelta mientras dura el aprendizaje, siempre
que SUMo sea medible y LPo esté fuera de banda, y se guarda el valor que
**estrecha** el intervalo (no el último que tocó), que es lo que lo hace
converger. La decisión sigue estando en la fase 1.

Con eso, x50/x1 aterrizó por primera vez: objetivo bisecado a **−7,8 mV**, SUMo
final **−9,7 mV** y LPo **−11,9 mV**, dentro de banda, en ~600 s de aprendizaje
en frío.

### La puerta: al lazo rápido se le da su oportunidad primero

Tal como estaba escrita, la bisección se habría activado **también en los pares
que ya funcionan** —porque LPo casi siempre está fuera de banda en el momento en
que la fase 1 mira, antes de que el lazo rápido actúe—, y eso es cambiar algo
que anda. Ahora hace falta que la fase 2 haya corrido y haya fracasado: sólo si
después de 2 τ (riel) o 4 τ (LPo válido pero corrido) LPo sigue fuera de banda se
declara que el objetivo de SUMo es el que está mal y se lo deja bisecar.

### x50/x1 cerrado (2026-09-19)

Con los cinco arreglos, el par que nunca había sostenido la banda aprendió en
frío y quedó regulando:

| t (s) | estado | banda | SUMo | LPo | I0/I1 | I2 | I3 |
|---|---|---|---|---|---|---|---|
| 2 | LEARNING | no | +107,8 | −268,6 | 0/0 | 0 | 0 |
| 218 | LEARNING | no | −18,5 | +50,5 | 7/7 | 0 | 0 |
| 614 | LEARNING | no | −26,1 | +84,4 | 3/1 | −8 | −5 |
| **758** | **RUNNING** | **sí** | −18,4 | +28,9 | 3/1 | −8 | −33 |
| 866 | RUNNING | sí | −19,5 | +26,8 | 3/1 | −8 | −47 |

SUMo terminó en **−19 mV**, que es exactamente el cero del pasabajos medido más
arriba de forma independiente. 758 s de aprendizaje **en frío**; a partir de acá
el punto queda en la tabla persistida y volver al par no vuelve a costar eso.

### La tabla persistida no servía de nada si nunca se escribía

Primer barrido con x50/x1 ya aprendido: **volvió a fallar**, y el motivo no
tenía nada que ver con el lazo. Colgada del autosave, la tabla exige además
`CP_SETTLED_MS` (60 s congelado); el aprendizaje de x50/x1 terminó en banda pero
con IDAC3 todavía caminando, así que **la tabla nunca se escribió**. Al reiniciar,
el primer comando de ganancia —pidiendo la **misma** ganancia que ya tenía— no
encontró punto y disparó un aprendizaje entero de cero.

Dos arreglos:

- **El punto se fija al terminar el aprendizaje**, no cuando el lazo se congela.
  Ese punto costó minutos y es exactamente el que evita repetirlos. Vale también
  para el cierre por vencimiento de plazo.
- **El perfil siembra la tabla al arrancar**: el perfil *es* el punto del par con
  el que fue aprendido. Sólo si la tabla no traía ya una entrada para ese par,
  que sería más nueva.

Lección general: una caché que se llena con un criterio más estricto que el que
la consume no es una caché, es código muerto que cuesta EEPROM.

## El ciclo límite de riel a riel: la bisección volvía a expandir (2026-09-19)

Ya con x50/x1 aprendido y el punto guardado, el nodo **seguía oscilando de riel
a riel**, con período de ~280 s y amplitud constante, indefinidamente:

```
LPo: -265 -265 -265 | 112 113 112 112 113 113 112 113 | -265 -265 ... | 112 112 ...
```

Amplitud constante es la firma de un ciclo límite, no de una búsqueda lenta.

El rescate de LPo es una bisección con expansión: el paso **duplica** mientras el
signo del error no cambia y se **parte al medio** cuando cambia. Pero las dos
reglas convivían sin orden: al cruzar partía el paso, y en cuanto el sentido
coincidía dos veces seguidas lo volvía a duplicar. Con una pendiente empinada
—donde un solo paso cruza la ventana entera— el ciclo es:

1. paso grande → cruza de un riel al otro,
2. cambio de signo → parte el paso al medio,
3. el paso partido cae **dentro** de la ventana pero fuera de la banda muerta,
4. mismo sentido otra vez → **duplica** → vuelve al riel. Y de nuevo.

Arreglo: la expansión es una **fase**, no una regla permanente. El primer cambio
de signo la termina para siempre (`rescue_bracket`): a partir de ahí el paso sólo
se parte al medio. Es el algoritmo clásico de expandir-hasta-acotar y después
bisecar; estaba escrito a medias.

Reproducido y arreglado **en host**, sin ocupar la placa: `tests/control_test.c`
simula una pendiente de 30 mV/código —un paso cruza la ventana— y exige entrar en
banda con a lo sumo 6 cruces. Antes daba 40 cruces y no entraba nunca.

Además, el bracket ya no se borra sólo porque LPo haya vuelto a ser medible —con
un paso grande LPo pasa por "válido" un instante al cruzar, y borrarlo ahí hacía
que el rescate arrancara de cero en el riel opuesto—. Se borra al entrar en HOLD,
que es cuando la búsqueda de verdad terminó.

## La guardia de SUMo dejaba al vernier sin correr (2026-09-19)

Traza de los cinco taps y los cuatro IDAC durante una recuperación fallida a
x50/x1, con el nodo en `RUNNING`:

| t (s) | SUMo | LPo | I2 | I3 |
|---|---|---|---|---|
| 26 | +62,9 | −265,4 | −12 | −23 |
| 98 | +98,4 | −265,3 | −13 | −23 |
| 170 | +100,8 | −265,4 | −15 | −23 |
| 194 | +100,7 | −265,3 | −15 | −23 |

**LPo estuvo 200 s contra el riel moviéndose un código de IDAC2 cada ~24 s
mientras IDAC3 no se movió ni una sola vez.** La guardia de SUMo hacía `return`
incondicional, así que con SUMo fuera de banda el rescate de LPo no llegaba a
ejecutarse nunca. Y el rescate de LPo **no mira SUMo para nada** —IDAC3 está
aguas abajo—, así que no había ninguna razón para bloquearlo.

Arreglo: la guardia mueve el grueso como antes, pero si LPo está contra un riel
deja correr la bisección del vernier antes de salir.

(A los 218 s la vigilancia de SUMo re-disparó el aprendizaje, funcionando como
está diseñada: el nodo llevaba 120 s con SUMo fuera de ventana. Pero es el
martillo pesado, y no debería hacer falta llegar ahí.)

## El aprendizaje descartaba puntos usables por seis milivoltios

En esa misma corrida, a los 530 s el aprendizaje tenía **SUMo −20,0 mV y LPo
+51,0 mV** —dentro de la ventana de ±97,7 con holgura— y lo descartó para seguir
buscando, porque la fase 2 exigía `|LPo| ≤ CP_DEADBAND_UV` (45 mV). Terminó
volviendo al riel.

El perfil no tiene que dejar LPo centrado: tiene que dejarlo **donde el vernier
pueda terminar el trabajo**, que es exactamente para lo que existe el vernier.
Dos cambios:

- La banda de aceptación pasa a ser la del **modo estable** (60 mV).
- Mientras LPo **mejore**, el reloj de "este punto no sirve" se reinicia. Si no,
  el lazo rápido está convergiendo y el aprendizaje le saca los códigos de
  abajo.

## Un punto guardado que no reproduce hay que tirarlo

Con el vernier ya corriendo durante la guardia, la traza mostró lo que faltaba:

| t (s) | SUMo | LPo | I2 | I3 |
|---|---|---|---|---|
| 22 | +65,3 | −265,1 | −12 | **−15** |
| 62 | +92,1 | −265,2 | −12 | **+97** |
| 102 | +97,8 | −265,2 | −13 | **+255** |
| 182 | +98,7 | −265,3 | −15 | **+255** |

IDAC3 recorrió 270 códigos —todo su rango— **y LPo no se movió del riel**. No es
un problema del vernier: con **SUMo plantado en +100 mV** y la ganancia del
pasabajos de ~−6,6, LPo queda a cientos de milivoltios del objetivo. Ninguna
autoridad aguas abajo alcanza.

¿Por qué quedó SUMo ahí? Porque el punto restaurado de la tabla tenía el **par
lento en 0/0** —lo había dejado ahí un rescate, no un aprendizaje— y al volver a
x50/x1 desde x24/x4 ese par de códigos aterrizó fuera de ventana. Es
[el estado depende del camino] otra vez: los cuatro códigos no definen el punto
de trabajo, por el acople de 680 µF y su absorción dieléctrica.

Arreglo: cuando una de las dos vigilancias tiene que re-disparar el aprendizaje,
**invalida la entrada de la tabla para ese par** antes. Si no, se restaura el
mismo punto malo en cada visita y hay que re-aprender siempre. La tabla se
vuelve así autocorrectiva: guarda lo que funcionó y olvida lo que no.

## El actuador grueso también tiene que expandir

Con el vernier ya libre de correr durante la guardia, la traza mostró el último
eslabón: **IDAC3 recorrió sus 255 códigos, se saturó, y a partir de ahí IDAC2
avanzaba UN código cada 45 s** cuando hacían falta decenas. LPo se quedaba
contra el riel minutos enteros con los dos actuadores "trabajando".

La escalada al grueso movía un código fijo por enfriamiento. Ahora expande igual
que el vernier y por la misma razón: duplica mientras el signo no cambie, se
parte al medio cuando hay bracket, tope de 16 códigos.

Es seguro en las dos puntas del rango de PGAout, y no por casualidad:

- a **PGAout x24** un código de IDAC2 mueve LPo la ventana entera, así que el
  signo se da vuelta en el primer paso, `rescue_bracket` se prende y la
  expansión se corta sola en uno — el comportamiento de siempre;
- a **PGAout x1** un código mueve ~6 mV y la expansión es justamente lo que
  hace falta.

El mismo mecanismo se autorregula según la autoridad real del actuador, sin
tabla de parámetros por ganancia.

## El aprendizaje mide su propia pendiente (2026-09-19)

`ctl_choose_slow_codes()` resolvía el par de códigos con las pendientes de
fábrica, medidas a **PGA x4**. A **x50** el modelo predice ~2,4 mV/código y la
realidad son ~**12,6**: cada paso se pasa de largo por cinco, cruza la ventana
entera, y el siguiente vuelve a cruzarla en el otro sentido. Eso es lo que hacía
que el aprendizaje a ganancia alta tardara cientos de segundos aunque cada paso
individual fuera correcto.

Ahora el paso **promete** un desplazamiento de SUMo (`ctl_slow_pred`) y, cuando
la cadena se aquieta y SUMo sigue siendo medible, se contrasta con lo que de
verdad pasó. El cociente corrige un factor acumulado (`ctl_slow_gain_x1000`,
0,1× a 20×) que multiplica las dos pendientes. Se emite como clave `0x10B`.

No se aprende de un paso cuyo signo no coincide con el predicho, ni de uno que
midió contra un riel: ahí el número no significa nada. El factor es propiedad
del punto de ganancia, así que se reinicia al cambiar de ganancia y no en cada
aprendizaje — el segundo aprendizaje en la misma ganancia empieza ya sabiendo.

## El atajo: si ya está donde tiene que estar, terminó

Dos veces en la misma tarde, a x50/x1, el aprendizaje pasó por un punto bueno y
lo tiró:

| I0/I1 | SUMo | LPo | banda |
|---|---|---|---|
| 12/0 | −13,8 mV | +16,5 mV | sí |
| 12/0 | −18,2 mV | +42,9 mV | **sí (`banda=1` en la telemetría)** |

Las dos veces siguió buscando hasta volver al riel. La causa no es ninguna de las
condiciones que ya se corrigieron: es que **la máquina de fases sólo mira en
instantes concretos**, y el punto bueno caía en mitad de una espera de fase.

Arreglo: una comprobación al principio de `ctl_learning()`, antes de toda la
máquina de fases. Si los cinco taps están frescos y válidos, SUMo es medible y
`|LPo| ≤ CP_SETTLED_BAND_UV` sostenido `CP_STABLE_MS`, se guarda el perfil, se
fija el punto en la tabla y se pasa a `RUNNING` — en la fase que sea y como sea
que se haya llegado. Razón `0x11` en la clave `0x108`.

Una máquina de estados que persigue un objetivo tiene que poder **reconocer que
ya lo alcanzó** en cualquier estado, no sólo en el que tenía previsto.

## x50/x1: qué se sabe y qué no (cierre del 2026-09-19)

**El punto existe y se sostiene.** Medido: el nodo aprendió en frío, pasó a
`RUNNING` a los 758 s y se quedó regulando con SUMo en −19 mV y LPo entre +27 y
+37 mV durante minutos, con el vernier trabajando. No es un par imposible.

**Lo que no es confiable es la búsqueda en frío.** La razón es geométrica y está
medida: el pasabajos amplifica desde SUMo con ganancia ≈ −6,6 y su cero está en
SUMo ≈ −19 mV, así que **la ventana de ±97,7 mV de LPo corresponde a apenas
±15 mV de SUMo**. A PGA x50 un código del par lento mueve SUMo ~12,6 mV: el
blanco son **poco más de dos códigos**. La cadena atraviesa esa franja en ~20 s
mientras el aprendizaje mide cada 44 s, así que el barrido la cruza sin verla.

Todo lo que se corrigió hoy ayuda pero no cambia esa aritmética. Queda anotado
que el camino real de mejora es de **hardware**: un paso más fino en el par lento
(resistencia serie mayor o fondo de escala menor en IDAC0/IDAC1), exactamente lo
mismo que pedía el techo de ganancia.

**Recomendación práctica:** x50/x1 da ganancia conjunta **50**, y `x4/x24` da
**96** entrando en banda en 5 s. Como punto de operación no hay ninguna razón
para preferir x50/x1. Se deja documentado, no bloqueado.

### Lo que se probó y se revirtió: expandir el actuador grueso

Probado y **descartado** el 2026-09-19: hacer que la escalada a IDAC2 expandiera
como el vernier (duplicar mientras el signo no cambie) para salir más rápido del
riel a PGAout x1.

**Regresionó x8/x8**, que venía pasando. El motivo es estructural, no de ajuste:
el corte de la expansión depende de **ver** el cambio de signo, y el efecto de
IDAC2 tarda los ~44 s de la cola del pasabanda en llegar. Entre paso y paso
(45 s) la lectura todavía no refleja el paso anterior, así que la expansión
duplica a ciegas durante varios enfriamientos y se pasa de largo.

La regla general: **no se puede expandir a lazo cerrado un actuador cuyo efecto
llega más tarde que el paso siguiente**, si el criterio de corte es la lectura
instantánea. El vernier sí puede porque actúa en el pasabajos, aguas abajo del
acople, y su efecto se ve enseguida.

## Tabla de pares estables (medida 2026-09-19, 12/12)

Barrido de regresión completo, dos vueltas, con el veredicto honesto: **estable**
es el instante desde el cual LPo ya no vuelve a salir de la ventana. El "primer
cruce" que declara el firmware puede ser un rescate cruzando de un riel al otro,
y como número miente.

| par | ganancia | 1ª visita | al volver | LPo final |
|---|---|---|---|---|
| x4/x24 | **96** | 5 s | 5 s | −25,7 mV |
| x8/x8 | 64 | 5 s | 10 s | +22,4 mV |
| x4/x8 | 32 | 10 s | 10 s | −24,8 mV |
| x24/x4 | **96** | 10 s | 10 s | −30,2 mV |
| x16/x4 | 64 | 15 s | 10 s | +17,3 mV |
| x50/x1 | 50 | 810 s | 5 s | +65,4 mV |

Cinco de los seis entran en banda y se quedan quietos en **15 s o menos**, contra
los 250–350 s que medían antes de esta sesión. La columna "al volver" es la que
importa en uso real y es donde se ve la tabla de puntos persistida: x50/x1 pasa
de 810 s a 5 s, x16/x4 de 290 s a 10 s.

**Recomendación de operación:** `x4/x24` (ganancia 96, 5 s). `x24/x4` da la misma
ganancia por otro reparto y sirve de alternativa si el PGA alto conviene por
ruido de entrada.

## El techo de ganancia era de la BÚSQUEDA, no del instrumento (2026-09-19)

Con el lazo ya arreglado, probé combinaciones por encima de las validadas. **Las
seis entraron en banda**, en una sola visita cada una:

| par | ganancia | estable en | LPo final | |
|---|---|---|---|---|
| x8/x24 | **192** | 5 s | +26,4 mV | limpio |
| x16/x24 | **384** | 10 s | +13,5 mV | limpio |
| x24/x8 | 192 | 20 s | −33,1 mV | bien |
| x50/x4 | 200 | 50 s | +24,2 mV | bien |
| x32/x4 | 128 | 95 s | +50,8 mV | aceptable |
| x32/x24 | 768 | 295 s | +91,7 mV | **marginal** |

**x16/x24 da ganancia 384 en 10 s con LPo a 13 mV del centro** — cuatro veces la
que este documento venía llamando el techo, y más rápido que varios de los pares
"seguros".

Sobre x32/x24 (768) hay que ser explícito: pasa el criterio, pero se estabiliza
en +91,7 mV contra un borde de ventana de 97,7. Está adentro **sin margen**;
cualquier deriva lo saca. No es un punto de operación, es el borde.

Lo que esto dice: el "techo de ganancia" que se venía documentando **no era del
instrumento sino de la búsqueda**. Mientras el aprendizaje no aterrizaba, cada
par que fallaba se leía como un límite físico. Arreglada la búsqueda, el límite
se corrió 4×. La lección vale para el resto del proyecto: *si un barrido termina
en su borde, verificar de quién es el borde.*

Pendiente: estas son visitas únicas. Los tres mejores se revalidan con dos
vueltas antes de darlos por buenos — una corrida es una corrida.
