/*******************************************************************************
* File Name: Opa_SAR_PM.c
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

#include "Opa_SAR.h"

static Opa_SAR_BACKUP_STRUCT  Opa_SAR_backup;


/*******************************************************************************  
* Function Name: Opa_SAR_SaveConfig
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
void Opa_SAR_SaveConfig(void) 
{
    /* Nothing to save as registers are System reset on retention flops */
}


/*******************************************************************************  
* Function Name: Opa_SAR_RestoreConfig
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
void Opa_SAR_RestoreConfig(void) 
{
    /* Nothing to restore */
}


/*******************************************************************************   
* Function Name: Opa_SAR_Sleep
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
*  Opa_SAR_backup: The structure field 'enableState' is modified 
*  depending on the enable state of the block before entering to sleep mode.
*
*******************************************************************************/
void Opa_SAR_Sleep(void) 
{
    /* Save OpAmp enable state */
    if((Opa_SAR_PM_ACT_CFG_REG & Opa_SAR_ACT_PWR_EN) != 0u)
    {
        /* Component is enabled */
        Opa_SAR_backup.enableState = 1u;
         /* Stops the component */
         Opa_SAR_Stop();
    }
    else
    {
        /* Component is disabled */
        Opa_SAR_backup.enableState = 0u;
    }
    /* Saves the configuration */
    Opa_SAR_SaveConfig();
}


/*******************************************************************************  
* Function Name: Opa_SAR_Wakeup
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
*  Opa_SAR_backup: The structure field 'enableState' is used to 
*  restore the enable state of block after wakeup from sleep mode.
*
*******************************************************************************/
void Opa_SAR_Wakeup(void) 
{
    /* Restore the user configuration */
    Opa_SAR_RestoreConfig();

    /* Enables the component operation */
    if(Opa_SAR_backup.enableState == 1u)
    {
        Opa_SAR_Enable();
    } /* Do nothing if component was disable before */
}


/* [] END OF FILE */
