/*******************************************************************************
* File Name: Opa_comun_PM.c
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

#include "Opa_comun.h"

static Opa_comun_BACKUP_STRUCT  Opa_comun_backup;


/*******************************************************************************  
* Function Name: Opa_comun_SaveConfig
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
void Opa_comun_SaveConfig(void) 
{
    /* Nothing to save as registers are System reset on retention flops */
}


/*******************************************************************************  
* Function Name: Opa_comun_RestoreConfig
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
void Opa_comun_RestoreConfig(void) 
{
    /* Nothing to restore */
}


/*******************************************************************************   
* Function Name: Opa_comun_Sleep
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
*  Opa_comun_backup: The structure field 'enableState' is modified 
*  depending on the enable state of the block before entering to sleep mode.
*
*******************************************************************************/
void Opa_comun_Sleep(void) 
{
    /* Save OpAmp enable state */
    if((Opa_comun_PM_ACT_CFG_REG & Opa_comun_ACT_PWR_EN) != 0u)
    {
        /* Component is enabled */
        Opa_comun_backup.enableState = 1u;
         /* Stops the component */
         Opa_comun_Stop();
    }
    else
    {
        /* Component is disabled */
        Opa_comun_backup.enableState = 0u;
    }
    /* Saves the configuration */
    Opa_comun_SaveConfig();
}


/*******************************************************************************  
* Function Name: Opa_comun_Wakeup
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
*  Opa_comun_backup: The structure field 'enableState' is used to 
*  restore the enable state of block after wakeup from sleep mode.
*
*******************************************************************************/
void Opa_comun_Wakeup(void) 
{
    /* Restore the user configuration */
    Opa_comun_RestoreConfig();

    /* Enables the component operation */
    if(Opa_comun_backup.enableState == 1u)
    {
        Opa_comun_Enable();
    } /* Do nothing if component was disable before */
}


/* [] END OF FILE */
