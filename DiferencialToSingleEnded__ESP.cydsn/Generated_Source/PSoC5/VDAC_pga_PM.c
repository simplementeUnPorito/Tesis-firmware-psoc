/*******************************************************************************
* File Name: VDAC_pga_PM.c  
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

#include "VDAC_pga.h"

static VDAC_pga_backupStruct VDAC_pga_backup;


/*******************************************************************************
* Function Name: VDAC_pga_SaveConfig
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
void VDAC_pga_SaveConfig(void) 
{
    if (!((VDAC_pga_CR1 & VDAC_pga_SRC_MASK) == VDAC_pga_SRC_UDB))
    {
        VDAC_pga_backup.data_value = VDAC_pga_Data;
    }
}


/*******************************************************************************
* Function Name: VDAC_pga_RestoreConfig
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
void VDAC_pga_RestoreConfig(void) 
{
    if (!((VDAC_pga_CR1 & VDAC_pga_SRC_MASK) == VDAC_pga_SRC_UDB))
    {
        if((VDAC_pga_Strobe & VDAC_pga_STRB_MASK) == VDAC_pga_STRB_EN)
        {
            VDAC_pga_Strobe &= (uint8)(~VDAC_pga_STRB_MASK);
            VDAC_pga_Data = VDAC_pga_backup.data_value;
            VDAC_pga_Strobe |= VDAC_pga_STRB_EN;
        }
        else
        {
            VDAC_pga_Data = VDAC_pga_backup.data_value;
        }
    }
}


/*******************************************************************************
* Function Name: VDAC_pga_Sleep
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
*  VDAC_pga_backup.enableState:  Is modified depending on the enable 
*  state  of the block before entering sleep mode.
*
*******************************************************************************/
void VDAC_pga_Sleep(void) 
{
    /* Save VDAC8's enable state */    
    if(VDAC_pga_ACT_PWR_EN == (VDAC_pga_PWRMGR & VDAC_pga_ACT_PWR_EN))
    {
        /* VDAC8 is enabled */
        VDAC_pga_backup.enableState = 1u;
    }
    else
    {
        /* VDAC8 is disabled */
        VDAC_pga_backup.enableState = 0u;
    }
    
    VDAC_pga_Stop();
    VDAC_pga_SaveConfig();
}


/*******************************************************************************
* Function Name: VDAC_pga_Wakeup
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
*  VDAC_pga_backup.enableState:  Is used to restore the enable state of 
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void VDAC_pga_Wakeup(void) 
{
    VDAC_pga_RestoreConfig();
    
    if(VDAC_pga_backup.enableState == 1u)
    {
        /* Enable VDAC8's operation */
        VDAC_pga_Enable();

        /* Restore the data register */
        VDAC_pga_SetValue(VDAC_pga_Data);
    } /* Do nothing if VDAC8 was disabled before */    
}


/* [] END OF FILE */
