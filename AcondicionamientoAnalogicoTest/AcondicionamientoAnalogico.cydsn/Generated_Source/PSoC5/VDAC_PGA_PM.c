/*******************************************************************************
* File Name: VDAC_PGA_PM.c  
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

#include "VDAC_PGA.h"

static VDAC_PGA_backupStruct VDAC_PGA_backup;


/*******************************************************************************
* Function Name: VDAC_PGA_SaveConfig
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
void VDAC_PGA_SaveConfig(void) 
{
    if (!((VDAC_PGA_CR1 & VDAC_PGA_SRC_MASK) == VDAC_PGA_SRC_UDB))
    {
        VDAC_PGA_backup.data_value = VDAC_PGA_Data;
    }
}


/*******************************************************************************
* Function Name: VDAC_PGA_RestoreConfig
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
void VDAC_PGA_RestoreConfig(void) 
{
    if (!((VDAC_PGA_CR1 & VDAC_PGA_SRC_MASK) == VDAC_PGA_SRC_UDB))
    {
        if((VDAC_PGA_Strobe & VDAC_PGA_STRB_MASK) == VDAC_PGA_STRB_EN)
        {
            VDAC_PGA_Strobe &= (uint8)(~VDAC_PGA_STRB_MASK);
            VDAC_PGA_Data = VDAC_PGA_backup.data_value;
            VDAC_PGA_Strobe |= VDAC_PGA_STRB_EN;
        }
        else
        {
            VDAC_PGA_Data = VDAC_PGA_backup.data_value;
        }
    }
}


/*******************************************************************************
* Function Name: VDAC_PGA_Sleep
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
*  VDAC_PGA_backup.enableState:  Is modified depending on the enable 
*  state  of the block before entering sleep mode.
*
*******************************************************************************/
void VDAC_PGA_Sleep(void) 
{
    /* Save VDAC8's enable state */    
    if(VDAC_PGA_ACT_PWR_EN == (VDAC_PGA_PWRMGR & VDAC_PGA_ACT_PWR_EN))
    {
        /* VDAC8 is enabled */
        VDAC_PGA_backup.enableState = 1u;
    }
    else
    {
        /* VDAC8 is disabled */
        VDAC_PGA_backup.enableState = 0u;
    }
    
    VDAC_PGA_Stop();
    VDAC_PGA_SaveConfig();
}


/*******************************************************************************
* Function Name: VDAC_PGA_Wakeup
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
*  VDAC_PGA_backup.enableState:  Is used to restore the enable state of 
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void VDAC_PGA_Wakeup(void) 
{
    VDAC_PGA_RestoreConfig();
    
    if(VDAC_PGA_backup.enableState == 1u)
    {
        /* Enable VDAC8's operation */
        VDAC_PGA_Enable();

        /* Restore the data register */
        VDAC_PGA_SetValue(VDAC_PGA_Data);
    } /* Do nothing if VDAC8 was disabled before */    
}


/* [] END OF FILE */
