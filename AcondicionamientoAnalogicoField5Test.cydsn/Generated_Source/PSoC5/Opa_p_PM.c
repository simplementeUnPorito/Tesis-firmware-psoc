/*******************************************************************************
* File Name: Opa_p_PM.c
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

#include "Opa_p.h"

static Opa_p_BACKUP_STRUCT  Opa_p_backup;


/*******************************************************************************  
* Function Name: Opa_p_SaveConfig
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
void Opa_p_SaveConfig(void) 
{
    /* Nothing to save as registers are System reset on retention flops */
}


/*******************************************************************************  
* Function Name: Opa_p_RestoreConfig
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
void Opa_p_RestoreConfig(void) 
{
    /* Nothing to restore */
}


/*******************************************************************************   
* Function Name: Opa_p_Sleep
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
*  Opa_p_backup: The structure field 'enableState' is modified 
*  depending on the enable state of the block before entering to sleep mode.
*
*******************************************************************************/
void Opa_p_Sleep(void) 
{
    /* Save OpAmp enable state */
    if((Opa_p_PM_ACT_CFG_REG & Opa_p_ACT_PWR_EN) != 0u)
    {
        /* Component is enabled */
        Opa_p_backup.enableState = 1u;
         /* Stops the component */
         Opa_p_Stop();
    }
    else
    {
        /* Component is disabled */
        Opa_p_backup.enableState = 0u;
    }
    /* Saves the configuration */
    Opa_p_SaveConfig();
}


/*******************************************************************************  
* Function Name: Opa_p_Wakeup
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
*  Opa_p_backup: The structure field 'enableState' is used to 
*  restore the enable state of block after wakeup from sleep mode.
*
*******************************************************************************/
void Opa_p_Wakeup(void) 
{
    /* Restore the user configuration */
    Opa_p_RestoreConfig();

    /* Enables the component operation */
    if(Opa_p_backup.enableState == 1u)
    {
        Opa_p_Enable();
    } /* Do nothing if component was disable before */
}


/* [] END OF FILE */
