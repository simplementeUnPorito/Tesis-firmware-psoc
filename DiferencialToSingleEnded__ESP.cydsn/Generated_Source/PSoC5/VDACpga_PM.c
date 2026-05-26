/*******************************************************************************
* File Name: VDACpga_PM.c  
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

#include "VDACpga.h"

static VDACpga_backupStruct VDACpga_backup;


/*******************************************************************************
* Function Name: VDACpga_SaveConfig
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
void VDACpga_SaveConfig(void) 
{
    if (!((VDACpga_CR1 & VDACpga_SRC_MASK) == VDACpga_SRC_UDB))
    {
        VDACpga_backup.data_value = VDACpga_Data;
    }
}


/*******************************************************************************
* Function Name: VDACpga_RestoreConfig
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
void VDACpga_RestoreConfig(void) 
{
    if (!((VDACpga_CR1 & VDACpga_SRC_MASK) == VDACpga_SRC_UDB))
    {
        if((VDACpga_Strobe & VDACpga_STRB_MASK) == VDACpga_STRB_EN)
        {
            VDACpga_Strobe &= (uint8)(~VDACpga_STRB_MASK);
            VDACpga_Data = VDACpga_backup.data_value;
            VDACpga_Strobe |= VDACpga_STRB_EN;
        }
        else
        {
            VDACpga_Data = VDACpga_backup.data_value;
        }
    }
}


/*******************************************************************************
* Function Name: VDACpga_Sleep
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
*  VDACpga_backup.enableState:  Is modified depending on the enable 
*  state  of the block before entering sleep mode.
*
*******************************************************************************/
void VDACpga_Sleep(void) 
{
    /* Save VDAC8's enable state */    
    if(VDACpga_ACT_PWR_EN == (VDACpga_PWRMGR & VDACpga_ACT_PWR_EN))
    {
        /* VDAC8 is enabled */
        VDACpga_backup.enableState = 1u;
    }
    else
    {
        /* VDAC8 is disabled */
        VDACpga_backup.enableState = 0u;
    }
    
    VDACpga_Stop();
    VDACpga_SaveConfig();
}


/*******************************************************************************
* Function Name: VDACpga_Wakeup
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
*  VDACpga_backup.enableState:  Is used to restore the enable state of 
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void VDACpga_Wakeup(void) 
{
    VDACpga_RestoreConfig();
    
    if(VDACpga_backup.enableState == 1u)
    {
        /* Enable VDAC8's operation */
        VDACpga_Enable();

        /* Restore the data register */
        VDACpga_SetValue(VDACpga_Data);
    } /* Do nothing if VDAC8 was disabled before */    
}


/* [] END OF FILE */
