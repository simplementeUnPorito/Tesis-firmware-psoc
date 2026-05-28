/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"

CY_ISR(getADCdata){

}

int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    Opa_ref_Start();
    VDAC_Start();
    Opa_buff_1_Start();
    Opa_buff_2_Start();
    Opa_buff_3_Start();
    PGA_1_Start();
    ADC_Start();
    isr_ADC_StartEx(getADCdata);
    LPF_1_Start();
    LPF_2_Start();
    /* Place your initialization/startup code here (e.g. MyInst_Start()) */

    for(;;)
    {
        /* Place your application code here. */
    }
}

/* [] END OF FILE */
