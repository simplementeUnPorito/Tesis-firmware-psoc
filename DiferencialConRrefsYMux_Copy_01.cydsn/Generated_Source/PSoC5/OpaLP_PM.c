/*******************************************************************************
* File Name: OpaLP_PM.c
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

#include "OpaLP.h"

static OpaLP_BACKUP_STRUCT  OpaLP_backup;


/*******************************************************************************  
* Function Name: OpaLP_SaveConfig
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
void OpaLP_SaveConfig(void) 
{
    /* Nothing to save as registers are System reset on retention flops */
}


/*******************************************************************************  
* Function Name: OpaLP_RestoreConfig
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
void OpaLP_RestoreConfig(void) 
{
    /* Nothing to restore */
}


/*******************************************************************************   
* Function Name: OpaLP_Sleep
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
*  OpaLP_backup: The structure field 'enableState' is modified 
*  depending on the enable state of the block before entering to sleep mode.
*
*******************************************************************************/
void OpaLP_Sleep(void) 
{
    /* Save OpAmp enable state */
    if((OpaLP_PM_ACT_CFG_REG & OpaLP_ACT_PWR_EN) != 0u)
    {
        /* Component is enabled */
        OpaLP_backup.enableState = 1u;
         /* Stops the component */
         OpaLP_Stop();
    }
    else
    {
        /* Component is disabled */
        OpaLP_backup.enableState = 0u;
    }
    /* Saves the configuration */
    OpaLP_SaveConfig();
}


/*******************************************************************************  
* Function Name: OpaLP_Wakeup
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
*  OpaLP_backup: The structure field 'enableState' is used to 
*  restore the enable state of block after wakeup from sleep mode.
*
*******************************************************************************/
void OpaLP_Wakeup(void) 
{
    /* Restore the user configuration */
    OpaLP_RestoreConfig();

    /* Enables the component operation */
    if(OpaLP_backup.enableState == 1u)
    {
        OpaLP_Enable();
    } /* Do nothing if component was disable before */
}


/* [] END OF FILE */
