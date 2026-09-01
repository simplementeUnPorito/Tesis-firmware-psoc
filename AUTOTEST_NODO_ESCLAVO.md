# Autotest de placa del nodo esclavo

## 1. Qué es y qué no es

Este autotest valida que una placa recién armada del nodo esclavo esté bien armada. Busca fallas gruesas de soldadura, ruteo, componentes y cables sin modificar el circuito y sin usar instrumental externo.

No valida el sistema completo ni el protocolo desde una computadora personal (PC). No reemplaza los runners E1–E19 de `docs/plan_pruebas_precampo.md`; esos runners prueban el sistema y el protocolo desde la PC. Este autotest prueba la placa local.

La fase automática corre al arrancar el firmware `slaveTest`. La fase interactiva se inicia por la consola serie y requiere que el operador pulse botones o golpee junto al geófono.

### Siglas y convenciones

| Sigla | Significado |
|---|---|
| AC | Corriente alterna (*alternating current*). |
| ADC | Convertidor analógico-digital (*analog-to-digital converter*). |
| AMux | Multiplexor analógico (*analog multiplexer*). |
| COM | Puerto serie identificado por Windows. |
| CRC | Comprobación de redundancia cíclica (*cyclic redundancy check*). |
| CS | Selección de chip (*chip select*). |
| DC | Corriente continua (*direct current*). |
| DFB | Bloque de filtro digital (*digital filter block*). |
| DMA | Acceso directo a memoria (*direct memory access*). |
| E2E | De extremo a extremo (*end to end*). |
| EEPROM | Memoria de solo lectura programable y borrable eléctricamente (*electrically erasable programmable read-only memory*). |
| FAT | Tabla de asignación de archivos (*File Allocation Table*). |
| FIR | Respuesta finita al impulso (*finite impulse response*). |
| GEO | Clase de hardware del nodo geófono. |
| GND | Masa o referencia de tierra (*ground*). |
| GPIO | Entrada/salida de propósito general (*general-purpose input/output*). |
| HAMMER | Clase de hardware del nodo martillo. |
| I2C | Bus de circuito interintegrado (*Inter-Integrated Circuit*). |
| IDAC | Convertidor digital-analógico de corriente (*current digital-to-analog converter*). |
| IRQ | Solicitud de interrupción (*interrupt request*). |
| JSON | Notación de objetos de JavaScript (*JavaScript Object Notation*). |
| LE | Orden de byte menos significativo primero (*little-endian*). |
| MOSI | Datos del maestro hacia el esclavo (*master out, slave in*). |
| NVS | Almacenamiento no volátil (*non-volatile storage*). |
| OLED | Diodo orgánico emisor de luz (*organic light-emitting diode*). |
| PC | Computadora personal (*personal computer*). |
| PGA | Amplificador de ganancia programable (*programmable gain amplifier*). |
| PSoC | Sistema programable en chip (*Programmable System-on-Chip*). |
| RMS | Valor eficaz (*root mean square*). |
| RX | Recepción (*receive*). |
| SCK | Reloj serie de SPI (*serial clock*). |
| SCL | Reloj serie de I2C (*serial clock line*). |
| SD | Tarjeta de memoria *Secure Digital*. |
| SDA | Datos serie de I2C (*serial data line*). |
| SPI | Interfaz periférica serie (*Serial Peripheral Interface*). |
| SRAM | Memoria estática de acceso aleatorio (*static random-access memory*). |
| SYNC | Señal de sincronismo. |
| TX | Transmisión (*transmit*). |
| UART | Transmisor-receptor asíncrono universal (*universal asynchronous receiver-transmitter*). |
| USB | Bus serie universal (*Universal Serial Bus*). |
| VDAC | Convertidor digital-analógico de tensión (*voltage digital-to-analog converter*). |
| XOR | Operación OR exclusiva (*exclusive OR*). |

Las tensiones se expresan en microvoltios (`uV`) o milivoltios (`mV`); las pendientes, en microvoltios por código de IDAC (`uV/código`); los tamaños, en kilobytes (`KB`) o megabytes (`MB`); las frecuencias, en hercios (`Hz`); y los tiempos, en milisegundos (`ms`) o segundos (`s`). `pp` significa pico a pico. `Fs` es la frecuencia de muestreo, `Vss` es la referencia baja de alimentación y *tap* es un punto de medición de la cadena. `PASS`, `FAIL`, `WARN`, `SKIP` e `INFO` significan aprobado, fallado, advertencia, omitido e informativo, respectivamente.

## 2. Estructura de archivos

El autotest usa dos firmwares:

```text
firmware/
├── psoc/
│   └── AcondicionamientoAnalogicoTest/
│       ├── AcondicionamientoAnalogico.cydsn/
│       │   ├── AcondicionamientoAnalogico.cywrk
│       │   ├── psoc_selftest.h
│       │   └── calibration.h
│       ├── Analog_LPF_v1_0.cylib/
│       ├── Component Libraries/
│       └── shared/
└── esp32/
    └── Nodo comunicación/
        └── slave/
            ├── platformio.ini
            ├── autotest_runner.py
            └── src/
                ├── main_selftest.cpp
                └── selftest_report.h
```

La carpeta interna se llama `AcondicionamientoAnalogico.cydsn`, y no `AcondicionamientoAnalogicoTest.cydsn`, porque PSoC Creator exige que la carpeta tenga el mismo nombre que el proyecto. Cambiarla produce `prj.M0143`. Renombrar el proyecto para acompañar ese cambio obligaría a editar a mano los archivos `.cyprj`, `.cywrk` y `.cydwr`; este último contiene el bloqueo de pines (*pin lock*).

`Analog_LPF_v1_0.cylib`, `Component Libraries` y `shared` están copiadas en `AcondicionamientoAnalogicoTest`, un nivel por encima de `AcondicionamientoAnalogico.cydsn`. El archivo `.cyprj` las referencia mediante rutas con `..`; al anidar el proyecto un nivel más, las referencias originales dejaron de resolver y PSoC Creator informó `sdb.M0033`. Estas copias son un *snapshot*: no se sincronizan solas y pueden divergir de sus originales.

Del lado del ESP32, `main_selftest.cpp` reemplaza a `main.cpp` solamente en el entorno `slaveTest`. Los drivers `psoc_uart`, `espnow_transport` y `local_ui` son los mismos que usa el firmware de campo. `SLAVE_SELFTEST=1` habilita el reporte; sin esa definición, `selftest_report.h` deja funciones vacías que el compilador puede eliminar.

## 3. Arquitectura

El PSoC expone operaciones atómicas:

- escribir el IDAC de una etapa;
- medir en DC un canal del AMux;
- medir una serie para obtener media, RMS, pico a pico y amplitud de 50 Hz;
- armar o leer el contador de flancos de sincronismo;
- devolver identidad, calibración, configuración del ADC, EEPROM, estado de la tarjeta SD, botón y contadores de excepciones.

Cada resultado vuelve al ESP32 en una trama de control `0xC5`. El ESP32 ordena las operaciones, calcula pendientes, cocientes y dispersiones, aplica los umbrales y decide `PASS`, `FAIL`, `WARN`, `SKIP` o `INFO`.

El reparto tiene dos motivos implementados:

1. Cada comando del PSoC termina rápido y permite que su lazo principal siga contestando *pings* entre mediciones. Un secuenciador completo en el PSoC bloquearía el lazo durante varios segundos y el ESP32 daría el enlace por caído.
2. Las pendientes, cocientes y tolerancias son más legibles y ajustables en el ESP32, que dispone de punto flotante cómodo y no tiene la misma presión de SRAM.

El estímulo analógico sale de los cuatro IDAC8 de referencia ya presentes en el diseño. Las medidas entran por los cinco canales existentes del AMux del ADC. El autotest no agrega componentes ni pines al `TopDesign`.

| Etapa IDAC | Canal AMux asociado | Punto medido |
|---:|---:|---|
| 0, `Vref_PGA` | 0 | Salida de `PGAgain`. |
| 1, `Vref_BP` | 1 | Salida del pasabanda, `BPo`. |
| 2, `Vref_ADDER` | 2 | Salida del sumador o `PGAout`, `SUMo`. |
| 3, `Vref_LP` | 3 | Salida del pasabajos, `LPo`. |
| — | 4 | `AMuxCapacitor`, capacitor de 100 nF a `Vss`. |

El mapeo efectivo no se duplica en `psoc_selftest.h`: sale de la tabla de calibración. Hay dos etapas en HAMMER y tres o cuatro en GEO, según `VDAC_ref_BP`.

### Cableado usado por el autotest

```text
PC
│ USB, 115200 bit/s, COM12
▼
ESP32
├── GPIO26, UART TX ───────────────────────────────► PSoC UART RX, P15[0]
├── GPIO27, SYNC_TO_PSOC_PIN ─────────────────────► PSoC SYNC_IN, P0[4]
├── GPIO21, I2C SDA ◄────────────────────────────── PSoC maestro I2C
├── GPIO22, I2C SCL ◄────────────────────────────── PSoC maestro I2C
│                         dirección del ESP32: 0x42
├── OLED SPI: SCK=GPIO18, MOSI=GPIO23, CS=GPIO33,
│             DC=GPIO16, RESET=GPIO17
├── botón UP=GPIO34, DOWN=GPIO35, OK=GPIO36, BACK=GPIO39
│             cada entrada con 10 kΩ a 3V3; el botón cierra a GND
└── GND común ───────────────────────────────────── PSoC GND

PSoC: botón de calibración en P2[2], incorporado en el CY8CKIT-059
```

El pin `GPIO25`, antes usado como recepción UART del ESP32, queda sin conectar por diseño porque el PSoC nuevo no tiene transmisión UART. Toda la subida PSoC→ESP32 se hace por I2C.

Hay una discrepancia dentro de las fuentes: `main_selftest.cpp`, que implementa y rotula los tests B2 y B3, usa `P15[0]` para UART RX y `P0[4]` para `SYNC_IN`; los comentarios generales de `platformio.ini` y `BUILD_PROGRAM_PSOC.md` todavía consignan `P2[0]` y `P1[5]`. El diagrama anterior sigue el pinout que declara el autotest implementado. Antes de usar las notas generales para cablear otra variante, hay que resolver esa diferencia contra el bloqueo de pines del proyecto.

## 4. Protocolo del autotest

### Comandos ESP32 → PSoC

| Código | Parámetros | Operación | Resultado `0xC5` |
|---:|---|---|---|
| `0xA0` | Un byte: selector de reporte. | Solicita un reporte compuesto o de valor único. | Una o más tramas, según el selector. |
| `0xA1` | Un byte: `1` arma y pone a cero el contador; `0` detiene y lee. | Cuenta flancos en `SYNC_IN`. | `ST_ID_SYNC`. |
| `0xA2` | Dos bytes: etapa `0..3`, código `0..255`. | Escribe el IDAC de la etapa. | `0x20..0x23`, según la etapa. |
| `0xA4` | Un byte: `settle_sel` en el nibble alto y canal en el bajo. | Espera, mide 32 conversiones DC con el capacitor del AMux conectado y devuelve media y pico a pico. | `0x50..0x54`, según el canal. |
| `0xA7` | Un byte: `n_sel` en el nibble alto y canal en el bajo. | Espera 120 ms, mide una serie sin el capacitor del AMux y calcula media, RMS, pico a pico y componente de 50 Hz. | Dos tramas por canal: `0x60..0x64` y `0x70..0x74`. |

Selectores de `0xA0`:

| Selector | Reporte |
|---:|---|
| `0` | Identidad y cantidad de etapas/canales. Emite dos tramas. |
| `1` | Slots válidos de EEPROM. |
| `2` | Resultado de calibración. Emite una trama por etapa. |
| `3` | Configuración y decimación del ADC. |
| `4` | IRQ inesperadas y fallas graves del procesador (*HardFault*). |
| `5` | Estado de la tarjeta SD; lo atiende `main.c`. |
| `6` | Nivel crudo del botón del PSoC. |

Selectores del nibble alto:

| Selector | Asentamiento de `0xA4` | Cantidad de muestras de `0xA7` |
|---:|---:|---:|
| 0 | 5 ms | 256 |
| 1 | 30 ms | 512 |
| 2 | 120 ms | 1024 |
| 3 | 500 ms | 2048 |
| 4 | 1200 ms | 4096 |
| 5 | 3000 ms | 8192 |
| 6 | 6000 ms | 128 |
| 7 | 12000 ms | 64 |

### Trama PSoC → ESP32

La trama `0xC5` mide exactamente 13 bytes:

```text
índice   0     1       2        3       4..7          8..11          12
       +-----+------+---------+--------+-------------+--------------+-----+
valor  | 0xAB| 0xC5 | test_id | status | v0 int32 LE | v1 int32 LE  | XOR |
       +-----+------+---------+--------+-------------+--------------+-----+
```

`int32 LE` es un entero de 32 bits con signo en orden de byte menos significativo primero (*little-endian*). El byte final es la operación OR exclusiva (`XOR`) de los bytes 2 a 11 inclusive; no incluye `0xAB`, `0xC5` ni el propio byte de comprobación.

| `status` | Significado |
|---:|---|
| `0`, `ST_OK` | Operación correcta. |
| `1`, `ST_ERR` | Error. |
| `2`, `ST_REJECTED` | Comando válido, rechazado por el estado actual. |

### Identificadores de resultado

| `test_id` | Alcance | `v0` | `v1` |
|---:|---|---|---|
| `0x01` | Identidad | Clase de hardware. | Frecuencia de muestreo nativa, en Hz. |
| `0x02` | EEPROM | Cantidad de slots válidos. | Máscara de slots válidos. |
| `0x03` | Sincronismo | Flancos contados. | Nivel actual de `SYNC_IN`. |
| `0x04` | Tarjeta SD | Byte de estado. | Banderas de error. |
| `0x05` | ADC | Configuración `1..4`. | Decimación. |
| `0x06` | Etapas | Cantidad de etapas. | Cantidad de canales del AMux. |
| `0x07` | Excepciones | IRQ inesperadas. | Cantidad de *HardFault*. |
| `0x08` | Botón | Nivel crudo de `P2[2]`. | Sin uso. |
| `0x10..0x13` | Calibración por etapa; nibble bajo = etapa. | Código IDAC final. | Medida final. |
| `0x20..0x23` | Escritura por etapa; nibble bajo = etapa. | Código IDAC escrito. | Tensión nominal, en uV. |
| `0x50..0x54` | Medición DC por canal; nibble bajo = canal. | Media, en uV. | Pico a pico, en uV. |
| `0x60..0x64` | Medición AC A por canal; nibble bajo = canal. | Media, en uV. | RMS, en uV. |
| `0x70..0x74` | Medición AC B por canal; nibble bajo = canal. | Pico a pico, en uV. | Amplitud de 50 Hz, en uV. |

## 5. Perfil de hardware presente

La placa se arma por partes. El autotest puede correr aunque todavía falten el OLED, los cuatro botones del ESP32, la tarjeta SD o el geófono. El perfil también incluye al PSoC. Las partes válidas son `oled`, `btn`, `geo`, `sd` y `psoc`.

Cada parte tiene uno de estos estados:

| Valor | Estado | Regla de veredicto |
|---:|---|---|
| `0` | AUSENTE | La parte no está montada. Sus pruebas dan `SKIP` y nunca reprueban la placa. |
| `1` | PRESENTE | La parte debería estar montada y operativa. Si no contesta o no se observa la señal esperada, da `FAIL`. |
| `2` | AUTO | Estado predeterminado. Ante una lectura compatible tanto con una parte no montada como con una parte averiada, da `WARN` y muestra las dos interpretaciones; no inventa cuál de ellas ocurrió. |

El perfil se guarda en la NVS del ESP32 y sobrevive al reflasheo. Se declara una vez y se conserva hasta cambiarlo explícitamente.

| Comando | Acción |
|---|---|
| `hw` | Muestra el perfil completo. |
| `hw <parte> <0|1|2>` | Cambia una parte y guarda el perfil en NVS. Ejemplo: `hw sd 0`. |
| `oled si` | Confirma visualmente el OLED y lo guarda como PRESENTE. |
| `oled no` | Guarda el OLED como AUSENTE; A2 dará `SKIP` en las corridas siguientes. |

## 6. Catálogo de tests

Los criterios de esta sección son los que aplica `main_selftest.cpp`. La columna de falla física indica qué problema de placa sugiere el resultado; cuando el test también puede fallar por firmware o no localiza un componente, se lo dice explícitamente.

### Grupo A — ESP32 solo

| Código | Qué prueba | Criterio implementado | Qué falla física implica un `FAIL` |
|---|---|---|---|
| A1 | Arranque del ESP32. | `FAIL` si el motivo de reinicio es pánico, caída de tensión, *watchdog* de interrupción o *watchdog* de tarea. Los demás motivos dan `PASS`. | Una caída de tensión apunta a alimentación insuficiente y, en una placa nueva, al regulador mal colocado. Pánico y *watchdog* también pueden indicar firmware; A1 no los atribuye a una soldadura concreta. |
| A2 | OLED SSD1306 por SPI. | El bus es de ida sola y el SSD1306 no se puede detectar. Si el OLED no está declarado AUSENTE, dibuja el patrón y da `INFO`; la confirmación es visual mediante `oled si` o `oled no`. `Adafruit_SSD1306::begin()` sólo da `false` si falla la reserva del *framebuffer*. | Un `FAIL` de A2 significa falta de RAM en el ESP32, no un OLED roto. El estado físico del OLED se confirma mirando la pantalla. |
| A3 | Entradas de los cuatro botones del ESP32. | En cada GPIO se hacen 200 lecturas separadas 1 ms. Si las cuatro permanecen en alto, da `PASS`, pero eso no prueba que estén los *pull-ups*: un pin de solo entrada flotante puede sostener el nivel por carga residual. Una entrada en bajo usa el perfil: AUSENTE da `SKIP`, AUTO da `WARN` y PRESENTE da `FAIL`. | Un nivel bajo detecta un pin pegado en bajo: puede faltar la resistencia externa de 10 kΩ a 3V3, estar el pulsador en corto o estar el pin puenteado a masa. |
| A4 | Inicialización, par y transmisión ESP-NOW. | `PASS` si `esp_now_send()` devuelve `ESP_OK`; de lo contrario, `FAIL`. | No localiza una unión concreta: señala la cadena de radio/ESP32 o su inicialización. |
| A5 | Visibilidad por radio del punto de acceso `GeoNetwork`. | Siempre `INFO`: informa canal y potencia recibida si lo encuentra, o cantidad de redes y ausencia del maestro. | No produce `FAIL`; que el maestro esté apagado no es falla de esta placa. |
| A6 | GPIO25, antigua recepción UART. | Siempre `SKIP`. | Ninguna: está sin conectar por diseño. |

### Grupo B — enlace ESP32 ↔ PSoC

| Código | Qué prueba | Criterio implementado | Qué falla física implica un `FAIL` |
|---|---|---|---|
| B1 | Subida por I2C desde el PSoC maestro al ESP32 esclavo `0x42`. | Observa 1,5 s. Si llegan bytes, da `PASS`, o `WARN` si hubo desbordamientos. Ante silencio total aplica el perfil del PSoC: AUSENTE da `SKIP`, AUTO da `WARN` y PRESENTE da `FAIL`. | Resistencias de polarización de `SDA`/`SCL`, cableado I2C, masa común o PSoC que no arrancó. |
| B2 | Bajada UART ESP32→PSoC y retorno por I2C. | Envía `STATUS` y exige que aumente la cuenta de diagnósticos antes de 800 ms. | Cable o soldadura desde GPIO26 a UART RX del PSoC. La vuelta I2C también participa, aunque B1 ya verificó actividad de subida. |
| B3 | Línea de sincronismo. | Arma el contador, genera diez ciclos y acepta 20 flancos si la interrupción cuenta ambos, o 10 si cuenta uno. Falta de armado, lectura o cuenta distinta da `FAIL`. | Cable o soldadura entre GPIO27 y `SYNC_IN` del PSoC. |
| B4 | Integridad de trama durante toda la corrida. | `PASS` si `bBad == 0` y `badLen == 0`. Si nunca llegó un byte, `SKIP`. `drop` es informativo y no participa. | Corrupción del enlace o de las tramas; el test no localiza una soldadura concreta. |

Si B1 falla, B2 y B3 quedan en `SKIP`, y los grupos C y D no corren. B4 se ejecuta al cierre de la corrida automática, después de C y D.

### Grupo C — infraestructura del PSoC

| Código | Qué prueba | Criterio implementado | Qué falla física implica un `FAIL` |
|---|---|---|---|
| C1 | Identidad, clase de hardware, `Fs`, etapas y canales AMux. | `PASS` si llegan las dos tramas `0x01` y `0x06`. Si no llegan **y el enlace está vivo** (B1 y B2 pasaron), el autotest da `FAIL` y **corta ahí los grupos C y D**. | Casi siempre significa que el PSoC tiene grabado el firmware de **campo**, que no conoce los comandos `0xA0`–`0xA7` y los descarta en silencio. Seguir daría diez `FAIL` por vencimiento de plazo, varios minutos de espera y un diagnóstico equivocado. La solución es grabar el proyecto `AcondicionamientoAnalogicoTest`. |
| C2 | Ausencia de IRQ inesperadas y *HardFault*. | Exige `v0 == 0`, `v1 == 0` y ninguna trampa vista por el ESP32. | No da un diagnóstico físico único; marca una falla interna grave o una IRQ inesperada. |
| C3 | Lectura de los nueve slots de calibración de EEPROM y su CRC de 16 bits. | Uno o más slots válidos dan `PASS`; cero slots da `WARN`, porque es normal en una placa nueva. Falta de respuesta da `FAIL`. | Un `FAIL` es falta de respuesta, no “EEPROM vacía”. |
| C4 | Camino digital E2E con rampa cruda. | Exige al menos 8 lotes, más de 60 muestras y al menos 95 % de transiciones crecientes. | Camino digital de captura o transporte. Si C4 pasa y el analógico falla, la avería queda acotada al frente analógico. |
| C5 | Filtro FIR de hardware en el DFB y su DMA. | Captura 32 lotes por el camino crudo y 32 por el filtrado, sobre la entrada real. `FAIL` si alguna captura falla, si el RMS filtrado es menor que 0,5 cuentas o si el cociente FIR/crudo es mayor que 0,80. RMS crudo menor que 3 cuentas da `SKIP`; en los demás casos da `PASS`. | Filtro, DFB, carga de coeficientes o DMA sin la reducción esperada. No localiza una unión concreta. |
| C6 | Tarjeta SD con FatFs y ruteo SPI nuevo. | Distingue cuatro casos: (1) el PSoC no ve tarjeta: AUSENTE da `SKIP`, AUTO da `WARN` y PRESENTE da `FAIL`; (2) ve la tarjeta pero FAT no monta: `FAIL`; (3) FAT monta pero falla el *self-test* de escritura: `FAIL`; (4) FAT montado y escritura y lectura correctas: `PASS`. Si el PSoC no contesta el reporte, da `FAIL`. | Sin tarjeta, revisar si efectivamente falta o si el zócalo no la detecta. Tarjeta sin FAT montado: formateo FAT32 o `MISO`/`MOSI` cruzados. FAT montado con escritura fallida: sospechar `CS` (`P1[6]`) o `SCK` (`P15[5]`). El caso OK valida montaje, FAT, escritura y lectura. |
| C7 | Nivel de reposo del botón del PSoC. | Hace cinco lecturas separadas 40 ms. Exige respuesta y un nivel estable. | Botón rebotando, pin flotante o conexión del botón; la falta de respuesta también puede ser el enlace. |

C5 no usa la rampa de depuración. En `superMaquina`, la rampa (`CE_CFG_SRC_DEBUG`) y el filtro (`CE_CFG_SRC_FILTER`) son fuentes distintas; seleccionar la rampa y el flujo FIR a la vez no hace pasar la rampa por el DFB.

### Grupo D — cadena analógica

| Código | Qué prueba | Criterio implementado | Qué falla física implica un `FAIL` |
|---|---|---|---|
| D1 | Reposo DC de todos los taps analógicos. | `FAIL` si alguna media obtenida es mayor o igual que +2,3 V o menor o igual que −2,3 V. Una medición individual que no llega no fuerza `FAIL` en este ítem. | Algún tap está contra un riel; el ítem no localiza por sí solo el componente. |
| D1b | `AMuxCapacitor`, 100 nF a `Vss`. | Siempre `INFO` si se puede medir. Estar cerca de `Vss` es lo esperado. | No produce `FAIL`. |
| D2 | Matriz de transferencia DC IDAC→taps. | Barre cada etapa alrededor de su código actual, inicialmente ±20 códigos; si satura, reintenta con la mitad hasta un mínimo de 4. `FAIL` si no hay ningún barrido o si una diagonal falta o tiene módulo menor que 200 uV/código. Una transferencia hacia atrás mayor que 10 % de la diagonal da `WARN`, no `FAIL`. | La etapa que no mueve su propio tap puede tener un resistor abierto, soldadura fría o amplificador operacional quemado. La fuga hacia un tap aguas arriba queda como advertencia. |
| D2.0…D2.3 | Pendientes crudas de cada fila de la matriz. | Siempre `INFO`; informa uV/código para diagnóstico. | No produce `FAIL`. |
| D3 | Asentamiento del último tap. | Compara la lectura a 5 ms con la de 500 ms. Deriva mayor que 50 000 uV da `WARN`; nunca da `FAIL`. Sin medida da `SKIP`. | No produce `FAIL`. Una diferencia grande puede corresponder a un capacitor mal colocado, pero los tiempos largos también son legítimos por el capacitor de 680 uF. |
| D4 | Cociente de ganancia `PGAout` entre 1× y 4×, solamente en GEO. | Si se puede medir, exige un cociente dentro de 4,0 ±25 %, es decir, entre 3,0 y 5,0. Sin pendiente medible a 1× da `FAIL`. Saturación da `SKIP`; HAMMER da `SKIP`. | Camino o selección de ganancia de `PGAout` sin respuesta o con cociente incorrecto. |
| D6.0…D6.3 | Piso de ruido de cada tap y componente de 50 Hz. | `FAIL` con RMS menor que 2 uV, mayor que 200 000 uV o sin medición. Más de 100 000 uV a 50 Hz da `WARN`. En los demás casos da `PASS`. | RMS menor que 2 uV sugiere ADC congelado; mayor que 200 000 uV, etapa oscilando. Captación alta de red sólo produce advertencia. |
| D6b | Carga que el geófono impone a la entrada. | Siempre `INFO`. Informa el valor eficaz (RMS) y la componente de 50 Hz del tap `ch0`. | No produce `FAIL`. El geófono no se puede detectar con un test digital, pero sí se puede medir su efecto: la bobina son unos 375 Ω en paralelo con la red de polarización de 50 kΩ, así que cargar la entrada baja de forma marcada el ruido de `ch0`; con la entrada abierta ese mismo tap levanta mucho más ruido y más captación de red. No se fija un umbral inventado: corriendo el autotest una vez con el geófono y otra sin él, la diferencia queda a la vista y ese sí es un discriminador confiable para esta placa. |
| D8 | Autocalibración y margen de los IDAC. | Debe terminar antes de 180 s. Cada etapa debe responder con `ST_OK` y código IDAC final entre 1 y 254. | Código 0 o 255 indica que no se pudo anular el desplazamiento de esa etapa y que hay una falla en ella. |
| D5 | Coherencia entre las cuatro configuraciones del ADC. | Tras inyectar una excursión, exige cuatro medidas, amplitud de al menos 20 000 uV y dispersión no mayor que 8 %. Si no puede inyectar, si supera ±0,45 V o si la excursión es insuficiente, da `SKIP`. Menos de cuatro medidas o dispersión mayor que 8 % da `FAIL`. | Incoherencia del ADC o de sus rangos; el ítem no localiza un componente físico único. |

El orden real es:

```text
D1 → D2 → D3 → D4 → D6 → D8 → D5
```

D8 va después de D1–D6 porque esos tests necesitan observar la placa sin calibrar. D5 va después de D8 porque necesita un punto conocido y acotado. La calibración deja el último tap cerca de cero; D5 desplaza el IDAC de la última etapa en 40 códigos, restaura el código calibrado al terminar y mantiene la prueba por debajo de ±0,45 V para no recortar la configuración 2, cuyo rango es ±0,512 V.

### Fase interactiva E

La fase E no corre sola. El comando `e` o `inter` ejecuta los cuatro botones del ESP32, el botón del PSoC y el golpe al geófono.

| Código | Qué prueba | Criterio implementado | Qué falla física implica un `FAIL` |
|---|---|---|---|
| E1.0 | Botón UP del ESP32, GPIO34. | Debe observar un nivel bajo dentro de 10 s. | Botón, resistencia, pista o soldadura de GPIO34. |
| E1.1 | Botón DOWN del ESP32, GPIO35. | Debe observar un nivel bajo dentro de 10 s. | Botón, resistencia, pista o soldadura de GPIO35. |
| E1.2 | Botón OK del ESP32, GPIO36. | Debe observar un nivel bajo dentro de 10 s. | Botón, resistencia, pista o soldadura de GPIO36. |
| E1.3 | Botón BACK del ESP32, GPIO39. | Debe observar un nivel bajo dentro de 10 s. | Botón, resistencia, pista o soldadura de GPIO39. |
| E2 | Botón del PSoC, `P2[2]`. | Debe cambiar respecto del primer nivel observado dentro de 10 s. | Botón incorporado, pin o conexión asociada. |
| D7 | Respuesta y polaridad del geófono. | Primero captura 16 lotes para medir el fondo. Define el disparo como dos veces el pico a pico del fondo, o 100 cuentas si el fondo es cero. Luego captura 128 lotes; da `PASS` si el pico supera tanto el disparo como el fondo. Informa el signo de la primera excursión grande, pero ese signo no cambia el veredicto. | Geófono o cadena de señal sin respuesta; el ítem no separa sensor, conexión y frente analógico. Con `hw geo 0` el ítem da `SKIP` y ni siquiera pide el golpe. |

La captura de D7 dura 128 lotes, aproximadamente 1,47 s a 2604 Hz. Para el geófono SM-24, cuya frecuencia natural es 10 Hz y cuyo factor de amortiguamiento es 0,25, cubre alrededor de 14 ciclos.

Los ítems de la fase interactiva también respetan el perfil: con `hw btn 0` el grupo `E1` da `SKIP` sin pedir pulsaciones, y con `hw psoc 0` lo hace `E2`.

## 7. Compilar y grabar ambos lados

### PSoC

Compilar desde la carpeta que contiene el espacio de trabajo `.cywrk`:

```powershell
Push-Location 'C:\Github\Tesis\src\firmware\psoc\AcondicionamientoAnalogicoTest\AcondicionamientoAnalogico.cydsn'
try {
  & 'C:\Program Files (x86)\Cypress\PSoC Creator\4.4\PSoC Creator\bin\cyprjmgr.exe' `
    -wrk 'AcondicionamientoAnalogico.cywrk' `
    -build
} finally {
  Pop-Location
}
```

El archivo generado está en:

```text
AcondicionamientoAnalogicoTest\AcondicionamientoAnalogico.cydsn\CortexM3\ARM_GCC_541\Debug\AcondicionamientoAnalogico.hex
```

Para este firmware de autotest:

```text
Flash usada = 66680 bytes
lastRow     = ceil(66680 / 256) - 1 = 260
filas       = 0..260
```

Antes de grabar, ejecutar `GetPorts` mediante un archivo de comandos con `quit`; `ppcli.exe GetPorts` directo puede quedar esperando en modo interactivo. El nombre de KitProg puede cambiar, así que no hay que reutilizarlo sin consultar:

```powershell
$script = Join-Path $env:TEMP 'psoc_getports.cli'
@('GetPorts', 'quit') | Set-Content -LiteralPath $script -Encoding ASCII
Push-Location 'C:\Program Files (x86)\Cypress\Programmer'
try {
  & .\ppcli.exe "--runfile $($script -replace '\\','/')"
} finally {
  Pop-Location
}
```

Para grabar, reemplazar `$port` por el nombre exacto que devolvió `GetPorts`:

```powershell
$timestamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$log = Join-Path $env:TEMP "psoc_program_selftest_$timestamp.log"
$hex = (Resolve-Path 'C:\Github\Tesis\src\firmware\psoc\AcondicionamientoAnalogicoTest\AcondicionamientoAnalogico.cydsn\CortexM3\ARM_GCC_541\Debug\AcondicionamientoAnalogico.hex').Path.Replace('\', '/')
$port = 'KitProg (CMSIS-DAP/236111)'
$programmer = 'C:/Program Files (x86)/Cypress/Programmer/'
$lastRow = 260

$cmds = New-Object System.Collections.Generic.List[string]
$cmds.Add(('OpenPort "{0}" "{1}"' -f $port, $programmer))
$cmds.Add('SetAcquireMode Reset')
$cmds.Add('SetProtocol 8')
$cmds.Add('SetProtocolConnector 1')
$cmds.Add('SetProtocolClock 152')
$cmds.Add(('HEX_ReadFile "{0}"' -f $hex))
$cmds.Add('DAP_Acquire')
$cmds.Add('PSoC3_GetJtagID')
$cmds.Add('PSoC3_EraseAll')
0..$lastRow | ForEach-Object {
  $cmds.Add(('PSoC3_ProgramRowFromHex 0x00 {0} 0x01' -f $_))
  $cmds.Add(('PSoC3_VerifyRowFromHex 0x00 {0} 0x01' -f $_))
}
$cmds.Add('PSoC3_ProtectAll')
$cmds.Add('PSoC3_VerifyProtect')
$cmds.Add('DAP_ReleaseChip')
$cmds.Add('ClosePort')
$cmds.Add('quit')

$script = Join-Path $env:TEMP "psoc_program_selftest_$timestamp.cli"
$cmds | Set-Content -LiteralPath $script -Encoding ASCII
Push-Location 'C:\Program Files (x86)\Cypress\Programmer'
try {
  & .\ppcli.exe "--runfile $($script -replace '\\','/')" |
    Tee-Object -FilePath $log
} finally {
  Pop-Location
}
```

Una grabación correcta devuelve `0 OK` para cada fila y termina con `0 OK` en `PSoC3_ProtectAll`, `PSoC3_VerifyProtect`, `DAP_ReleaseChip` y `ClosePort`.

### ESP32

Desde la carpeta del esclavo:

```powershell
Set-Location 'C:\Github\Tesis\src\firmware\esp32\Nodo comunicación\slave'
pio run -e slaveTest -t upload
pio device monitor -p COM12 -b 115200
```

El entorno `slaveTest` usa `COM12`, excluye `main.cpp`, incluye `main_selftest.cpp`, define `SLAVE_SELFTEST=1`, conserva `NODE_ID=2` y desactiva la autocalibración al arranque porque D8 debe ejecutarla en su lugar.

Regrabar el ESP32 cuelga al PSoC. Después de cada `upload` hay que ejecutar `ToggleReset` sobre el PSoC mediante KitProg. Reemplazar el nombre de puerto por el que devolvió `GetPorts`:

```powershell
$script = Join-Path $env:TEMP 'psoc_reset.cli'
@(
  'OpenPort "KitProg (CMSIS-DAP/236111)" "C:\Program Files (x86)\Cypress\Programmer\"',
  'SetProtocol 8',
  'SetProtocolClock 152',
  'SetProtocolConnector 1',
  'ToggleReset 0 100',
  'ClosePort',
  'quit'
) | Set-Content -LiteralPath $script -Encoding ASCII
Push-Location 'C:\Program Files (x86)\Cypress\Programmer'
try {
  & .\ppcli.exe "--runfile $($script -replace '\\','/')"
} finally {
  Pop-Location
}
```

## 8. Lectura de la salida

La salida USB del ESP32 usa 115200 bit/s. Al arrancar, espera al PSoC, prueba el enlace y ejecuta la corrida automática. El resultado aparece de tres maneras:

- checklist por USB, con código, nombre, veredicto y detalle medido;
- una línea prefijada con `#JSON` para consumo automático;
- resumen en el OLED con cantidades de `PASS`, `FAIL`, `WARN` y `SKIP`, más el primer `FAIL`.

Ejemplo del contenido del checklist, usando los textos y campos definidos en `main_selftest.cpp`:

```text
[A1] Arranque ESP32                         PASS  POWERON, heap <valor> KB, flash <valor> MB
[B3] Linea SYNC GPIO27 -> P0[4]             PASS  20 flancos en 10 ciclos (ambos flancos)
[D4] Ganancia PGAout 1x vs 4x               PASS  cociente 4.00 (nominal 4.00, tol 25%)
[D8] Auto-calibracion                        PASS  IDAC <códigos por etapa>, 0 etapa(s) al riel, 0 fallada(s), <tiempo> s
```

Los valores entre `<...>` dependen de la placa y de la corrida. El runner reconoce los ítems por el código entre corchetes y parsea la línea `#JSON` como un objeto JSON. También cruza sus conteos y su lista de fallas con el checklist. La forma esperada es:

```text
#JSON {"verdict":"PASS","pass":14,"fail":[],"warn":[],"skip":1,"info":1}
```

Comandos disponibles por la consola USB:

| Comando | Acción |
|---|---|
| `run` o `test` | Repite toda la fase automática. |
| `a`, `b`, `c`, `d` | Ejecuta solamente ese grupo. |
| `tap` o `golpe` | Ejecuta solamente D7. |
| `e` o `inter` | Ejecuta botones y golpe al geófono. |
| `probe` | Informa el estado del enlace con el PSoC. |
| `hw` | Muestra el perfil de hardware presente. |
| `hw <parte> <0|1|2>` | Cambia una parte del perfil y la guarda en NVS. |
| `oled si` o `oled no` | Registra la confirmación visual del OLED. |
| `help` o `?` | Lista los comandos. |

## 9. Diagnóstico

Si la corrida contiene al menos un `FAIL` o un `WARN`, después del resumen el firmware imprime:

```text
DIAGNOSTICO — donde mirar:
```

`DIAG_HINTS` agrupa por prefijo: por ejemplo, un problema en D6.2 usa la pista D6 y uno en D2.1 usa D2. Se imprime una línea por prefijo afectado que tenga una entrada en la tabla. Los textos definidos en `main_selftest.cpp` son:

| Código | Dónde mirar |
|---|---|
| A1 | Alimentacion del ESP32. Un reset por BROWNOUT es regulador o condensador de desacople insuficiente. |
| A2 | OLED: CS=33 DC=16 RST=17 SCK=18 MOSI=23, y 3V3/GND del modulo. No es detectable por software: confirmar mirando la pantalla. |
| A3 | Pull-ups de 10 k a 3V3 en GPIO34/35/36/39. Esos pines son de solo entrada y NO tienen pull-up interno. |
| A4 | Radio del ESP32. Si A1 tambien fallo, mirar primero la alimentacion. |
| B1 | Enlace de subida: pull-ups de SDA(21)/SCL(22) a 3V3, GND comun, y que el PSoC este programado y alimentado. |
| B2 | Enlace de bajada: ESP GPIO26 -> PSoC Rx P15[0]. Si B1 pasa y B2 no, el problema esta en ESE cable, no en el I2C. |
| B3 | Linea de sincronismo: ESP GPIO27 -> PSoC SYNC_IN P0[4]. |
| B4 | Tramas corruptas: ruido en el bus I2C, pull-ups de valor muy alto, o cables largos. Ojo que `drop` NO cuenta como corrupcion. |
| C1 | El PSoC contesta pero no se identifica: firmware de PSoC viejo, sin los comandos de autotest. |
| C2 | Falla interna del PSoC: IRQ inesperada o HardFault. Anotar el numero de IRQ del detalle. |
| C4 | Camino digital de captura: superMaquina, DMA o el enlace. Si C4 falla no hay que creerle a ninguna medida analogica. |
| C5 | Filtro FIR: bloque DFB o su DMA. El resto de la captura funciona. |
| C6 | SD: CS=P1[6] SCK=P15[5] MOSI=P2[5] MISO=P15[4], mas alimentacion del zocalo. Son cuatro puertos distintos en la placa nueva. |
| C7 | Pulsador del PSoC en P2[2] (el de la placa CY8CKIT-059). |
| D1 | Algun tap contra un riel. Mirar la fila D2 correspondiente para saber que etapa. |
| D2 | Etapa analogica sin respuesta: resistencia abierta, soldadura fria o amplificador operacional. El detalle dice que etapa. |
| D4 | Etapa PGAout o su red de realimentacion. |
| D5 | Referencia del ADC o el escalado de rangos. |
| D6 | Ruido: RMS casi nulo es ADC congelado; RMS enorme es una etapa oscilando. Mucho 50 Hz es masa o apantallamiento. |
| D7 | Geofono: bornera, continuidad de la bobina y polaridad del par. |
| D8 | Calibracion: un IDAC contra el riel significa que el offset de esa etapa no se puede anular. Mirar la resistencia de 30 k de esa referencia y su etapa. |
| E1 | Pulsadores del ESP y sus pull-ups. |
| E2 | Pulsador del PSoC. |

El bloque termina aclarando que los `WARN` no reprueban la placa y los `FAIL` sí.

## 10. Runner de PC (`autotest_runner.py`)

El runner abre el USB del esclavo o lee una captura guardada, parsea el checklist y la línea `#JSON`, y compara ambos con el resumen. Además de detectar `FAIL`, exige los ítems obligatorios: una corrida que se corta después de varios `PASS` queda marcada como incompleta. Si el firmware emite los marcadores `C*` y `D*` porque el PSoC no contestó, reconoce una corrida parcial deliberada en vez de confundirla con una corrida trunca. Al terminar escribe un archivo JSON de evidencia con los ítems, conteos, fallas, advertencias, cobertura, problemas de consistencia y metadatos de la ejecución.

Opciones:

| Opción | Acción |
|---|---|
| `--self-test` | Ejecuta 23 comprobaciones offline del parser y del *gate*. No importa `pyserial`, no abre un puerto COM y no requiere `--output`. |
| `--from-file <captura>` | Evalúa una captura existente en vez de abrir el puerto serie. |
| `--port <COM>` | Selecciona el puerto USB del esclavo; el valor predeterminado es `COM12`. |
| `--output <archivo.json>` | Escribe la evidencia JSON. Es obligatorio salvo con `--self-test`. |
| `--timeout <segundos>` | Fija el tiempo máximo de recolección. El valor predeterminado es 900 s. |
| `--no-trigger` | No manda `run`; se engancha a la corrida automática iniciada durante el arranque. |

Códigos de salida:

| Código | Significado |
|---:|---|
| `0` | No hay `FAIL` y la cobertura es completa. |
| `1` | Hay al menos un `FAIL` o la corrida quedó trunca o inconsistente. |
| `3` | No hay `FAIL`, pero la cobertura es parcial porque el PSoC no contestó. |

Al abrir el puerto, el runner configura DTR y RTS en `False`. Los valores predeterminados de `pyserial` pueden resetear el ESP32 al abrir el COM y cortar la corrida que se quiere registrar.

## 11. Nada se congela

Todas las esperas del autotest están acotadas con `millis()`. Los topes que protegen los puntos donde una falla de hardware podría dejar una espera abierta son:

| Espera | Tope |
|---|---:|
| El PSoC llega a `ARMED` antes de una captura. | 2 s |
| Respuesta al comando `STATUS`. | 800 ms |
| Autocalibración D8. | 180 s |
| Cada pulsación de la fase interactiva. | 10 s |
| Soltar cada botón del ESP32 después de pulsarlo. | 3 s |
| Escaneo WiFi asíncrono. | 6 s |

Si un botón sigue en bajo después de 3 s, el firmware lo reporta como pin pegado en vez de esperar indefinidamente. El escaneo WiFi usa la modalidad asíncrona y, si no termina dentro de 6 s, se cancela y la corrida continúa.

También imprime una línea al entrar en cada grupo. Durante D2 informa qué etapa está barriendo y, al comenzar D8, avisa que está calibrando y recuerda el tope de 180 s. D2 y D8 son los tramos largos, por lo que esas líneas permiten distinguir trabajo en curso de un cuelgue.

## 12. Limitaciones conocidas

1. No hay inyección de barrido AC. `0xA7` mide el piso de ruido y la componente de 50 Hz, pero este firmware no reproduce una forma de onda mediante los IDAC. La onda de Ricker pertenecía a proyectos históricos que usaban un par diferencial de VDAC8; ese par ya no existe en este diseño.
2. No hay medición de batería.
3. El firmware no puede resetear el PSoC. El reset posterior a la grabación del ESP32 se hace con `ToggleReset` por KitProg.
4. La primera versión (`v1`) de los umbrales está elegida para fallar solamente ante fallas gruesas. Hay que ajustarla con medidas de placas reales; no sirve como control fino de tolerancias.
5. El SSD1306 conectado por SPI no se puede detectar por software porque el bus es de ida sola. `Adafruit_SSD1306::begin()` sólo confirma que pudo reservar el *framebuffer*; devuelve `false` por falta de RAM del ESP32, no por un OLED ausente, sin alimentación o averiado. A2 queda en `INFO` y requiere confirmación visual con `oled si` o `oled no`.
6. A3 puede dar un falso `PASS`. Los GPIO34, GPIO35, GPIO36 y GPIO39 son de solo entrada y no tienen resistencia de polarización interna. Un pin flotante puede sostener un nivel alto durante los 200 ms por carga residual, de modo que ese nivel no demuestra que esté el *pull-up*. A3 sí detecta un pin pegado en bajo, compatible con resistencia de 10 kΩ ausente, pulsador en corto o puente a masa. Conviene confirmar los *pull-ups* con un tester.
