/*******************************************************************************
* File Name: VDAC_Ref_Adder_PM.c  
* Version 1.90
*
* Description:
*  This file provides the power management source code to API for the
*  VDAC8.  
*
* Note:
*  None
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#include "VDAC_Ref_Adder.h"

static VDAC_Ref_Adder_backupStruct VDAC_Ref_Adder_backup;


/*******************************************************************************
* Function Name: VDAC_Ref_Adder_SaveConfig
********************************************************************************
* Summary:
*  Save the current user configuration
*
* Parameters:  
*  void  
*
* Return: 
*  void
*
*******************************************************************************/
void VDAC_Ref_Adder_SaveConfig(void) 
{
    if (!((VDAC_Ref_Adder_CR1 & VDAC_Ref_Adder_SRC_MASK) == VDAC_Ref_Adder_SRC_UDB))
    {
        VDAC_Ref_Adder_backup.data_value = VDAC_Ref_Adder_Data;
    }
}


/*******************************************************************************
* Function Name: VDAC_Ref_Adder_RestoreConfig
********************************************************************************
*
* Summary:
*  Restores the current user configuration.
*
* Parameters:  
*  void
*
* Return: 
*  void
*
*******************************************************************************/
void VDAC_Ref_Adder_RestoreConfig(void) 
{
    if (!((VDAC_Ref_Adder_CR1 & VDAC_Ref_Adder_SRC_MASK) == VDAC_Ref_Adder_SRC_UDB))
    {
        if((VDAC_Ref_Adder_Strobe & VDAC_Ref_Adder_STRB_MASK) == VDAC_Ref_Adder_STRB_EN)
        {
            VDAC_Ref_Adder_Strobe &= (uint8)(~VDAC_Ref_Adder_STRB_MASK);
            VDAC_Ref_Adder_Data = VDAC_Ref_Adder_backup.data_value;
            VDAC_Ref_Adder_Strobe |= VDAC_Ref_Adder_STRB_EN;
        }
        else
        {
            VDAC_Ref_Adder_Data = VDAC_Ref_Adder_backup.data_value;
        }
    }
}


/*******************************************************************************
* Function Name: VDAC_Ref_Adder_Sleep
********************************************************************************
* Summary:
*  Stop and Save the user configuration
*
* Parameters:  
*  void:  
*
* Return: 
*  void
*
* Global variables:
*  VDAC_Ref_Adder_backup.enableState:  Is modified depending on the enable 
*  state  of the block before entering sleep mode.
*
*******************************************************************************/
void VDAC_Ref_Adder_Sleep(void) 
{
    /* Save VDAC8's enable state */    
    if(VDAC_Ref_Adder_ACT_PWR_EN == (VDAC_Ref_Adder_PWRMGR & VDAC_Ref_Adder_ACT_PWR_EN))
    {
        /* VDAC8 is enabled */
        VDAC_Ref_Adder_backup.enableState = 1u;
    }
    else
    {
        /* VDAC8 is disabled */
        VDAC_Ref_Adder_backup.enableState = 0u;
    }
    
    VDAC_Ref_Adder_Stop();
    VDAC_Ref_Adder_SaveConfig();
}


/*******************************************************************************
* Function Name: VDAC_Ref_Adder_Wakeup
********************************************************************************
*
* Summary:
*  Restores and enables the user configuration
*  
* Parameters:  
*  void
*
* Return: 
*  void
*
* Global variables:
*  VDAC_Ref_Adder_backup.enableState:  Is used to restore the enable state of 
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void VDAC_Ref_Adder_Wakeup(void) 
{
    VDAC_Ref_Adder_RestoreConfig();
    
    if(VDAC_Ref_Adder_backup.enableState == 1u)
    {
        /* Enable VDAC8's operation */
        VDAC_Ref_Adder_Enable();

        /* Restore the data register */
        VDAC_Ref_Adder_SetValue(VDAC_Ref_Adder_Data);
    } /* Do nothing if VDAC8 was disabled before */    
}


/* [] END OF FILE */
