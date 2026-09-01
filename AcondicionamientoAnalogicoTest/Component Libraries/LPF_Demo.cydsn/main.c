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
#include "version.h"
#include "stdio.h"

static int8_t cf = -1;

/****************************************************************/
CY_ISR(user_sw_isr)
{

	cf++;
	cf %= 5;	// 0 to 4 allowed
	if(cf <= 3)
	{	/* Change the corner frequency and make sure the LFP is enabled. */
		LPF_1_SetFc(cf);
		LPF_1_Enable();
		LPF_2_SetFc(cf);
		LPF_2_Enable();
	}
	else
	{	/* special case to Disable/Stop = Bypass LPF. */
		LPF_1_Disable();
		LPF_2_Disable();		
	}
}
char build[] = "\r\n" CY_PROJECT_NAME " " TOSTRING(SW_VER_REV_REL) "\r\n";
/****************************************************************/
int main(void)
{
uint8_t cf_prev = cf;
	
    CyGlobalIntEnable; /* Enable global interrupts. */
	/* Start the debug UART */
	UART_Start();

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
	LPF_1_Start();
	Opamp_1_Start();

	LPF_2_Start();
	Opamp_2_Start();
	
	isr_sw_ClearPending();
	isr_sw_StartEx(user_sw_isr);
	
/* Send build and user instructions */
	UART_PutString(build);
	UART_PutString("Press the user switch to change the corner frequency...\r\n");
	
    for(;;)
    {
        if(cf_prev != cf)
		{	/* corner frequency setting changed */
			UART_PutString("LPF_1 Fc = "); UART_PutString(LPF_1_GetFc_Str()); UART_PutString("\t");
			UART_PutString("LPF_2 Fc = "); UART_PutString(LPF_2_GetFc_Str());
			UART_PutString("\r\n");
			cf_prev = cf;
		}
    }
}

/* [] END OF FILE */
