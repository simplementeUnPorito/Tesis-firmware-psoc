/*******************************************************************************
* File Name: Opa_ref_2V5_PM.c
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

#include "Opa_ref_2V5.h"

static Opa_ref_2V5_BACKUP_STRUCT  Opa_ref_2V5_backup;


/*******************************************************************************  
* Function Name: Opa_ref_2V5_SaveConfig
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
void Opa_ref_2V5_SaveConfig(void) 
{
    /* Nothing to save as registers are System reset on retention flops */
}


/*******************************************************************************  
* Function Name: Opa_ref_2V5_RestoreConfig
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
void Opa_ref_2V5_RestoreConfig(void) 
{
    /* Nothing to restore */
}


/*******************************************************************************   
* Function Name: Opa_ref_2V5_Sleep
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
*  Opa_ref_2V5_backup: The structure field 'enableState' is modified 
*  depending on the enable state of the block before entering to sleep mode.
*
*******************************************************************************/
void Opa_ref_2V5_Sleep(void) 
{
    /* Save OpAmp enable state */
    if((Opa_ref_2V5_PM_ACT_CFG_REG & Opa_ref_2V5_ACT_PWR_EN) != 0u)
    {
        /* Component is enabled */
        Opa_ref_2V5_backup.enableState = 1u;
         /* Stops the component */
         Opa_ref_2V5_Stop();
    }
    else
    {
        /* Component is disabled */
        Opa_ref_2V5_backup.enableState = 0u;
    }
    /* Saves the configuration */
    Opa_ref_2V5_SaveConfig();
}


/*******************************************************************************  
* Function Name: Opa_ref_2V5_Wakeup
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
*  Opa_ref_2V5_backup: The structure field 'enableState' is used to 
*  restore the enable state of block after wakeup from sleep mode.
*
*******************************************************************************/
void Opa_ref_2V5_Wakeup(void) 
{
    /* Restore the user configuration */
    Opa_ref_2V5_RestoreConfig();

    /* Enables the component operation */
    if(Opa_ref_2V5_backup.enableState == 1u)
    {
        Opa_ref_2V5_Enable();
    } /* Do nothing if component was disable before */
}


/* [] END OF FILE */
