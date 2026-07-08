#ifndef PSOC_HW_H
#define PSOC_HW_H

#include "project.h"

#define PSOC_HW_GEO    0u
#define PSOC_HW_HAMMER 1u

#if defined(PGAgain_DEFAULT_GAIN)
    #define PSOC_HW_CLASS PSOC_HW_GEO
    #define PSOC_PGA_DEFAULT_CODE PGAgain_DEFAULT_GAIN
    #if defined(PGAvdac_DEFAULT_GAIN)
        #define PSOC_PGAVDAC_DEFAULT_CODE PGAvdac_DEFAULT_GAIN
    #else
        #define PSOC_PGAVDAC_DEFAULT_CODE 0u
    #endif
#elif defined(PGA_DEFAULT_GAIN)
    #define PSOC_HW_CLASS PSOC_HW_HAMMER
    #define PSOC_PGA_DEFAULT_CODE PGA_DEFAULT_GAIN
    #if defined(PGA_ref_DEFAULT_GAIN)
        #define PSOC_PGAVDAC_DEFAULT_CODE PGA_ref_DEFAULT_GAIN
    #else
        #define PSOC_PGAVDAC_DEFAULT_CODE 0u
    #endif
#else
    #error "Hardware no reconocido: el TopDesign debe tener PGAgain (GEO) o PGA (HAMMER)."
#endif

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

#define PSOC_CMD_STATUS         0xA5u
#define PSOC_CMD_PGA            0xA6u
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

void psoc_hw_start_analog(uint8 pga_code, uint8 pgavdac_code);
void psoc_hw_set_pga(uint8 code);
void psoc_hw_set_pgavdac(uint8 code);
uint8 psoc_hw_get_pga_code(void);
uint16 psoc_hw_pga_gain_x1000(void);

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
