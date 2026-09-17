/*******************************************************************************
* File Name: TIA_2_PM.c  
* Version 2.0
*
* Description:
*  This file provides the power management source code to the API for the 
*  TIA component.
*
* Note:
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#include "TIA_2.h"

static TIA_2_BACKUP_STRUCT  TIA_2_backup;


/*******************************************************************************  
* Function Name: TIA_2_SaveConfig
********************************************************************************
*
* Summary:
*  Saves the current user configuration registers.
*
* Parameters:
*  TIA_2_backup - global structure, where configuration registers are
*  stored.
*
* Return:
*  None
*
*******************************************************************************/
void TIA_2_SaveConfig(void) 
{
    /* Nothing to save as registers are System reset on retention flops */
}


/*******************************************************************************  
* Function Name: TIA_2_RestoreConfig
********************************************************************************
*
* Summary:
*  Restores the current user configuration.
*
* Parameters:
*  None
*
* Return:
*  void
*
*******************************************************************************/
void TIA_2_RestoreConfig(void) 
{
    /* Nothing to restore */
}


/*******************************************************************************   
* Function Name: TIA_2_Sleep
********************************************************************************
*
* Summary:
*  Disables block's operation and saves its configuration. Should be called 
*  just prior to entering sleep.
*
* Parameters:
*  None
*
* Return:
*  None
*
* Global variables:
*  TIA_2_backup: The structure field 'enableState' is modified 
*  depending on the enable state of the block before entering to sleep mode.
*
*******************************************************************************/
void TIA_2_Sleep(void) 
{
    /* Save TIA enable state */
    if((TIA_2_PM_ACT_CFG_REG & TIA_2_ACT_PWR_EN) != 0u)
    {
        /* Component is enabled */
        TIA_2_backup.enableState = 1u;
        /* Stop the configuration */
        TIA_2_Stop();
    }
    else
    {
        /* Component is disabled */
        TIA_2_backup.enableState = 0u;
    }
    /* Save the configuration */
    TIA_2_SaveConfig();
}


/*******************************************************************************  
* Function Name: TIA_2_Wakeup
********************************************************************************
*
* Summary:
*  Enables block's operation and restores its configuration. Should be called
*  just after awaking from sleep.
*
* Parameters:
*  None.
*
* Return:
*  void
*
* Global variables:
*  TIA_2_backup: The structure field 'enableState' is used to 
*  restore the enable state of block after wakeup from sleep mode.
* 
*******************************************************************************/
void TIA_2_Wakeup(void) 
{
    /* Restore the configuration */
    TIA_2_RestoreConfig();

    /* Enable's the component operation */
    if(TIA_2_backup.enableState == 1u)
    {
        TIA_2_Enable();
    } /* Do nothing if component was disabled before */
}


/* [] END OF FILE */
