/*******************************************************************************
* File Name: VDACIn_n_PM.c  
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

#include "VDACIn_n.h"

static VDACIn_n_backupStruct VDACIn_n_backup;


/*******************************************************************************
* Function Name: VDACIn_n_SaveConfig
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
void VDACIn_n_SaveConfig(void) 
{
    if (!((VDACIn_n_CR1 & VDACIn_n_SRC_MASK) == VDACIn_n_SRC_UDB))
    {
        VDACIn_n_backup.data_value = VDACIn_n_Data;
    }
}


/*******************************************************************************
* Function Name: VDACIn_n_RestoreConfig
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
void VDACIn_n_RestoreConfig(void) 
{
    if (!((VDACIn_n_CR1 & VDACIn_n_SRC_MASK) == VDACIn_n_SRC_UDB))
    {
        if((VDACIn_n_Strobe & VDACIn_n_STRB_MASK) == VDACIn_n_STRB_EN)
        {
            VDACIn_n_Strobe &= (uint8)(~VDACIn_n_STRB_MASK);
            VDACIn_n_Data = VDACIn_n_backup.data_value;
            VDACIn_n_Strobe |= VDACIn_n_STRB_EN;
        }
        else
        {
            VDACIn_n_Data = VDACIn_n_backup.data_value;
        }
    }
}


/*******************************************************************************
* Function Name: VDACIn_n_Sleep
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
*  VDACIn_n_backup.enableState:  Is modified depending on the enable 
*  state  of the block before entering sleep mode.
*
*******************************************************************************/
void VDACIn_n_Sleep(void) 
{
    /* Save VDAC8's enable state */    
    if(VDACIn_n_ACT_PWR_EN == (VDACIn_n_PWRMGR & VDACIn_n_ACT_PWR_EN))
    {
        /* VDAC8 is enabled */
        VDACIn_n_backup.enableState = 1u;
    }
    else
    {
        /* VDAC8 is disabled */
        VDACIn_n_backup.enableState = 0u;
    }
    
    VDACIn_n_Stop();
    VDACIn_n_SaveConfig();
}


/*******************************************************************************
* Function Name: VDACIn_n_Wakeup
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
*  VDACIn_n_backup.enableState:  Is used to restore the enable state of 
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void VDACIn_n_Wakeup(void) 
{
    VDACIn_n_RestoreConfig();
    
    if(VDACIn_n_backup.enableState == 1u)
    {
        /* Enable VDAC8's operation */
        VDACIn_n_Enable();

        /* Restore the data register */
        VDACIn_n_SetValue(VDACIn_n_Data);
    } /* Do nothing if VDAC8 was disabled before */    
}


/* [] END OF FILE */
