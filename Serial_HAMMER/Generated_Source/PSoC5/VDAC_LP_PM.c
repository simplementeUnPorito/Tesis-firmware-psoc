/*******************************************************************************
* File Name: VDAC_LP_PM.c  
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

#include "VDAC_LP.h"

static VDAC_LP_backupStruct VDAC_LP_backup;


/*******************************************************************************
* Function Name: VDAC_LP_SaveConfig
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
void VDAC_LP_SaveConfig(void) 
{
    if (!((VDAC_LP_CR1 & VDAC_LP_SRC_MASK) == VDAC_LP_SRC_UDB))
    {
        VDAC_LP_backup.data_value = VDAC_LP_Data;
    }
}


/*******************************************************************************
* Function Name: VDAC_LP_RestoreConfig
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
void VDAC_LP_RestoreConfig(void) 
{
    if (!((VDAC_LP_CR1 & VDAC_LP_SRC_MASK) == VDAC_LP_SRC_UDB))
    {
        if((VDAC_LP_Strobe & VDAC_LP_STRB_MASK) == VDAC_LP_STRB_EN)
        {
            VDAC_LP_Strobe &= (uint8)(~VDAC_LP_STRB_MASK);
            VDAC_LP_Data = VDAC_LP_backup.data_value;
            VDAC_LP_Strobe |= VDAC_LP_STRB_EN;
        }
        else
        {
            VDAC_LP_Data = VDAC_LP_backup.data_value;
        }
    }
}


/*******************************************************************************
* Function Name: VDAC_LP_Sleep
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
*  VDAC_LP_backup.enableState:  Is modified depending on the enable 
*  state  of the block before entering sleep mode.
*
*******************************************************************************/
void VDAC_LP_Sleep(void) 
{
    /* Save VDAC8's enable state */    
    if(VDAC_LP_ACT_PWR_EN == (VDAC_LP_PWRMGR & VDAC_LP_ACT_PWR_EN))
    {
        /* VDAC8 is enabled */
        VDAC_LP_backup.enableState = 1u;
    }
    else
    {
        /* VDAC8 is disabled */
        VDAC_LP_backup.enableState = 0u;
    }
    
    VDAC_LP_Stop();
    VDAC_LP_SaveConfig();
}


/*******************************************************************************
* Function Name: VDAC_LP_Wakeup
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
*  VDAC_LP_backup.enableState:  Is used to restore the enable state of 
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void VDAC_LP_Wakeup(void) 
{
    VDAC_LP_RestoreConfig();
    
    if(VDAC_LP_backup.enableState == 1u)
    {
        /* Enable VDAC8's operation */
        VDAC_LP_Enable();

        /* Restore the data register */
        VDAC_LP_SetValue(VDAC_LP_Data);
    } /* Do nothing if VDAC8 was disabled before */    
}


/* [] END OF FILE */
