# PSoC 5LP — AcondicionamientoAnalogico

Firmware unificado para los nodos **GEO** (geófono) y **HAMMER** (martillo). El tipo
de hardware se selecciona automáticamente en tiempo de compilación según los
componentes del TopDesign (`PGAgain` → GEO, `PGA` → HAMMER). No hay que tocar
`main.c` al cambiar entre proyectos.

## Rol en la cadena

```
MATLAB/Web ──USB──> ESP maestro ──ESP-NOW──> ESP esclavo ──UART──> PSoC
                                                    <──UART──  (muestras raw)
```

El PSoC recibe N lotes y configuración por UART, queda armado, y arranca a
muestrear **solo cuando llega un flanco de subida en `SYNC_IN`** (lo levanta el
ESP esclavo). Captura N lotes de 30 muestras en RAM sin UART durante la ventana
crítica; después los envía al ESP.

## Cadena analógica

### GEO (4 etapas)
```
Geófono → HPF → PGAgain (0-24dB) → ADC_DelSig 18bit
               VDAC_ref_PGA, VDAC_ref_BP, VDAC_Ref_Adder, VDAC_ref_LP
               (calibración por PI: PGA → BP → ADDER → LP)
```

### HAMMER (2 etapas)
```
Señal → PGA + Opa_ref_PGA → Opa_LP → ADC_DelSig 18bit
         VDAC_PGA, VDAC_LP
         (calibración por PI: PGA → LP)
```

## Máquina de estados

`PSOC_IDLE → (0xB1 arm) → PSOC_ARMED → (flanco SYNC_IN) → PSOC_SAMPLING → PSOC_IDLE`

Durante calibración: `→ PSOC_CALIBRATING → PSOC_IDLE`

## Protocolo UART

### TX (PSoC → ESP)

| Frame | Bytes | Descripción |
|-------|-------|-------------|
| Datos raw | 95 | `[0xAB][n=30][seq_lo][seq_hi] + 30×3 bytes LE + [CRC XOR]` |
| Ping | 4 | `[0xAB][0xC0][0x00][0xC0]` |
| CFG ACK | 5 | `[0xAB][0xC2][cmd][val][cs]` |
| FS report | 5 | `[0xAB][0xC3][fs_lo][fs_hi][cs]` — reporta 1020 Hz |
| Diag event | 6 | `[0xAB][0xC4][event][value][state][cs]` |

### RX (ESP → PSoC)

| Comando | Bytes | Descripción |
|---------|-------|-------------|
| `0xA3` setN | 5 | `[0xAB][0xA3][n_lo][n_hi][cs]` — número de lotes (16 bits) |
| `0xA5` status | 4 | Sondeo de estado |
| `0xA6` PGA | 4 | Ganancia PGA (código 0-8) |
| `0xA9` PGAvdac | 4 | PGA+VDAC combinado (legacy) |
| `0xAA` VDAC | 4 | VDAC (legacy, mantiene sombra) |
| `0xB1` arm | 4 | Arma el PSoC (espera SYNC_IN) |
| `0xB3` debug | 4 | Activa rampa debug (diente de sierra) |
| `0xB4` start-now | 4 | Arranque inmediato sin esperar SYNC |
| `0xB5` calibrar | 4 | Inicia calibración PI async |
| `0xB6` save EEPROM | 4 | Guarda DACs de calibración en EEPROM |
| `0xB7` select stream | 4 | `0`=crudo, `1`=FIR hardware |
| `0xB8` ADC snapshot | 4 | Diagnóstico ADC por etapa |
| `0xB9` blink LED | 4 | Titila el LED de identificación ~8 s |
| `0xBA` ADC config | 4 | `1`=±2.5 V, `2`=±0.512 V; ambos a 1020 Hz |
| `0xC1` pong | 4 | Respuesta al ping del PSoC |

## DMA y filtro de hardware

El PSoC usa tres canales DMA y un registro de control `Reg_Select` para enrutar:

- **Modo crudo** (default): `ADC_DelSig → DMA_DelSig_RAM → g_dma_raw_buf[3]`
- **Modo FIR** (`0xB7 param=1`): `ADC_DelSig → Filter (DFB) → DMA_Filter_RAM → g_dma_filt_buf[3]`
- **Calibración**: usa `FIR_calibration.h` en el Filter para leer DC limpio; restaura `FIR_adquisicion.h` al terminar.

Los coeficientes FIR se definen en:
- `FIR_adquisicion.h` / `FIR_adquisicion.c` — captura normal
- `FIR_calibration.h` / `FIR_calibration.c` — calibración PI
- `filter_coeffs.h` — `FILTER_FIR_NTAPS` (fuente única de verdad)

## Calibración PI

Motor en `calibration.c/.h`. Arquitectura por etapas independientes con PI:

1. Verifica DACs actuales contra deadband de tolerancia.
2. Si todas las etapas ya están en rango → responde CAL OK sin mover VDACs.
3. Si alguna etapa está fuera → corre el PI desde el DAC sembrado (no desde el centro).
4. Cada etapa: objetivo 0 counts (diferencial), PI converge cuando N muestras consecutivas
   dentro del deadband (1024 muestras = ~1.00 s a 1020 Hz para GEO_BP/LP).

Parámetros por etapa en `calibration_tables_geo_*.h` / `calibration_tables_hammer_*.h`.

## EEPROM (psoc_nv.h)

Layout v2: **9 slots** (códigos PGA 0..8), 16 bytes por slot.

```
[0] magic=0xCA  [1] version=0x02  [2] hw_class  [3] pga_code
[4] stage_count  [5] valid_mask  [6..9] cal_dac[4]
[10..13] reserved  [14..15] CRC-16 LE
```

Al arrancar y al cambiar de ganancia PGA, el PSoC intenta cargar el slot
correspondiente. Si no existe o tiene CRC inválido, usa los valores nominales
de tabla. `PSOC_CMD_SAVE_EEPROM` solo guarda si la calibración terminó con
todas las etapas `ok=1`.

## LED

| Estado | Comportamiento |
|--------|----------------|
| Sin ESP (wait_for_esp) | Parpadeo lento (PING_PERIOD=700ms) |
| ESP conectado, operación normal | Encendido fijo (sólido) |
| CMD 0xB9 BLINK_LED recibido | Parpadea ~8 s a 2.5 Hz y vuelve a fijo |

## Frecuencia de muestreo

- **Configs expuestas**: `ADC_CF_2V5` (±2.5 V, default) y `ADC_CF_0V512` (±0.512 V)
- **Reportada por firmware**: 1020 Hz para ambas
- **Batches**: 30 muestras/lote → 29.4 ms/lote a 1020 Hz
- **Máximo capturable**: 512 lotes = 15360 muestras ≈ 15.06 s

## Archivos fuente

| Archivo | Rol |
|---------|-----|
| `main.c` | Firmware completo: ISRs, protocolo UART, estados |
| `psoc_hw.h/.c` | Detección GEO/HAMMER, comandos, arranque analógico |
| `psoc_adc.h/.c` | Configuración ADC_DelSig, sample hook para calibración |
| `calibration.h/.c` | Motor PI async, servo, telemetría de progreso |
| `calibration_tables.h` | Agregador de etapas activas (incluye los sub-headers) |
| `calibration_tables_geo_*.h` | Parámetros PI por etapa GEO (PGA/BP/ADDER/LP) |
| `calibration_tables_hammer_*.h` | Parámetros PI por etapa HAMMER (PGA/LP) |
| `psoc_nv.h/.c` | EEPROM v2: guardar/cargar DACs por ganancia PGA |
| `filter_coeffs.h/.c` | Definición de FILTER_FIR_NTAPS |
| `FIR_adquisicion.h/.c` | Coeficientes FIR para captura |
| `FIR_calibration.h/.c` | Coeficientes FIR para calibración PI |
| `crc.h/.c` | CRC XOR para EEPROM |
| `psoc_debug.h/.c` | Decodificador de eventos DIAG para monitor serial |
