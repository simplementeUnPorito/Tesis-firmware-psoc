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

void psoc_hw_start_analog(uint8 pga_code, uint8 pgavdac_code);
void psoc_hw_set_pga(uint8 code);
void psoc_hw_set_pgavdac(uint8 code);

/* Contador de ticks de 10 ms (g_timer_ticks de main.c), usado por
 * calibration.c para el watchdog y la telemetria periodica de progreso. */
uint32 psoc_now_ticks(void);

#endif
