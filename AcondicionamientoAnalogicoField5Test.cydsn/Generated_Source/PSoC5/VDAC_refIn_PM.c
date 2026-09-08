/*******************************************************************************
* File Name: VDAC_refIn_PM.c  
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

#include "VDAC_refIn.h"

static VDAC_refIn_backupStruct VDAC_refIn_backup;


/*******************************************************************************
* Function Name: VDAC_refIn_SaveConfig
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
void VDAC_refIn_SaveConfig(void) 
{
    if (!((VDAC_refIn_CR1 & VDAC_refIn_SRC_MASK) == VDAC_refIn_SRC_UDB))
    {
        VDAC_refIn_backup.data_value = VDAC_refIn_Data;
    }
}


/*******************************************************************************
* Function Name: VDAC_refIn_RestoreConfig
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
void VDAC_refIn_RestoreConfig(void) 
{
    if (!((VDAC_refIn_CR1 & VDAC_refIn_SRC_MASK) == VDAC_refIn_SRC_UDB))
    {
        if((VDAC_refIn_Strobe & VDAC_refIn_STRB_MASK) == VDAC_refIn_STRB_EN)
        {
            VDAC_refIn_Strobe &= (uint8)(~VDAC_refIn_STRB_MASK);
            VDAC_refIn_Data = VDAC_refIn_backup.data_value;
            VDAC_refIn_Strobe |= VDAC_refIn_STRB_EN;
        }
        else
        {
            VDAC_refIn_Data = VDAC_refIn_backup.data_value;
        }
    }
}


/*******************************************************************************
* Function Name: VDAC_refIn_Sleep
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
*  VDAC_refIn_backup.enableState:  Is modified depending on the enable 
*  state  of the block before entering sleep mode.
*
*******************************************************************************/
void VDAC_refIn_Sleep(void) 
{
    /* Save VDAC8's enable state */    
    if(VDAC_refIn_ACT_PWR_EN == (VDAC_refIn_PWRMGR & VDAC_refIn_ACT_PWR_EN))
    {
        /* VDAC8 is enabled */
        VDAC_refIn_backup.enableState = 1u;
    }
    else
    {
        /* VDAC8 is disabled */
        VDAC_refIn_backup.enableState = 0u;
    }
    
    VDAC_refIn_Stop();
    VDAC_refIn_SaveConfig();
}


/*******************************************************************************
* Function Name: VDAC_refIn_Wakeup
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
*  VDAC_refIn_backup.enableState:  Is used to restore the enable state of 
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void VDAC_refIn_Wakeup(void) 
{
    VDAC_refIn_RestoreConfig();
    
    if(VDAC_refIn_backup.enableState == 1u)
    {
        /* Enable VDAC8's operation */
        VDAC_refIn_Enable();

        /* Restore the data register */
        VDAC_refIn_SetValue(VDAC_refIn_Data);
    } /* Do nothing if VDAC8 was disabled before */    
}


/* [] END OF FILE */
