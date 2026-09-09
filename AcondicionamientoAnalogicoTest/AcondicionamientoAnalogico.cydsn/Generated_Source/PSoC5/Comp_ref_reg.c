/*******************************************************************************
* File Name: Comp_ref_reg.c  
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

#include "Comp_ref_reg.h"

#if !defined(Comp_ref_reg_sts_sts_reg__REMOVED) /* Check for removal by optimization */


/*******************************************************************************
* Function Name: Comp_ref_reg_Read
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
uint8 Comp_ref_reg_Read(void) 
{ 
    return Comp_ref_reg_Status;
}


/*******************************************************************************
* Function Name: Comp_ref_reg_InterruptEnable
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
void Comp_ref_reg_InterruptEnable(void) 
{
    uint8 interruptState;
    interruptState = CyEnterCriticalSection();
    Comp_ref_reg_Status_Aux_Ctrl |= Comp_ref_reg_STATUS_INTR_ENBL;
    CyExitCriticalSection(interruptState);
}


/*******************************************************************************
* Function Name: Comp_ref_reg_InterruptDisable
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
void Comp_ref_reg_InterruptDisable(void) 
{
    uint8 interruptState;
    interruptState = CyEnterCriticalSection();
    Comp_ref_reg_Status_Aux_Ctrl &= (uint8)(~Comp_ref_reg_STATUS_INTR_ENBL);
    CyExitCriticalSection(interruptState);
}


/*******************************************************************************
* Function Name: Comp_ref_reg_WriteMask
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
void Comp_ref_reg_WriteMask(uint8 mask) 
{
    #if(Comp_ref_reg_INPUTS < 8u)
    	mask &= ((uint8)(1u << Comp_ref_reg_INPUTS) - 1u);
	#endif /* End Comp_ref_reg_INPUTS < 8u */
    Comp_ref_reg_Status_Mask = mask;
}


/*******************************************************************************
* Function Name: Comp_ref_reg_ReadMask
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
uint8 Comp_ref_reg_ReadMask(void) 
{
    return Comp_ref_reg_Status_Mask;
}

#endif /* End check for removal by optimization */


/* [] END OF FILE */
