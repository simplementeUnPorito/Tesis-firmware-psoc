/*******************************************************************************
* File Name: VDAC_n_PM.c  
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

#include "VDAC_n.h"

static VDAC_n_backupStruct VDAC_n_backup;


/*******************************************************************************
* Function Name: VDAC_n_SaveConfig
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
void VDAC_n_SaveConfig(void) 
{
    if (!((VDAC_n_CR1 & VDAC_n_SRC_MASK) == VDAC_n_SRC_UDB))
    {
        VDAC_n_backup.data_value = VDAC_n_Data;
    }
}


/*******************************************************************************
* Function Name: VDAC_n_RestoreConfig
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
void VDAC_n_RestoreConfig(void) 
{
    if (!((VDAC_n_CR1 & VDAC_n_SRC_MASK) == VDAC_n_SRC_UDB))
    {
        if((VDAC_n_Strobe & VDAC_n_STRB_MASK) == VDAC_n_STRB_EN)
        {
            VDAC_n_Strobe &= (uint8)(~VDAC_n_STRB_MASK);
            VDAC_n_Data = VDAC_n_backup.data_value;
            VDAC_n_Strobe |= VDAC_n_STRB_EN;
        }
        else
        {
            VDAC_n_Data = VDAC_n_backup.data_value;
        }
    }
}


/*******************************************************************************
* Function Name: VDAC_n_Sleep
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
*  VDAC_n_backup.enableState:  Is modified depending on the enable 
*  state  of the block before entering sleep mode.
*
*******************************************************************************/
void VDAC_n_Sleep(void) 
{
    /* Save VDAC8's enable state */    
    if(VDAC_n_ACT_PWR_EN == (VDAC_n_PWRMGR & VDAC_n_ACT_PWR_EN))
    {
        /* VDAC8 is enabled */
        VDAC_n_backup.enableState = 1u;
    }
    else
    {
        /* VDAC8 is disabled */
        VDAC_n_backup.enableState = 0u;
    }
    
    VDAC_n_Stop();
    VDAC_n_SaveConfig();
}


/*******************************************************************************
* Function Name: VDAC_n_Wakeup
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
*  VDAC_n_backup.enableState:  Is used to restore the enable state of 
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void VDAC_n_Wakeup(void) 
{
    VDAC_n_RestoreConfig();
    
    if(VDAC_n_backup.enableState == 1u)
    {
        /* Enable VDAC8's operation */
        VDAC_n_Enable();

        /* Restore the data register */
        VDAC_n_SetValue(VDAC_n_Data);
    } /* Do nothing if VDAC8 was disabled before */    
}


/* [] END OF FILE */
