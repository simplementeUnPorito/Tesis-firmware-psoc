# Notas para transferir TopDesign a KiCad

Este documento conserva los hallazgos usados para llevar el diseño activo
`AcondicionamientoAnalogico` a la placa intermedia basada en los development
kits CY8CKIT-059 y ESP32 DevKitC.

## Fuentes de verdad

Usar las fuentes en este orden:

1. `AcondicionamientoAnalogico.cydsn/TopDesign/TopDesign.cysch`: topología,
   instancias y valores actuales del TopDesign.
2. `AcondicionamientoAnalogico.cydsn/Generated_Source/PSoC5/cyfitter.h`:
   asignación física generada por el fitter.
3. Los archivos `Generated_Source/PSoC5/<Pin>.h`: modo y API de cada pin.
4. El firmware ESP32 actual: asignación del otro extremo de UART y `SYNC_IN`.
5. Capturas o comentarios antiguos: sólo como apoyo visual; pueden estar
   desactualizados.

`Generated_Source` no contiene los valores de las resistencias y capacitores
dibujados en TopDesign. Esos valores están serializados dentro de
`TopDesign.cysch`, un archivo binario `CyDesigner.Common.Base.CyArchive`.

## Extracción sin interfaz gráfica

El script `extract_topdesign_strings.ps1` recupera las cadenas UTF-16
almacenadas en el archivo sin modificarlo:

```powershell
.\extract_topdesign_strings.ps1
.\extract_topdesign_strings.ps1 -ContextFor '^R_11$|^27k$'
.\extract_topdesign_strings.ps1 -All
```

El archivo emplea referencias internas para reutilizar cadenas. Por eso, una
cadena que aparece cerca de una instancia suele identificar su propiedad, pero
la ausencia de un valor junto a otra instancia no significa que no lo tenga.
Los valores repetidos pueden aparecer una sola vez en el archivo. Para una
transferencia definitiva se debe contrastar la proximidad de las cadenas con
la topología y el nombre de red.

Asociaciones recuperadas directamente del archivo actual:

| Instancia PSoC | Valor | Confianza |
|---|---:|---|
| `R_5` | 12 kOhm | alta |
| `R_6` | 150 kOhm | alta |
| `R_7` | 30 kOhm | alta |
| `R_10` | 43 kOhm | alta |
| `R_11` | 27 kOhm | alta |
| `R_18` | 6.8 kOhm | alta |
| `C_3` | 1 uF polarizado | alta |
| `C_4` | 3.3 nF | alta |
| `C_5` | 47 nF | alta |
| `C_6` | 100 nF | alta |
| `C_7` | 15 nF | alta |
| `C_16` | 27 pF | alta |
| `R_12` | potenciómetro 2 kOhm, posición 31.7 % | alta |

El contraste entre esas cadenas, la topología visible del TopDesign y las
capturas actuales dio la siguiente transferencia externa completa:

| Bloque | Elementos KiCad |
|---|---|
| Polarización de entrada | `R2 = R3 = 50 kOhm` hacia `VREF` |
| Pasa-banda | `R4 = 43 kOhm`, `C1 = 680 uF`, `R5 = 47 kOhm`, `C2 = 27 pF` |
| Sumador | `R6 = 6.8 kOhm`, `R7 = 47 kOhm`, `RV1 = 2 kOhm (31.7 % nominal)`, `R8 = 27 kOhm`, `C3 = 15 nF` |
| Pasa-bajos | `C4 = 47 nF`, `R9 = 12 kOhm`, `R10 = 150 kOhm`, `C5 = 3.3 nF` |
| Referencia principal | `C6 = 1 uF`, `C7 = 100 nF` |
| Referencias por etapa | `R11..R14 = 30 kOhm`; `C8,C10,C12,C14 = 1 uF`; `C9,C11,C13,C15 = 100 nF` |
| AMux | `C16 = 100 nF` |
| Pulsador | `SW1`, `R15 = 30 kOhm` a masa |
| Indicador | `R16 = 390 Ohm`, `D1 = LED verde`, activo alto |

La cadena `150 pF` también existe en el archivo binario, pero no pertenece a
la topología externa activa identificada. No se trasladó a KiCad.

## Pinout físico actual del PSoC

La siguiente tabla proviene del `cyfitter.h` generado y de la asignación
visible del proyecto actual:

| Señal TopDesign | Puerto PSoC | CY8CKIT-059 |
|---|---|---|
| `AMuxCapacitor` | P1[4] | J1.21 |
| `BPm` | P3[3] | J2.22 |
| `BPo` | P3[7] | J2.18 |
| `Button` | P12[4] | J1.12 |
| `INn` | P1[6] | J1.23 |
| `INp` | P1[7] | J1.24 |
| `LED` | P1[2] | J1.19 |
| `LPm` | P0[3] | J2.8 |
| `LPo` | P0[1] | J2.10 |
| `PGAo` | P3[4] | J2.21 |
| `Rx` | P2[0] | J1.1 |
| `SEo` | P2[7] | J1.8 |
| `SPIp_CS` | P2[3] | J1.4 |
| `SPIp_SCK` | P2[4] | J1.5 |
| `SPIp_MOSI` | P2[5] | J1.6 |
| `SPIp_MISO` | P2[6] | J1.7 |
| `SUMm` | P0[5] | J2.6 |
| `SUMo` | P0[0] | J2.11 |
| `SYNC_IN` | P1[5] | J1.22 |
| `Tx` | P12[7] | J1.9 |
| `Vref` | P3[6] | J2.19 |
| `Vref_ADDER` | P0[7] | J2.4 |
| `Vref_BP` | P3[1] | J2.24 |
| `Vref_LP` | P0[6] | J2.5 |
| `Vref_PGA` | P3[0] | J2.25 |

Este pinout está bloqueado en `AcondicionamientoAnalogico.cydwr`, fue
recompilado con éxito el 31 de julio de 2026 y coincide con el símbolo y
footprint CY8CKIT-059 del proyecto KiCad. El dato eléctricamente autoritativo
sigue siendo `cyfitter.h`.

Para comprobar las tres representaciones a la vez:

```powershell
.\verify_carrier_pinout.ps1
```

El script compara `cyfitter.h`, los GUID activos/bloqueados del `.cydwr` y el
último estado de compilación. Es importante conservar los GUID activos: PSoC
Creator mantiene entradas históricas duplicadas para `PGAo`, `LPm`, `LPo` y
`Vref_BP`.

## Interfaz PSoC - ESP32 actual

| Función | PSoC | ESP32 esclavo |
|---|---|---|
| PSoC TX, colector/drenador abierto | P12[7], J1.9 | GPIO25 RX |
| PSoC RX | P2[0], J1.1 | GPIO26 TX |
| Disparo síncrono | P1[5], J1.22 | GPIO27 |
| Sincronismo externo opcional | — | GPIO32 |

La línea de TX del PSoC necesita pull-up hacia 3.3 V del ESP32. El proyecto
KiCad usa 4.7 kOhm. Los comentarios antiguos que muestran otros GPIO o 2.7
kOhm no son autoritativos.

## Bloques físicos que deben aparecer en KiCad

- Conector de geófono y polarización de entrada.
- Red externa alrededor de `BPm`/`BPo`.
- Sumador externo entre `SEo`, `BPo`, `SUMm` y `SUMo`.
- Filtro externo alrededor de `PGAo`, `LPm` y `LPo`.
- Cuatro ramas de referencia `Vref_PGA`, `Vref_BP`, `Vref_ADDER` y
  `Vref_LP`.
- Capacitor externo de `AMuxCapacitor`.
- Pulsador y resistencia de polarización.
- Conector microSD con SPI.
- UART, `SYNC_IN`, pull-up y masas compartidas entre los development kits.

Los PGA, amplificadores operacionales, IDAC, AMux, LPF, ADC, DMA, filtro DFB,
temporizadores, debouncer y controlador de señales son bloques internos del
PSoC. En KiCad se documentan con notas o bloques funcionales, pero no deben
crear componentes físicos adicionales en la BOM.

## Criterio de reasignación y compilación

Un mapa anterior ya no cerraba el ruteo analógico del TopDesign actual. Si se
dejaban todos los pines libres, el fitter podía colocar `Tx` en P15[7], que es
USB D- y no está expuesto en J1/J2. El procedimiento reproducible fue:

1. fijar `Tx` en P12[7] y SPI contiguo en P2[3..6];
2. dejar al fitter resolver las demás restricciones analógicas;
3. confirmar que todos los resultados estaban expuestos en J1/J2;
4. bloquear exactamente la solución encontrada;
5. recompilar de nuevo con cero pines libres.

La primera solución válida colocó `PGAo` en P2[2], pero ese pin está unido al
pulsador de usuario del CY8CKIT-059. Se volvió a rutear y se fijó `PGAo` en
P3[4]/J2.21 para evitar que una pulsación accidental cortocircuite la salida
analógica. P2[1], que maneja el LED azul onboard, también queda sin usar por la
carrier.

El kit también marca P0[2], P0[3], P0[4] y P3[2] como GPIO con capacitores de
bypass ADC onboard. Se pudo mover `SYNC_IN` de P0[2] a P1[5]/J1.22 y `BPm` de
P3[2] a P3[3]/J2.22. El fitter no encontró solución al intentar mover `LPm`
de P0[3] a P3[5]; por eso `LPm=P0[3]/J2.8` es una restricción consciente del
prototipo y debe contrastarse en la respuesta analógica durante bring-up.

Comando utilizado:

```powershell
& 'C:\Program Files (x86)\Cypress\PSoC Creator\4.4\PSoC Creator\bin\cyprjmgr.exe' `
  -wrk .\AcondicionamientoAnalogico.cydsn\AcondicionamientoAnalogico.cywrk `
  -prj AcondicionamientoAnalogico `
  -build
```

La compilación final usa 59 880 bytes de flash (22.8 %) y 51 952 bytes de SRAM
(79.3 %, stack 2 048 bytes, heap 128 bytes). Permanece la advertencia de timing
`sta.M0019`; no impidió generar el `.hex`, pero debe revisarse antes de cerrar
la placa final.
