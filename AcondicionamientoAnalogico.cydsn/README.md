# PSoC 5LP — Nodo geófono (variante ESP / UART)

Firmware del PSoC que digitaliza la señal del geófono y la envía **en crudo (raw)**
al ESP esclavo por **UART**. Es la variante inalámbrica de `DiferencialToSingleEnded.cydsn`
(esa otra es solo USB↔PC y **no** se toca).

## Rol en la cadena
```
MATLAB ──USB──> ESP maestro ──ESP-NOW──> ESP esclavo ──UART──> PSoC
                                                  <──UART──  (muestras raw)
```
El PSoC recibe **N** (nº de lotes) y **VDAC** (calibración) por UART, queda armado,
y arranca a muestrear **solo cuando llega un flanco de subida en `SYNC_IN`** (lo
levanta el ESP esclavo). Muestrea N lotes de 30 muestras y se detiene solo.

## Protocolo UART
**Datos PSoC → ESP** (frame de 95 bytes, raw 24-bit):
```
[0xAB][n=30][seq_lo][seq_hi] + 30×3 bytes (raw LE) + [crc XOR]
```
**Comandos ESP → PSoC** (checksum XOR):
- 1 parámetro `[0xAB][cmd][p][cmd^p]`:
  `0xA6` PGA · `0xA9` PGAvdac · `0xAA` VDAC · `0xB1` pre-start/arm · `0xB3` debug rampa
- 2 parámetros `[0xAB][0xA3][n_lo][n_hi][cmd^n_lo^n_hi]`: `0xA3` set N (16 bits)

## Máquina de estados
`PSOC_IDLE → (0xB1 arma) → PSOC_ARMED → (flanco SYNC) → PSOC_SAMPLING → (N lotes) → PSOC_IDLE`
El arranque es **siempre por el pin** (`isr_SyncIn`), nunca por software.

## ⚠️ Pendiente en PSoC Creator (esquemático)
`main.c` ya usa los nombres finales; falta en el TopDesign (ver cabecera de `main.c`):
1. Renombrar el componente UART `UART_PC` → **`UART`**.
2. **Quitar** el componente `Filter` (FIR) y su ISR `isr_Filter`.
3. **Quitar** el `SPI Slave (SPIS_1)` y el pin `DATA_READY`.
4. Conservar `SYNC_IN` (input, *rising & falling*, ISR `isr_SyncIn`).
5. Cablear `UART.TX→RX(ESP)` y `UART.RX←TX(ESP)`; baud = `PSOC_UART_BAUD`
   del esclavo (115200). TX buffer ≥ 96, RX buffer ≥ 16.

## Archivos
- `main.c` — todo el firmware (init analógico, ISRs, RX comandos, TX lotes).
  La cabecera documenta los cambios de esquemático pendientes.

## Debug
- `0xB3` activa una **rampa** en vez del ADC (se ve un diente de sierra en MATLAB).
- El LED parpadea al recibir comandos de configuración.
