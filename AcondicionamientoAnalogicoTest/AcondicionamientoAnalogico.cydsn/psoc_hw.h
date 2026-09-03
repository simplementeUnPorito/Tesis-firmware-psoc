#ifndef PSOC_HW_H
#define PSOC_HW_H

#include "project.h"

#define PSOC_HW_GEO    0u
#define PSOC_HW_HAMMER 1u

/* TODO(metadata-hw-revision): definir una revisión de circuito y un número de
 * placa estables, independientes de PSOC_HW_CLASS y de la versión de firmware.
 * Reportarlos al ESP mediante el protocolo PSoC->ESP y hacer que el ESP los
 * persista en los metadatos de cada captura junto con ganancia y calibración.
 * Esto debe permitir distinguir prototipos universales, placa planchada/ácido
 * y futuras PCB fabricadas, sin inferir la revisión analógica desde el código. */

#if defined(PGAgain_DEFAULT_GAIN)
    #define PSOC_HW_CLASS PSOC_HW_GEO
    #define PSOC_PGA_DEFAULT_CODE PGAgain_DEFAULT_GAIN
    #if defined(PGAvdac_DEFAULT_GAIN)
        #define PSOC_PGAVDAC_DEFAULT_CODE PGAvdac_DEFAULT_GAIN
    #else
        #define PSOC_PGAVDAC_DEFAULT_CODE 0u
    #endif
    /* PGAout: etapa de salida agregada al pipeline GEO en la placa nueva.
     * Va después del sumador y antes del LPF de entrada al ADC, así que su
     * ganancia se configura por separado de PGAgain. */
    #if defined(PGAout_DEFAULT_GAIN)
        #define PSOC_PGAOUT_DEFAULT_CODE PGAout_DEFAULT_GAIN
    #else
        #define PSOC_PGAOUT_DEFAULT_CODE 0u
    #endif
#elif defined(PGA_DEFAULT_GAIN)
    #define PSOC_HW_CLASS PSOC_HW_HAMMER
    #define PSOC_PGA_DEFAULT_CODE PGA_DEFAULT_GAIN
    #if defined(PGA_ref_DEFAULT_GAIN)
        #define PSOC_PGAVDAC_DEFAULT_CODE PGA_ref_DEFAULT_GAIN
    #else
        #define PSOC_PGAVDAC_DEFAULT_CODE 0u
    #endif
    /* HAMMER no tiene PGAout: el código igual existe para que el protocolo
     * sea uno solo, pero psoc_hw_set_pgaout() no toca hardware. */
    #define PSOC_PGAOUT_DEFAULT_CODE 0u
#else
    #error "Hardware no reconocido: el TopDesign debe tener PGAgain (GEO) o PGA (HAMMER)."
#endif

/* ==========================================================================
 * Referencias de calibración: IDAC8 + resistencia a Vref (placa nueva)
 * --------------------------------------------------------------------------
 * Los cuatro DAC de calibración (VDAC_ref_PGA / VDAC_ref_LP / VDAC_ref_BP /
 * VDAC_Ref_Sum — nombres heredados, ver TopDesign) dejaron de ser VDAC8 y
 * ahora son IDAC8. Se usan como DAC de tensión inyectando la corriente sobre
 * una resistencia referida a Vref:
 *
 *     I(code) = code * (I_fondo_escala / 255)
 *     V(code) = Vref + I(code) * R
 *
 * Con los valores actuales de placa (R = 30 kΩ, Vref ≈ 2.062 V,
 * I = 0 .. 31.875 µA) el LSB es 125 nA -> 3.75 mV y el rango útil va de
 * Vref a Vref + 0.956 V. Si cambia la resistencia o la referencia de la placa,
 * alcanza con tocar las constantes de abajo (o pisar las globales en runtime):
 * las tablas de calibración y los helpers derivan todo de ellas.
 *
 * Vref NO es un número fijo: lo genera un AMS1117-ADJ en la placa y se calcula
 * a partir de su divisor, así que cambiar un resistor es cambiar un #define.
 * Hoja de datos UMW AMS1117, Figura 2 (Typical Adjustable Output Voltage):
 *
 *     VOUT = VREF * (1 + R2/R1) + IADJ * R2
 *
 * con R1 entre VOUT y ADJ (R22 en la placa) y R2 entre ADJ y GND (R23).
 * Ojo con la convención: es R2/R1, no R1/R2.
 * ========================================================================== */
#define PSOC_AMS1117_R1_OHM             1000u       /* R22: VOUT -> ADJ      */
#define PSOC_AMS1117_R2_OHM             620u        /* R23: ADJ  -> GND      */
#define PSOC_AMS1117_VREF_UV            1250000u    /* 1.250 V typ           */
#define PSOC_AMS1117_IADJ_NA            60000u      /* 60 µA typ             */

#define PSOC_IDAC_RSET_OHM_DEFAULT      30000u      /* R de conversión I->V  */
/* 1250000 + (1250000/1000)*620 + (60000*620)/1000 = 2062200 µV = 2.0622 V */
#define PSOC_IDAC_VREF_UV_DEFAULT                                              \
    (PSOC_AMS1117_VREF_UV                                                      \
     + (PSOC_AMS1117_VREF_UV / PSOC_AMS1117_R1_OHM) * PSOC_AMS1117_R2_OHM      \
     + (PSOC_AMS1117_IADJ_NA * PSOC_AMS1117_R2_OHM) / 1000u)
#define PSOC_IDAC_FULLSCALE_NA_DEFAULT  31875u      /* 31.875 µA en nA       */
#define PSOC_IDAC_CODE_MAX              255u

/* ------------------------------------------------------------------
 * Referencias con signo
 * ------------------------------------------------------------------
 * Cada IDAC8 tiene una entrada `ipolarity` cableada a un bit del Control
 * Register `polarity_reg` del TopDesign. Con eso la referencia no queda de
 * un solo lado de Vref: vale Vref +- R*Idac. El codigo 0 pasa a ser
 * exactamente Vref, que es el punto natural para arrancar a calibrar.
 *
 * El rango se acota a +-127 y no a +-255 por una razon concreta: el slot de
 * EEPROM guarda un uint8 por etapa, y +-127 entra sesgado por 128 sin tocar
 * el layout ni el CRC. En codigos de esta placa son +-238 mV en la
 * referencia, de sobra para anular los offsets medidos (el peor es el del
 * pasabajos, que necesita unos 84 codigos).
 *
 * OJO: que el bit en 1 signifique sumidero (por debajo de Vref) esta por
 * confirmar en banco. Si sale al reves, alcanza con dar vuelta
 * PSOC_IDAC_POLARITY_NEGATIVE_BIT. */
#define PSOC_IDAC_SIGNED_MAX            127
#define PSOC_IDAC_POLARITY_NEGATIVE_BIT 1u

/* Sesgo con el que un codigo con signo entra en un uint8 de EEPROM. */
#define PSOC_IDAC_EEPROM_BIAS           128


extern uint32 g_psoc_idac_rset_ohm;
extern uint32 g_psoc_idac_vref_uv;
extern uint32 g_psoc_idac_fullscale_na;

/* Fija la polaridad de la etapa segun el signo y devuelve la magnitud que
 * hay que escribirle al IDAC. Se llama ANTES de escribir la magnitud. */
uint8 psoc_hw_idac_apply_polarity(uint8 stage, int16 code);
/* Satura un codigo con signo al rango util. */
int16 psoc_hw_idac_clamp_signed(int16 code);
/* Deja las cuatro referencias en polaridad positiva. */
void  psoc_hw_idac_polarity_reset(void);
uint8 psoc_hw_idac_polarity_mask(void);

/* Corriente inyectada por el IDAC para un código dado, en nA. */
uint32 psoc_idac_code_to_na(uint8 code);
/* Tensión que ve la etapa analógica para un código dado, en µV. */
uint32 psoc_idac_code_to_uv(uint8 code);
/* Igual pero con signo: devuelve la DESVIACION respecto de Vref, en µV, que
 * es lo que tiene sentido informar ahora que el codigo 0 es Vref. */
int32 psoc_idac_code_to_uv_signed(int16 code);
/* Código IDAC más cercano a una tensión pedida en µV (saturado al rango). */
uint8  psoc_idac_uv_to_code(uint32 uv);
/* Paso del DAC en µV (LSB). Útil para deadbands de la calibración. */
uint32 psoc_idac_lsb_uv(void);

#define PSOC_IDLE      0u
#define PSOC_ARMED     1u
#define PSOC_SAMPLING  2u
#define PSOC_CALIBRATING 3u

#define PSOC_CMD_PING      0xC0u
#define PSOC_CMD_PONG      0xC1u
#define PSOC_CMD_CFG_ACK   0xC2u
#define PSOC_CMD_FS_REPORT 0xC3u
#define PSOC_CMD_DIAG_EVT  0xC4u

#define PSOC_EVT_BOOT             0x01u
#define PSOC_EVT_ANALOG_READY     0x02u
#define PSOC_EVT_CAL_START        0x10u
#define PSOC_EVT_CAL_DONE         0x11u
#define PSOC_EVT_CAL_BUSY         0x12u
#define PSOC_EVT_CAL_STAGE_DAC    0x13u
#define PSOC_EVT_CAL_STAGE_MEAS   0x14u
#define PSOC_EVT_CAL_STAGE_BEGIN  0x15u
#define PSOC_EVT_CAL_STAGE_OK     0x16u
#define PSOC_EVT_CAL_VERIFY_BEGIN 0x17u
#define PSOC_EVT_CAL_VERIFY_OK    0x18u
#define PSOC_EVT_CAL_AMUX_IN      0x19u
#define PSOC_EVT_CAL_PROGRESS     0x1Au
#define PSOC_EVT_CAL_WATCHDOG     0x1Bu
#define PSOC_EVT_CAL_LP_BAD       0x1Cu
#define PSOC_EVT_CAL_STAGE_MEAS32 0x1Du
#define PSOC_EVT_SERVO_STAGE      0x1Eu
#define PSOC_EVT_SERVO_STEP       0x1Fu
#define PSOC_EVT_WAIT_ESP         0x20u
#define PSOC_EVT_ESP_SEEN         0x21u
#define PSOC_EVT_CAL_LOOP         0x22u
#define PSOC_EVT_CAL_STAGE_SAT    0x23u  /* value=1 si el candidato actual satura (|medido|>=sat_counts) */
#define PSOC_EVT_CAL_STAGE_SAT_ALL 0x24u /* todas las mediciones de la etapa saturaron (value=stage_index) */
#define PSOC_EVT_CAL_REALCHECK_BEGIN 0x25u /* value=stage_index, inicio fase de señal real para esa etapa */
#define PSOC_EVT_CAL_REALCHECK_DAC   0x26u /* value=dac actual evaluado en realcheck */
#define PSOC_EVT_CAL_REALCHECK_MEAS32 0x27u /* 4 bytes MSB-first, igual patron que CAL_STAGE_MEAS32 */
#define PSOC_EVT_CAL_REALCHECK_NUDGE 0x28u /* value=nudge aplicado, signed 8-bit (0xFF=-1,0x01=+1,0x00=revertido/sin cambio) */
#define PSOC_EVT_CAL_REALCHECK_OK    0x29u /* value=1 si la etapa quedo dentro de tol_counts, 0 si no */
#define PSOC_EVT_CAL_STAGE_TARGET32 0x2Au /* 4 bytes MSB-first: target_counts de la etapa activa */
#define PSOC_EVT_CAL_SWEEP_DAC      0x2Bu /* value=dac evaluado en sweep diagnostico */
#define PSOC_EVT_CAL_SWEEP_MEAS32   0x2Cu /* 4 bytes MSB-first: ADC directo para sweep diagnostico */
#define PSOC_EVT_ADC_SNAPSHOT_BEGIN 0x2Du /* value=stage_index, inicio snapshot ADC manual */
#define PSOC_EVT_ADC_RAW32          0x2Eu /* 4 bytes MSB-first: ADC directo del canal/VDAC actual */
#define PSOC_EVT_ADC_FILT32         0x2Fu /* 4 bytes MSB-first: salida Filter/DFB, reservado para diagnostico */
#define PSOC_EVT_RX_CMD           0x30u
#define PSOC_EVT_SETN             0x31u
#define PSOC_EVT_ARMED            0x32u
#define PSOC_EVT_SYNC_RISE        0x33u
#define PSOC_EVT_SYNC_FALL        0x34u
#define PSOC_EVT_SAMPLING_START   0x35u
#define PSOC_EVT_CAPTURE_DONE     0x36u
#define PSOC_EVT_DUMP_START       0x37u
#define PSOC_EVT_DUMP_DONE        0x38u
#define PSOC_EVT_START_NOW        0x39u
#define PSOC_EVT_DEBUG_MODE       0x3Au
#define PSOC_EVT_STATUS_REQ       0x3Bu
#define PSOC_EVT_BUTTON           0x3Cu
#define PSOC_EVT_BUTTON_IGNORED   0x3Du
#define PSOC_EVT_CAPTURE_CLAMPED  0x3Eu  /* value=lotes pedidos saturado a 8 bit; el equipo recortó a PSOC_CAPTURE_MAX_BATCHES */
#define PSOC_EVT_CAL_PI_GAIN32    0x3Fu  /* 4 bytes MSB-first: ganancia fisica VDAC->medida, x1000, con signo */
#define PSOC_EVT_CAL_PI_DEADBAND  0x40u  /* value=deadband en codigos DAC (saturado a 255 para telemetria) */
#define PSOC_EVT_CAL_PI_ERROR32   0x41u  /* 4 bytes MSB-first: error reescalado a codigos DAC */
#define PSOC_EVT_CAL_PI_BUCKET32  0x42u  /* 4 bytes MSB-first: celda cuantizada de error */
#define PSOC_EVT_CAL_PI_STABLE    0x43u  /* value=muestras consecutivas en la misma celda (saturado a 255) */
#define PSOC_EVT_CAL_AMUX_CAP     0x44u  /* value=canal AMux_ADC del capacitor conectado durante calibracion */
#define PSOC_EVT_CAPTURE_WATCHDOG 0x45u  /* value=state de superMaquina al vencer el watchdog de captura (Timer_3) */
#define PSOC_EVT_TIMER_STORM      0x46u  /* value=timer (0=Timer RX,1,2,3): IRQ en tormenta, breaker disparado y timer re-inicializado */
#define PSOC_EVT_CHAIN_NEXT       0x47u  /* value=lotes DECIMADOS acumulados; se terminó un trozo y se re-arma la próxima corrida (captura encadenada) */
#define PSOC_EVT_SD_STATUS        0x48u  /* bit0 presente, bits1-2 tipo, bit3 test, bit4 FAT, bit5 sesión, bit6 capture-enable */
#define PSOC_EVT_SD_SESSION       0x49u  /* value=1: GEOLAST.BIN COMPLETE/listo; 0: no válido */
#define PSOC_EVT_SD_ERROR         0x4Au  /* value: bit0 write fail, bit1 overrun ring, bit2 header/dir fail, bit3 read fail en dump */
#define PSOC_EVT_ARMED_TIMEOUT    0x4Bu  /* no llego SYNC dentro del limite; captura abortada y estado restaurado a IDLE */

#define PSOC_CMD_STATUS         0xA5u
#define PSOC_CMD_PGA            0xA6u
#define PSOC_CMD_PGAOUT         0xA8u  /* GEO: ganancia de la etapa PGAout (código 0-8) */
#define PSOC_CMD_PGAVDAC        0xA9u
#define PSOC_CMD_VDAC           0xAAu
#define PSOC_CMD_SETN           0xA3u
#define PSOC_CMD_PRESTART       0xB1u
#define PSOC_CMD_DEBUG          0xB3u
#define PSOC_CMD_START_NOW      0xB4u
#define PSOC_CMD_CALIBRATE      0xB5u
#define PSOC_CMD_SAVE_EEPROM    0xB6u  /* Guardar config calibración en EEPROM */
#define PSOC_CMD_SELECT_STREAM  0xB7u  /* Param: 0=crudo, 1=filtrado FIR */
#define PSOC_CMD_ADC_SNAPSHOT   0xB8u  /* Reporte diagnostico de ADC por etapa */
#define PSOC_CMD_BLINK_LED      0xB9u  /* Titilar LED de identificación del nodo */
#define PSOC_CMD_ADC_CONFIG     0xBAu  /* Param: 1=ADC_CF_2V5 (±2.5V), 2=ADC_CF_0V512 (±0.512V),
                                          3=ADC_CF_1V024 (±1.024V), 4=ADC_CF_0V625 (±0.625V) */
#define PSOC_CMD_SET_DECIMATION 0xBBu  /* Param: factor de decimación 1..100 (1=sin decimar,
                                          Fs efectiva = 2604/factor). Promedia `factor` muestras
                                          consecutivas del stream ya elegido (raw/FIR) antes de
                                          guardarlas. Solo aceptado en PSOC_IDLE. */
#define PSOC_CMD_SD_STATUS      0xBCu  /* Param: 0=consultar estado cacheado, 1=re-init (solo IDLE).
                                          Ack: sd_spi_status_byte(). */
#define PSOC_CMD_SD_TEST        0xBDu  /* Self-test FatFs no destructivo: usa y elimina GEOTEST.BIN. */
#define PSOC_CMD_SD_CAPTURE     0xBEu  /* Param: 1=capturar a SD (ring RAM->SD, permite N>512),
                                          0=modo RAM-only clásico. Solo IDLE. Ack: 0/1=aplicado,
                                          0xEE=rechazado (sin SD o estado ocupado). */
#define PSOC_CMD_SD_READ_BATCH  0xBFu  /* 2 params: índice uint16 LE. Éxito: frame normal con
                                          seq=índice; fallo: CFG_ACK(BF,0), sin datos falsos. */

void psoc_hw_start_analog(uint8 pga_code, uint8 pgavdac_code, uint8 pgaout_code);
void psoc_hw_set_pga(uint8 code);
void psoc_hw_set_pgavdac(uint8 code);
void psoc_hw_set_pgaout(uint8 code);
uint8 psoc_hw_get_pga_code(void);
uint8 psoc_hw_get_pgaout_code(void);
uint16 psoc_hw_pga_gain_x1000(void);
uint16 psoc_hw_pgaout_gain_x1000(void);

/* ==========================================================================
 * Enlace PSoC -> ESP32 (I2C maestro)
 * --------------------------------------------------------------------------
 * En la placa nueva la UART quedó SOLO como RX: el ESP manda comandos por
 * UART y el PSoC contesta/streamea por I2C, donde el PSoC es maestro y el
 * ESP esclavo. Se hizo así porque la UART a 115200 era el cuello de botella
 * del stream de muestras.
 *
 * Toda la salida del firmware (pings, acks, diagnóstico y frames de captura)
 * pasa por psoc_link_put_array(), que reemplaza al viejo UART_PutArray().
 * ========================================================================== */
#ifndef PSOC_LINK_I2C_ADDR
#define PSOC_LINK_I2C_ADDR      0x42u   /* dirección del ESP32 como esclavo I2C */
#endif
/* Frame más largo del protocolo: 4 cabecera + 30*3 muestras + 1 CRC. */
#define PSOC_LINK_MAX_FRAME     95u

void  psoc_link_start(void);
void  psoc_link_put_array(const uint8 *buf, uint16 len);
void  psoc_link_wait_idle(void);
uint8 psoc_link_last_ok(void);

/* Compatibilidad para el servo legacy de calibracion. La calibracion activa
 * usa Timer_1 (modo CAL_TICK, nunca concurrente con los pings) y los helpers
 * de abajo, no un tick periodico de sistema. Timer_3 es el watchdog de
 * captura dedicado (ver mapa de timers en main.c). */
uint32 psoc_now_ticks(void);

void psoc_cal_timer_start(uint32 progress_ms, uint32 watchdog_ms);
void psoc_cal_timer_stop(void);
uint8 psoc_cal_timer_take_progress_due(void);
uint8 psoc_cal_timer_take_watchdog_due(void);

/* Selecciona la ruta de DMA que deja pasar superMaquina: raw directo a RAM
 * (use_filter=0) o ADC->Filter + Filter->RAM (use_filter=1). Con la maquina
 * deshabilitada actua como bypass para calibracion/diagnostico. */
void dma_route_select(uint8 use_filter);

#endif
