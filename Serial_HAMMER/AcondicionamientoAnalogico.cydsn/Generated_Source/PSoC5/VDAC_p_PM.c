/*******************************************************************************
* File Name: VDAC_p_PM.c  
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

#include "VDAC_p.h"

static VDAC_p_backupStruct VDAC_p_backup;


/*******************************************************************************
* Function Name: VDAC_p_SaveConfig
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
void VDAC_p_SaveConfig(void) 
{
    if (!((VDAC_p_CR1 & VDAC_p_SRC_MASK) == VDAC_p_SRC_UDB))
    {
        VDAC_p_backup.data_value = VDAC_p_Data;
    }
}


/*******************************************************************************
* Function Name: VDAC_p_RestoreConfig
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
void VDAC_p_RestoreConfig(void) 
{
    if (!((VDAC_p_CR1 & VDAC_p_SRC_MASK) == VDAC_p_SRC_UDB))
    {
        if((VDAC_p_Strobe & VDAC_p_STRB_MASK) == VDAC_p_STRB_EN)
        {
            VDAC_p_Strobe &= (uint8)(~VDAC_p_STRB_MASK);
            VDAC_p_Data = VDAC_p_backup.data_value;
            VDAC_p_Strobe |= VDAC_p_STRB_EN;
        }
        else
        {
            VDAC_p_Data = VDAC_p_backup.data_value;
        }
    }
}


/*******************************************************************************
* Function Name: VDAC_p_Sleep
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
*  VDAC_p_backup.enableState:  Is modified depending on the enable 
*  state  of the block before entering sleep mode.
*
*******************************************************************************/
void VDAC_p_Sleep(void) 
{
    /* Save VDAC8's enable state */    
    if(VDAC_p_ACT_PWR_EN == (VDAC_p_PWRMGR & VDAC_p_ACT_PWR_EN))
    {
        /* VDAC8 is enabled */
        VDAC_p_backup.enableState = 1u;
    }
    else
    {
        /* VDAC8 is disabled */
        VDAC_p_backup.enableState = 0u;
    }
    
    VDAC_p_Stop();
    VDAC_p_SaveConfig();
}


/*******************************************************************************
* Function Name: VDAC_p_Wakeup
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
*  VDAC_p_backup.enableState:  Is used to restore the enable state of 
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void VDAC_p_Wakeup(void) 
{
    VDAC_p_RestoreConfig();
    
    if(VDAC_p_backup.enableState == 1u)
    {
        /* Enable VDAC8's operation */
        VDAC_p_Enable();

        /* Restore the data register */
        VDAC_p_SetValue(VDAC_p_Data);
    } /* Do nothing if VDAC8 was disabled before */    
}


/* [] END OF FILE */
