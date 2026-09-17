#ifndef PSOC_DEBUG_H
#define PSOC_DEBUG_H

#include "project.h"

/* UART de debug directo a PC (componente "PC" en TopDesign, separado del
 * UART que habla con el ESP). Todo lo que llama a estas funciones compila
 * igual este o no el componente en el esquema -- si "PC" no esta en
 * TopDesign (CY_UART_PC_H no definido), los cuerpos de psoc_debug.c quedan
 * vacios (no-op) y no hay ninguna referencia a simbolos del componente, asi
 * que sacarlo de TopDesign no rompe el build. */

void psoc_debug_start(void);
void psoc_debug_print(const char *msg);
void psoc_debug_print_u32(const char *prefix, uint32 value);
void psoc_debug_print_i32(const char *prefix, int32 value);

/* Espejo legible de cada PSOC_EVT_* (psoc_hw.h) que ya se manda al ESP por
 * uart_send_diag (main.c) -- mismo evento/value/state, una linea por evento. */
void psoc_debug_print_evt(uint8 event, uint8 value, uint8 state);

#endif
