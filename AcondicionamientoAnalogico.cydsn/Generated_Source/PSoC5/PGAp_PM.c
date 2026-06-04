/*******************************************************************************
* File Name: PGAp_PM.c  
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

#include "PGAp.h"

static PGAp_BACKUP_STRUCT  PGAp_backup;


/*******************************************************************************
* Function Name: PGAp_SaveConfig
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
void PGAp_SaveConfig(void) 
{
    /* Nothing to save as registers are System reset on retention flops */
}


/*******************************************************************************  
* Function Name: PGAp_RestoreConfig
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
void PGAp_RestoreConfig(void) 
{
    /* Nothing to restore */
}


/*******************************************************************************   
* Function Name: PGAp_Sleep
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
*  PGAp_backup: The structure field 'enableState' is modified 
*  depending on the enable state of the block before entering to sleep mode.
*
*******************************************************************************/
void PGAp_Sleep(void) 
{
    /* Save PGA enable state */
    if((PGAp_PM_ACT_CFG_REG & PGAp_ACT_PWR_EN) != 0u)
    {
        /* Component is enabled */
        PGAp_backup.enableState = 1u;
        /* Stop the configuration */
        PGAp_Stop();
    }
    else
    {
        /* Component is disabled */
        PGAp_backup.enableState = 0u;
    }
    /* Save the configuration */
    PGAp_SaveConfig();
}


/*******************************************************************************
* Function Name: PGAp_Wakeup
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
*  PGAp_backup: The structure field 'enableState' is used to 
*  restore the enable state of block after wakeup from sleep mode.
* 
*******************************************************************************/
void PGAp_Wakeup(void) 
{
    /* Restore the configurations */
    PGAp_RestoreConfig();
     /* Enables the component operation */
    if(PGAp_backup.enableState == 1u)
    {
        PGAp_Enable();
    } /* Do nothing if component was disable before */
}


/* [] END OF FILE */
