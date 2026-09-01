/*******************************************************************************
* File Name: tmr_event.c  
* Version 1.90
*
* Description:
*  This file contains API to enable firmware to read the value of a Status 
*  Register.
*
* Note:
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#include "tmr_event.h"

#if !defined(tmr_event_sts_sts_reg__REMOVED) /* Check for removal by optimization */


/*******************************************************************************
* Function Name: tmr_event_Read
********************************************************************************
*
* Summary:
*  Reads the current value assigned to the Status Register.
*
* Parameters:
*  None.
*
* Return:
*  The current value in the Status Register.
*
*******************************************************************************/
uint8 tmr_event_Read(void) 
{ 
    return tmr_event_Status;
}


/*******************************************************************************
* Function Name: tmr_event_InterruptEnable
********************************************************************************
*
* Summary:
*  Enables the Status Register interrupt.
*
* Parameters:
*  None.
*
* Return:
*  None.
*
*******************************************************************************/
void tmr_event_InterruptEnable(void) 
{
    uint8 interruptState;
    interruptState = CyEnterCriticalSection();
    tmr_event_Status_Aux_Ctrl |= tmr_event_STATUS_INTR_ENBL;
    CyExitCriticalSection(interruptState);
}


/*******************************************************************************
* Function Name: tmr_event_InterruptDisable
********************************************************************************
*
* Summary:
*  Disables the Status Register interrupt.
*
* Parameters:
*  None.
*
* Return:
*  None.
*
*******************************************************************************/
void tmr_event_InterruptDisable(void) 
{
    uint8 interruptState;
    interruptState = CyEnterCriticalSection();
    tmr_event_Status_Aux_Ctrl &= (uint8)(~tmr_event_STATUS_INTR_ENBL);
    CyExitCriticalSection(interruptState);
}


/*******************************************************************************
* Function Name: tmr_event_WriteMask
********************************************************************************
*
* Summary:
*  Writes the current mask value assigned to the Status Register.
*
* Parameters:
*  mask:  Value to write into the mask register.
*
* Return:
*  None.
*
*******************************************************************************/
void tmr_event_WriteMask(uint8 mask) 
{
    #if(tmr_event_INPUTS < 8u)
    	mask &= ((uint8)(1u << tmr_event_INPUTS) - 1u);
	#endif /* End tmr_event_INPUTS < 8u */
    tmr_event_Status_Mask = mask;
}


/*******************************************************************************
* Function Name: tmr_event_ReadMask
********************************************************************************
*
* Summary:
*  Reads the current interrupt mask assigned to the Status Register.
*
* Parameters:
*  None.
*
* Return:
*  The value of the interrupt mask of the Status Register.
*
*******************************************************************************/
uint8 tmr_event_ReadMask(void) 
{
    return tmr_event_Status_Mask;
}

#endif /* End check for removal by optimization */


/* [] END OF FILE */
