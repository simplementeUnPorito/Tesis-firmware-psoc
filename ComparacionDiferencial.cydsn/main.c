/*******************************************************************************
* main.c — WaveDAC8 con divisor de clock rotatorio via isr_boton
*
* El botón rota el divisor de Clock_Wave entre:
*   1, 3, 5, 10, 20, 100, 1000, 10000
*******************************************************************************/

#include "project.h"

static const uint16 DIVISORES[] = {1u, 3u, 5u, 10u, 20u, 100u, 1000u, 10000u};
#define NUM_DIV (sizeof(DIVISORES) / sizeof(DIVISORES[0]))

static volatile uint8 g_div_idx = 0u;

CY_ISR(isr_boton_Handler)
{
    isr_boton_ClearPending();
    g_div_idx = (g_div_idx + 1u) % NUM_DIV;
    Clock_Wave_SetDivider(DIVISORES[g_div_idx]);
}

int main(void)
{
    CyGlobalIntEnable;

    
    Opa_comun_Start();
    Opa_ref_Start();
    
    
    WaveDAC_Start();
    WaveDAC_Enable();

    PGA_p_Start();
    PGA_n_Start();

    TIA_p_Start();
    TIA_n_Start();

    

    Clock_Wave_SetDivider(DIVISORES[0u]);
    isr_boton_StartEx(isr_boton_Handler);

    for(;;)
    {
    }

    return 0u;
}

/* [] END OF FILE */
