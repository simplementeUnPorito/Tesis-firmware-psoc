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
    LPF_ref_Start();
    LPF_ADC_Start();
    
    Opa_ref_Start();
    PGA_ref_Start();
    
    VDAC_Start();
    
    ADC_Start();
    
    PGA_Start();
    
    isr_ADC_StartEx(getADCdata);
   
    /* Place your initialization/startup code here (e.g. MyInst_Start()) */

    for(;;)
    {
        /* Place your application code here. */
    }
}

/* [] END OF FILE */
