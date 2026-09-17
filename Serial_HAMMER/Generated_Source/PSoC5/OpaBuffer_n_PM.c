/*******************************************************************************
* File Name: OpaBuffer_n_PM.c
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

#include "OpaBuffer_n.h"

static OpaBuffer_n_BACKUP_STRUCT  OpaBuffer_n_backup;


/*******************************************************************************  
* Function Name: OpaBuffer_n_SaveConfig
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
void OpaBuffer_n_SaveConfig(void) 
{
    /* Nothing to save as registers are System reset on retention flops */
}


/*******************************************************************************  
* Function Name: OpaBuffer_n_RestoreConfig
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
void OpaBuffer_n_RestoreConfig(void) 
{
    /* Nothing to restore */
}


/*******************************************************************************   
* Function Name: OpaBuffer_n_Sleep
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
*  OpaBuffer_n_backup: The structure field 'enableState' is modified 
*  depending on the enable state of the block before entering to sleep mode.
*
*******************************************************************************/
void OpaBuffer_n_Sleep(void) 
{
    /* Save OpAmp enable state */
    if((OpaBuffer_n_PM_ACT_CFG_REG & OpaBuffer_n_ACT_PWR_EN) != 0u)
    {
        /* Component is enabled */
        OpaBuffer_n_backup.enableState = 1u;
         /* Stops the component */
         OpaBuffer_n_Stop();
    }
    else
    {
        /* Component is disabled */
        OpaBuffer_n_backup.enableState = 0u;
    }
    /* Saves the configuration */
    OpaBuffer_n_SaveConfig();
}


/*******************************************************************************  
* Function Name: OpaBuffer_n_Wakeup
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
*  OpaBuffer_n_backup: The structure field 'enableState' is used to 
*  restore the enable state of block after wakeup from sleep mode.
*
*******************************************************************************/
void OpaBuffer_n_Wakeup(void) 
{
    /* Restore the user configuration */
    OpaBuffer_n_RestoreConfig();

    /* Enables the component operation */
    if(OpaBuffer_n_backup.enableState == 1u)
    {
        OpaBuffer_n_Enable();
    } /* Do nothing if component was disable before */
}


/* [] END OF FILE */
