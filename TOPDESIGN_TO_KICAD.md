# Notas para transferir TopDesign a KiCad

Este documento conserva los hallazgos usados para llevar el diseño activo
`AcondicionamientoAnalogico` a la placa portadora basada en los development
kits CY8CKIT-059 y ESP32 DevKitC.

Revisado el 2 de septiembre de 2026 contra `cyfitter.h` del build vigente y
contra el netlist de la portadora JitX. La revisión anterior de este
documento tenía ocho señales mal asignadas; ver §"Errores corregidos".

## Fuentes de verdad

Usar las fuentes en este orden:

1. `AcondicionamientoAnalogico.cydsn/Generated_Source/PSoC5/cyfitter.h`:
   asignación física generada por el fitter. Es el dato eléctricamente
   autoritativo del lado del firmware.
2. `PCBs/JitX/designs/tesis_carrier.main.TesisCarrier/cache/netlist.json` y
   `bom/bom.json`: topología y valores de la próxima placa portadora. Su
   pinout ya es el vigente; su red de referencia todavía no está construida.
3. `AcondicionamientoAnalogico.cydsn/TopDesign/TopDesign.cysch`: topología e
   instancias del TopDesign. Es lo que está armado hoy: la portadora JitX
   todavía no se fabricó (§"Divergencias").
4. Los archivos `Generated_Source/PSoC5/<Pin>.h`: modo y API de cada pin.
5. El firmware ESP32 actual: asignación del otro extremo de UART y `SYNC_IN`.
6. Capturas o comentarios antiguos: sólo como apoyo visual; están
   desactualizados.

`PCBs/KiCad/` es un intento manual anterior, sin cablear y con el pinout viejo.
No usarlo como referencia; ver `PCBs/KiCad/README.md`.

## Extracción sin interfaz gráfica

`Generated_Source` no contiene los valores de las resistencias y capacitores
dibujados en TopDesign. Esos valores están serializados dentro de
`TopDesign.cysch`, un archivo binario `CyDesigner.Common.Base.CyArchive`.
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
Los valores repetidos pueden aparecer una sola vez en el archivo.

Cadenas de valor presentes hoy en el archivo: `50k`, `43k`, `680uF`, `47k`,
`27pF`, `150pF`, `6.8k`, `2K`, `31.7`, `27k`, `15nF`, `30k`, `12k`, `150k`,
`47nF`, `3.3nF`, `15k`, `1uF`, `100nF`, `2k2`.

## Pinout físico actual del PSoC

De `cyfitter.h`. Coincide pin a pin con `netlist.json` de la portadora.

| Señal TopDesign | Puerto PSoC | Red en la portadora |
|---|---|---|
| `INp` | P1[2] | `IN_P` |
| `INn` | P1[7] | `IN_N` |
| `Vref` | P3[6] | `VREF` |
| `SEo` | P2[7] | `SEo` |
| `BPm` | P3[2] | `BPm` |
| `BPo` | P3[7] | `BPo` |
| `SUMm` | P0[5] | `SUMm` |
| `SUMo` | P0[0] | `SUMo` |
| `PGAo` | P2[6] | `PGAo` |
| `LPm` | P0[3] | `LPm` |
| `LPo` | P0[1] | `LPo` |
| `Vref_PGA` | P3[0] | `PGA_REF` |
| `Vref_BP` | P3[1] | `BP_REF` |
| `Vref_ADDER` | P0[6] | `SUM_REF` |
| `Vref_LP` | P0[7] | `LP_REF` |
| `AMuxCapacitor` | P15[3] | `AMUX_CAP` |
| `SYNC_IN` | P0[4] | `PSOC_SYNC_IN` |
| `Rx` | P15[0] | `ESP_TX_TO_PSOC_RX` |
| `I2Cp_SDA` | P2[1] | `PSOC_I2C_SDA` |
| `I2Cp_SCL` | P2[3] | `PSOC_I2C_SCL` |
| `SPIp_CS` | P1[6] | `SD_CS` |
| `SPIp_SCK` | P15[5] | `SD_SCK` |
| `SPIp_MOSI` | P2[5] | `SD_MOSI` |
| `SPIp_MISO` | P15[4] | `SD_MISO` |
| `Button` | P2[2] | **sin conectar** |

No hay pin de `LED` ni de `Tx`: ambos se eliminaron del diseño. La subida
PSoC → ESP32 va por I2C; la UART quedó sólo como RX.

`Button` sigue instanciado en el TopDesign sobre P2[2], pero la portadora no
lo cablea (los cuatro pulsadores cuelgan del ESP32). Ese pin entra flotante a
un debouncer.

Para comprobar las representaciones a la vez:

```powershell
.\verify_carrier_pinout.ps1
```

El script compara `cyfitter.h`, los GUID activos/bloqueados del `.cydwr` y el
último estado de compilación. Es importante conservar los GUID activos: PSoC
Creator mantiene entradas históricas duplicadas para `PGAo`, `LPm`, `LPo` y
`Vref_BP`.

## Red externa

Valores del dibujo del TopDesign —que es la placa que existe— contrastados
con el BOM de la portadora (`bom/bom.json`). Coinciden salvo donde se indica.

| Bloque | Elementos |
|---|---|
| Polarización de entrada | `R2 = R3 = 50 k` entre `IN_P`/`IN_N` y `VREF`; el geófono (J4) va en paralelo entre `IN_P` e `IN_N` |
| Pasa-banda | `R4 = 43 k` desde `SEo`; `C1 = 680 µF` a `BPm`; realimentación `R5 = 47 k` ∥ `C2 = 27 pF` ∥ `C17 = 150 pF` entre `BPm` y `BPo` |
| Sumador | `R6 = 6.8 k` de `SEo` a `SUMm`; `R7 = 6.8 k` de `BPo` al trimmer; `RV1` (Bourns 3296W, cursor y un extremo unidos a `SUMm`); realimentación `R8 = 27 k` ∥ `C3 = 15 nF` |
| Pasa-bajos | `R15 = 30 k` de `PGAo` al nodo interno; `C4 = 47 nF` de ese nodo a `VREF`; `R10 = 150 k` a `LPo`; `R9 = 12 k` a `LPm`; `C5 = 3.3 nF` entre `LPm` y `LPo` |
| Referencias por etapa | En el TopDesign, `R11..R14 = 15 k` desde cada `Vref_XX` hasta `Vref` (P3[6]); en la portadora, `30 k` hasta `VREF_2V048`. En ambos casos `C8,C10,C12,C14 = 1 µF` y `C9,C11,C13,C15 = 100 nF` a masa en cada pin, que es el único camino a Vss: contra masa esos pines dan abierto |
| Referencia externa | **Sólo en la portadora**, no en la placa actual: `U1` AMS1117-ADJ, `R22 = 1.0 k` (VOUT→ADJ), `R23 = 620` (ADJ→GND), `C18 = 22 µF`, `C19 = 10 µF` |
| `VREF` (P3[6]) | `C6 = 1 µF` + `C7 = 100 nF` a masa, y la guarda del conector de geófono. Lo excita el buffer interno `OPAref`; no lo alimenta el regulador |
| AMux | `C16 = 100 nF` a masa en P15[3] |
| I2C | `R1 = R17 = 4.7 k` a +3V3 en SDA y SCL |
| Interfaz de usuario | `SW2..SW5` con `R18..R21 = 10 k`, y OLED `A3`, todos sobre el ESP32 |

## Divergencias entre el TopDesign y la portadora

La placa que existe hoy es la del TopDesign; la portadora JitX no se fabricó
(su `.kicad_pcb` está vacío). Estas diferencias son para cuando se fabrique:

1. **Ramas de referencia.** El TopDesign dibuja `15 k` desde cada `Vref_XX`
   hasta el mismo nodo `Vref` (P3[6]), y contra masa sólo hay capacitores.
   La portadora lleva `30 k` y los cuatro van a `VREF_2V048`, la salida de un
   AMS1117-ADJ propio; ahí `Vref_XX` contra P3[6] pasaría a dar abierto.
2. **Pull-ups de I2C.** El TopDesign dibuja `2k2`; la portadora lleva `4.7 k`.
3. **Divisor del regulador.** Con `R22 = 1.0 k` arriba y `R23 = 620` abajo, el
   AMS1117-ADJ entrega `1.25 · (1 + 1000/620) ≈ 3.27 V`, no los 2.048 V que
   sugiere el nombre de la red `VREF_2V048`. Para 2.048 V los dos valores van
   al revés. Verificar con voltímetro antes de asumir el valor.

## Errores corregidos en la revisión anterior de este documento

| Señal | Decía | Es |
|---|---|---|
| `INp` | P1[7] | **P1[2]** |
| `INn` | P1[6] | **P1[7]** |
| `BPm` | P3[3] | **P3[2]** |
| `PGAo` | P3[4] | **P2[6]** |
| `Vref_ADDER` | P0[7] | **P0[6]** |
| `Vref_LP` | P0[6] | **P0[7]** |
| `Rx` | P2[0] | **P15[0]** |
| `SPIp` CS/SCK/MOSI/MISO | P2[3..6] | **P1[6] / P15[5] / P2[5] / P15[4]** |
| `LED` | P1[2], con `R16 = 390` | **eliminado**; P1[2] es hoy `INp` |
| `Button` | P12[4] | **P2[2]**; la portadora no lo cablea (los pulsadores pasan al ESP32) |
| Sumador | `R7 = 47 k` | **`R7 = 6.8 k`**; el 47 k es `R5` |
| Referencias | `30 k` a `Vref` | **`15 k` a `Vref`** en el TopDesign; `30 k` a `VREF_2V048` recién en la portadora (ver §Divergencias) |
| `C17` | no figuraba | `150 pF` ∥ `R5` |

## Verificación con óhmetro

`debug_analogico/red_analogica.py` genera la tabla de resistencias esperadas
entre todos los pares de pines analógicos, incluyendo GND y +5 V, a partir de
esta misma red. Ver `debug_analogico/README.md`.
