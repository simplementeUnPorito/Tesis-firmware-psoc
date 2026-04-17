/*******************************************************************************
* File Name: INA_PGA_p_PM.c  
* Version 2.0
*
* Description:
*  This file provides the power manager source code to the API for PGA 
*  Component.
*
* Note:
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#include "INA_PGA_p.h"

static INA_PGA_p_BACKUP_STRUCT  INA_PGA_p_backup;


/*******************************************************************************
* Function Name: INA_PGA_p_SaveConfig
********************************************************************************
*
* Summary:
*  Save the current user register configuration.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void INA_PGA_p_SaveConfig(void) 
{
    /* Nothing to save as registers are System reset on retention flops */
}


/*******************************************************************************  
* Function Name: INA_PGA_p_RestoreConfig
********************************************************************************
*
* Summary:
*  Restore the register configurations.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void INA_PGA_p_RestoreConfig(void) 
{
    /* Nothing to restore */
}


/*******************************************************************************   
* Function Name: INA_PGA_p_Sleep
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
*  INA_PGA_p_backup: The structure field 'enableState' is modified 
*  depending on the enable state of the block before entering to sleep mode.
*
*******************************************************************************/
void INA_PGA_p_Sleep(void) 
{
    /* Save PGA enable state */
    if((INA_PGA_p_PM_ACT_CFG_REG & INA_PGA_p_ACT_PWR_EN) != 0u)
    {
        /* Component is enabled */
        INA_PGA_p_backup.enableState = 1u;
        /* Stop the configuration */
        INA_PGA_p_Stop();
    }
    else
    {
        /* Component is disabled */
        INA_PGA_p_backup.enableState = 0u;
    }
    /* Save the configuration */
    INA_PGA_p_SaveConfig();
}


/*******************************************************************************
* Function Name: INA_PGA_p_Wakeup
********************************************************************************
*
* Summary:
*  Enables block's operation and restores its configuration. Should be called
*  just after awaking from sleep.
*
* Parameters:
*  None
*
* Return:
*  void
*
* Global variables:
*  INA_PGA_p_backup: The structure field 'enableState' is used to 
*  restore the enable state of block after wakeup from sleep mode.
* 
*******************************************************************************/
void INA_PGA_p_Wakeup(void) 
{
    /* Restore the configurations */
    INA_PGA_p_RestoreConfig();
     /* Enables the component operation */
    if(INA_PGA_p_backup.enableState == 1u)
    {
        INA_PGA_p_Enable();
    } /* Do nothing if component was disable before */
}


/* [] END OF FILE */
