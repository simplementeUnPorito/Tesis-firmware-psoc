/*******************************************************************************
* File Name: PGAgain_PM.c  
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

#include "PGAgain.h"

static PGAgain_BACKUP_STRUCT  PGAgain_backup;


/*******************************************************************************
* Function Name: PGAgain_SaveConfig
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
void PGAgain_SaveConfig(void) 
{
    /* Nothing to save as registers are System reset on retention flops */
}


/*******************************************************************************  
* Function Name: PGAgain_RestoreConfig
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
void PGAgain_RestoreConfig(void) 
{
    /* Nothing to restore */
}


/*******************************************************************************   
* Function Name: PGAgain_Sleep
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
*  PGAgain_backup: The structure field 'enableState' is modified 
*  depending on the enable state of the block before entering to sleep mode.
*
*******************************************************************************/
void PGAgain_Sleep(void) 
{
    /* Save PGA enable state */
    if((PGAgain_PM_ACT_CFG_REG & PGAgain_ACT_PWR_EN) != 0u)
    {
        /* Component is enabled */
        PGAgain_backup.enableState = 1u;
        /* Stop the configuration */
        PGAgain_Stop();
    }
    else
    {
        /* Component is disabled */
        PGAgain_backup.enableState = 0u;
    }
    /* Save the configuration */
    PGAgain_SaveConfig();
}


/*******************************************************************************
* Function Name: PGAgain_Wakeup
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
*  PGAgain_backup: The structure field 'enableState' is used to 
*  restore the enable state of block after wakeup from sleep mode.
* 
*******************************************************************************/
void PGAgain_Wakeup(void) 
{
    /* Restore the configurations */
    PGAgain_RestoreConfig();
     /* Enables the component operation */
    if(PGAgain_backup.enableState == 1u)
    {
        PGAgain_Enable();
    } /* Do nothing if component was disable before */
}


/* [] END OF FILE */
