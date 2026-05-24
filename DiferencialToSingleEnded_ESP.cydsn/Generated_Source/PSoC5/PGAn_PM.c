/*******************************************************************************
* File Name: PGAn_PM.c  
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

#include "PGAn.h"

static PGAn_BACKUP_STRUCT  PGAn_backup;


/*******************************************************************************
* Function Name: PGAn_SaveConfig
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
void PGAn_SaveConfig(void) 
{
    /* Nothing to save as registers are System reset on retention flops */
}


/*******************************************************************************  
* Function Name: PGAn_RestoreConfig
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
void PGAn_RestoreConfig(void) 
{
    /* Nothing to restore */
}


/*******************************************************************************   
* Function Name: PGAn_Sleep
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
*  PGAn_backup: The structure field 'enableState' is modified 
*  depending on the enable state of the block before entering to sleep mode.
*
*******************************************************************************/
void PGAn_Sleep(void) 
{
    /* Save PGA enable state */
    if((PGAn_PM_ACT_CFG_REG & PGAn_ACT_PWR_EN) != 0u)
    {
        /* Component is enabled */
        PGAn_backup.enableState = 1u;
        /* Stop the configuration */
        PGAn_Stop();
    }
    else
    {
        /* Component is disabled */
        PGAn_backup.enableState = 0u;
    }
    /* Save the configuration */
    PGAn_SaveConfig();
}


/*******************************************************************************
* Function Name: PGAn_Wakeup
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
*  PGAn_backup: The structure field 'enableState' is used to 
*  restore the enable state of block after wakeup from sleep mode.
* 
*******************************************************************************/
void PGAn_Wakeup(void) 
{
    /* Restore the configurations */
    PGAn_RestoreConfig();
     /* Enables the component operation */
    if(PGAn_backup.enableState == 1u)
    {
        PGAn_Enable();
    } /* Do nothing if component was disable before */
}


/* [] END OF FILE */
