/*******************************************************************************
* File Name: VDACIn_p_PM.c  
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

#include "VDACIn_p.h"

static VDACIn_p_backupStruct VDACIn_p_backup;


/*******************************************************************************
* Function Name: VDACIn_p_SaveConfig
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
void VDACIn_p_SaveConfig(void) 
{
    if (!((VDACIn_p_CR1 & VDACIn_p_SRC_MASK) == VDACIn_p_SRC_UDB))
    {
        VDACIn_p_backup.data_value = VDACIn_p_Data;
    }
}


/*******************************************************************************
* Function Name: VDACIn_p_RestoreConfig
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
void VDACIn_p_RestoreConfig(void) 
{
    if (!((VDACIn_p_CR1 & VDACIn_p_SRC_MASK) == VDACIn_p_SRC_UDB))
    {
        if((VDACIn_p_Strobe & VDACIn_p_STRB_MASK) == VDACIn_p_STRB_EN)
        {
            VDACIn_p_Strobe &= (uint8)(~VDACIn_p_STRB_MASK);
            VDACIn_p_Data = VDACIn_p_backup.data_value;
            VDACIn_p_Strobe |= VDACIn_p_STRB_EN;
        }
        else
        {
            VDACIn_p_Data = VDACIn_p_backup.data_value;
        }
    }
}


/*******************************************************************************
* Function Name: VDACIn_p_Sleep
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
*  VDACIn_p_backup.enableState:  Is modified depending on the enable 
*  state  of the block before entering sleep mode.
*
*******************************************************************************/
void VDACIn_p_Sleep(void) 
{
    /* Save VDAC8's enable state */    
    if(VDACIn_p_ACT_PWR_EN == (VDACIn_p_PWRMGR & VDACIn_p_ACT_PWR_EN))
    {
        /* VDAC8 is enabled */
        VDACIn_p_backup.enableState = 1u;
    }
    else
    {
        /* VDAC8 is disabled */
        VDACIn_p_backup.enableState = 0u;
    }
    
    VDACIn_p_Stop();
    VDACIn_p_SaveConfig();
}


/*******************************************************************************
* Function Name: VDACIn_p_Wakeup
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
*  VDACIn_p_backup.enableState:  Is used to restore the enable state of 
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void VDACIn_p_Wakeup(void) 
{
    VDACIn_p_RestoreConfig();
    
    if(VDACIn_p_backup.enableState == 1u)
    {
        /* Enable VDAC8's operation */
        VDACIn_p_Enable();

        /* Restore the data register */
        VDACIn_p_SetValue(VDACIn_p_Data);
    } /* Do nothing if VDAC8 was disabled before */    
}


/* [] END OF FILE */
