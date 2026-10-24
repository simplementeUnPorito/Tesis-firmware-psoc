#include "filter_coeffs.h"
#include "Filter.h"
#include "Filter_PVT.h"
#include <string.h>
#include "FIR_adquisition.h"

static uint8 g_fir_stage[FILTER_FIR_NTAPS * 4u];
static uint8 g_fir_user[FILTER_FIR_NTAPS * 4u];
static uint8 g_fir_user_valid = 0u;

const uint8 *psoc_fir_acquisition_coeffs(void)
{
    return g_fir_user_valid ? g_fir_user : g_fir_adquisition_coeffs_q23;
}

uint8 psoc_fir_user_command(uint8 cmd, uint8 p1, uint8 p2, uint8 p3)
{
    uint16 i;
    uint16 sum = 0u;
    if (cmd == PSOC_CMD_FIR_TAP_LO) {
        if (p1 < FILTER_FIR_NTAPS) {
            g_fir_stage[(uint16)p1 * 4u + 0u] = p2;
            g_fir_stage[(uint16)p1 * 4u + 1u] = p3;
        }
        return 0xFFu;
    }
    if (cmd == PSOC_CMD_FIR_TAP_HI) {
        if (p1 < FILTER_FIR_NTAPS) {
            g_fir_stage[(uint16)p1 * 4u + 2u] = p2;
            g_fir_stage[(uint16)p1 * 4u + 3u] = 0u;
        }
        return 0xFFu;
    }
    if (cmd != PSOC_CMD_FIR_COMMIT) {
        return 0xFFu;
    }
    if (p1 == 0u) {
        g_fir_user_valid = 0u;
        return psoc_filter_load_fir_coefficients(g_fir_adquisition_coeffs_q23, FILTER_FIR_NTAPS);
    }
    for (i = 0u; i < FILTER_FIR_NTAPS; i++) {
        sum = (uint16)(sum + g_fir_stage[i * 4u] + g_fir_stage[i * 4u + 1u] +
                       g_fir_stage[i * 4u + 2u]);
    }
    if (sum != (uint16)((uint16)p2 | ((uint16)p3 << 8u))) {
        return 0u;
    }
    (void)memcpy(g_fir_user, g_fir_stage, sizeof(g_fir_user));
    g_fir_user_valid = 1u;
    return psoc_filter_load_fir_coefficients(g_fir_user, FILTER_FIR_NTAPS);
}

/* Si esto frena la compilacion, FILTER_FIR_NTAPS (filter_coeffs.h) quedo
 * desincronizado del "Number of Taps" configurado en el customizer del
 * componente Filter (TopDesign) -- Filter_FIR_A_SIZE lo refleja siempre que
 * se haga "Generate Application". Arreglar ANTES de tocar el arreglo de
 * abajo: cargar un N de taps distinto al microcodigo real del DFB corrompe
 * la RAM de coeficientes. */
#if FILTER_FIR_NTAPS != (Filter_FIR_A_SIZE / 4u)
#error "FILTER_FIR_NTAPS (filter_coeffs.h) debe ser igual a Filter_FIR_A_SIZE/4 (Filter.h, generado desde el customizer del componente Filter)"
#endif

uint8 psoc_filter_load_fir_coefficients(const uint8 *coeffs_bytes, uint16 ntaps)
{
    uint8 wasRunning;

    if (coeffs_bytes == (const uint8 *)0 || ntaps != FILTER_FIR_NTAPS) {
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
     * unico canal usado, Canal B esta Disabled en TopDesign). Mismo memcpy
     * directo que usa Filter_Init() con Filter_data_b -- coeffs_bytes ya
     * viene en el layout nativo (ver filter_coeffs.h), no hace falta
     * convertir nada tap por tap. */
    (void)memcpy((void *)Filter_DB_RAM, coeffs_bytes, (size_t)ntaps * 4u);

    /* Sacar las RAMs del bus */
    Filter_RAM_DIR_REG = Filter_RAM_DIR_DFB;

    /* Limpiar interrupciones pendientes (bits [2..0] son solo lectura) */
    Filter_SR_REG = 0xF8u;

    if (wasRunning) {
        Filter_CR_REG |= Filter_RUN_MASK;
    }

    return 1u;
}

void psoc_filter_reset_history(void)
{
    uint8 wasRunning;

    Filter_PM_ACT_CFG_REG |= Filter_PM_ACT_MSK;

    wasRunning = Filter_CR_REG & Filter_RUN_MASK;
    Filter_CR_REG &= (uint8)~Filter_RUN_MASK;

    Filter_RAM_EN_REG = Filter_RAM_DIR_BUS;
    Filter_RAM_DIR_REG = Filter_RAM_DIR_BUS;

    (void)memcpy(Filter_DA_RAM, Filter_data_a, Filter_DA_RAM_SIZE);

    Filter_RAM_DIR_REG = Filter_RAM_DIR_DFB;
    Filter_SR_REG = 0xF8u;

    if (wasRunning) {
        Filter_CR_REG |= Filter_RUN_MASK;
    }
}
