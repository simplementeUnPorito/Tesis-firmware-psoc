/*******************************************************************************
* File Name: Comp_LP.c
* Version 2.0
*
* Description:
*  This file provides the power management source code APIs for the
*  Comparator.
*
* Note:
*  None
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#include "Comp_LP.h"

static Comp_LP_backupStruct Comp_LP_backup;


/*******************************************************************************
* Function Name: Comp_LP_SaveConfig
********************************************************************************
*
* Summary:
*  Save the current user configuration
*
* Parameters:
*  void:
*
* Return:
*  void
*
*******************************************************************************/
void Comp_LP_SaveConfig(void) 
{
    /* Empty since all are system reset for retention flops */
}


/*******************************************************************************
* Function Name: Comp_LP_RestoreConfig
********************************************************************************
*
* Summary:
*  Restores the current user configuration.
*
* Parameters:
*  void
*
* Return:
*  void
*
********************************************************************************/
void Comp_LP_RestoreConfig(void) 
{
    /* Empty since all are system reset for retention flops */    
}


/*******************************************************************************
* Function Name: Comp_LP_Sleep
********************************************************************************
*
* Summary:
*  Stop and Save the user configuration
*
* Parameters:
*  void:
*
* Return:
*  void
*
* Global variables:
*  Comp_LP_backup.enableState:  Is modified depending on the enable 
*   state of the block before entering sleep mode.
*
*******************************************************************************/
void Comp_LP_Sleep(void) 
{
    /* Save Comp's enable state */    
    if(Comp_LP_ACT_PWR_EN == (Comp_LP_PWRMGR & Comp_LP_ACT_PWR_EN))
    {
        /* Comp is enabled */
        Comp_LP_backup.enableState = 1u;
    }
    else
    {
        /* Comp is disabled */
        Comp_LP_backup.enableState = 0u;
    }    
    
    Comp_LP_Stop();
    Comp_LP_SaveConfig();
}


/*******************************************************************************
* Function Name: Comp_LP_Wakeup
********************************************************************************
*
* Summary:
*  Restores and enables the user configuration
*  
* Parameters:
*  void
*
* Return:
*  void
*
* Global variables:
*  Comp_LP_backup.enableState:  Is used to restore the enable state of 
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void Comp_LP_Wakeup(void) 
{
    Comp_LP_RestoreConfig();
    
    if(Comp_LP_backup.enableState == 1u)
    {
        /* Enable Comp's operation */
        Comp_LP_Enable();

    } /* Do nothing if Comp was disabled before */ 
}


/* [] END OF FILE */
