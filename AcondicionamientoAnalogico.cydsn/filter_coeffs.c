#include "filter_coeffs.h"
#include "Filter.h"
#include "Filter_PVT.h"
#include <string.h>

/* Si esto frena la compilacion, FILTER_FIR_NTAPS (filter_coeffs.h) quedo
 * desincronizado del "Number of Taps" configurado en el customizer del
 * componente Filter (TopDesign) -- Filter_FIR_A_SIZE lo refleja siempre que
 * se haga "Generate Application". Arreglar ANTES de tocar el arreglo de
 * abajo: cargar un N de taps distinto al microcodigo real del DFB corrompe
 * la RAM de coeficientes. */
#if FILTER_FIR_NTAPS != (Filter_FIR_A_SIZE / 4u)
#error "FILTER_FIR_NTAPS (filter_coeffs.h) debe ser igual a Filter_FIR_A_SIZE/4 (Filter.h, generado desde el customizer del componente Filter)"
#endif

uint8 psoc_filter_load_fir_coefficients(const int32 *coeffs_q23, uint16 ntaps)
{
    uint16 i;
    uint8 wasRunning;

    if (coeffs_q23 == (const int32 *)0 || ntaps != FILTER_FIR_NTAPS) {
        return 0u;
    }

    /* Power on DFB antes de tocar las RAMs */
    Filter_PM_ACT_CFG_REG |= Filter_PM_ACT_MSK;

    /* Apagar el run bit para escribir, y restaurarlo al final al estado que
     * tenia antes -- el snippet de referencia de Cypress lo apaga y nunca lo
     * vuelve a prender, lo que frena el filtro para siempre si se llama
     * mientras esta en uso (p.ej. el stream filtrado de captura). */
    wasRunning = Filter_CR_REG & Filter_RUN_MASK;
    Filter_CR_REG &= (uint8)~Filter_RUN_MASK;

    /* Habilitar las RAMs del DFB en el bus */
    Filter_RAM_EN_REG = Filter_RAM_DIR_BUS;
    Filter_RAM_DIR_REG = Filter_RAM_DIR_BUS;

    /* Limpiar la linea de retardo (Data RAM A): coeficientes nuevos no deben
     * mezclarse con historial de muestras del diseño anterior. */
    (void)memcpy(Filter_DA_RAM, Filter_data_a, Filter_DA_RAM_SIZE);

    /* Cargar los coeficientes nuevos en Data RAM B (Canal A, offset 0 --
     * unico canal usado, Canal B esta Disabled en TopDesign). */
    for (i = 0u; i < ntaps; i++) {
        uint32 raw = (uint32)coeffs_q23[i] & 0x00FFFFFFUL;
        ((uint8 *)Filter_DB_RAM)[(i * 4u) + 0u] = (uint8)(raw & 0xFFu);
        ((uint8 *)Filter_DB_RAM)[(i * 4u) + 1u] = (uint8)((raw >> 8u) & 0xFFu);
        ((uint8 *)Filter_DB_RAM)[(i * 4u) + 2u] = (uint8)((raw >> 16u) & 0xFFu);
        ((uint8 *)Filter_DB_RAM)[(i * 4u) + 3u] = 0x00u;
    }

    /* Sacar las RAMs del bus */
    Filter_RAM_DIR_REG = Filter_RAM_DIR_DFB;

    /* Limpiar interrupciones pendientes (bits [2..0] son solo lectura) */
    Filter_SR_REG = 0xF8u;

    if (wasRunning) {
        Filter_CR_REG |= Filter_RUN_MASK;
    }

    return 1u;
}
