/*******************************************************************************
* File Name: OPAlp_PM.c
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

#include "OPAlp.h"

static OPAlp_BACKUP_STRUCT  OPAlp_backup;


/*******************************************************************************  
* Function Name: OPAlp_SaveConfig
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
void OPAlp_SaveConfig(void) 
{
    /* Nothing to save as registers are System reset on retention flops */
}


/*******************************************************************************  
* Function Name: OPAlp_RestoreConfig
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
void OPAlp_RestoreConfig(void) 
{
    /* Nothing to restore */
}


/*******************************************************************************   
* Function Name: OPAlp_Sleep
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
*  OPAlp_backup: The structure field 'enableState' is modified 
*  depending on the enable state of the block before entering to sleep mode.
*
*******************************************************************************/
void OPAlp_Sleep(void) 
{
    /* Save OpAmp enable state */
    if((OPAlp_PM_ACT_CFG_REG & OPAlp_ACT_PWR_EN) != 0u)
    {
        /* Component is enabled */
        OPAlp_backup.enableState = 1u;
         /* Stops the component */
         OPAlp_Stop();
    }
    else
    {
        /* Component is disabled */
        OPAlp_backup.enableState = 0u;
    }
    /* Saves the configuration */
    OPAlp_SaveConfig();
}


/*******************************************************************************  
* Function Name: OPAlp_Wakeup
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
*  OPAlp_backup: The structure field 'enableState' is used to 
*  restore the enable state of block after wakeup from sleep mode.
*
*******************************************************************************/
void OPAlp_Wakeup(void) 
{
    /* Restore the user configuration */
    OPAlp_RestoreConfig();

    /* Enables the component operation */
    if(OPAlp_backup.enableState == 1u)
    {
        OPAlp_Enable();
    } /* Do nothing if component was disable before */
}


/* [] END OF FILE */
