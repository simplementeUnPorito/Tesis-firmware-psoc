/*******************************************************************************
* File Name: INA_Opamp_PM.c
* Version 1.90
*
* Description:
*  This file provides the power management source code to the API for the 
*  OpAmp (Analog Buffer) component.
*
* Note:
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#include "INA_Opamp.h"

static INA_Opamp_BACKUP_STRUCT  INA_Opamp_backup;


/*******************************************************************************  
* Function Name: INA_Opamp_SaveConfig
********************************************************************************
*
* Summary:
*  Saves the current user configuration registers.
* 
* Parameters:
*  void
* 
* Return:
*  void
*
*******************************************************************************/
void INA_Opamp_SaveConfig(void) 
{
    /* Nothing to save as registers are System reset on retention flops */
}


/*******************************************************************************  
* Function Name: INA_Opamp_RestoreConfig
********************************************************************************
*
* Summary:
*  Restores the current user configuration registers.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void INA_Opamp_RestoreConfig(void) 
{
    /* Nothing to restore */
}


/*******************************************************************************   
* Function Name: INA_Opamp_Sleep
********************************************************************************
*
* Summary:
*  Disables block's operation and saves its configuration. Should be called 
*  just prior to entering sleep.
*
* Parameters:
*  void
*
* Return:
*  void
*
* Global variables:
*  INA_Opamp_backup: The structure field 'enableState' is modified 
*  depending on the enable state of the block before entering to sleep mode.
*
*******************************************************************************/
void INA_Opamp_Sleep(void) 
{
    /* Save OpAmp enable state */
    if((INA_Opamp_PM_ACT_CFG_REG & INA_Opamp_ACT_PWR_EN) != 0u)
    {
        /* Component is enabled */
        INA_Opamp_backup.enableState = 1u;
         /* Stops the component */
         INA_Opamp_Stop();
    }
    else
    {
        /* Component is disabled */
        INA_Opamp_backup.enableState = 0u;
    }
    /* Saves the configuration */
    INA_Opamp_SaveConfig();
}


/*******************************************************************************  
* Function Name: INA_Opamp_Wakeup
********************************************************************************
*
* Summary:
*  Enables block's operation and restores its configuration. Should be called
*  just after awaking from sleep.
*
* Parameters:
*  void
*
* Return:
*  void
*
* Global variables:
*  INA_Opamp_backup: The structure field 'enableState' is used to 
*  restore the enable state of block after wakeup from sleep mode.
*
*******************************************************************************/
void INA_Opamp_Wakeup(void) 
{
    /* Restore the user configuration */
    INA_Opamp_RestoreConfig();

    /* Enables the component operation */
    if(INA_Opamp_backup.enableState == 1u)
    {
        INA_Opamp_Enable();
    } /* Do nothing if component was disable before */
}


/* [] END OF FILE */
