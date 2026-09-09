/*******************************************************************************
* File Name: Comp_SUM.c
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

#include "Comp_SUM.h"

static Comp_SUM_backupStruct Comp_SUM_backup;


/*******************************************************************************
* Function Name: Comp_SUM_SaveConfig
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
void Comp_SUM_SaveConfig(void) 
{
    /* Empty since all are system reset for retention flops */
}


/*******************************************************************************
* Function Name: Comp_SUM_RestoreConfig
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
void Comp_SUM_RestoreConfig(void) 
{
    /* Empty since all are system reset for retention flops */    
}


/*******************************************************************************
* Function Name: Comp_SUM_Sleep
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
*  Comp_SUM_backup.enableState:  Is modified depending on the enable 
*   state of the block before entering sleep mode.
*
*******************************************************************************/
void Comp_SUM_Sleep(void) 
{
    /* Save Comp's enable state */    
    if(Comp_SUM_ACT_PWR_EN == (Comp_SUM_PWRMGR & Comp_SUM_ACT_PWR_EN))
    {
        /* Comp is enabled */
        Comp_SUM_backup.enableState = 1u;
    }
    else
    {
        /* Comp is disabled */
        Comp_SUM_backup.enableState = 0u;
    }    
    
    Comp_SUM_Stop();
    Comp_SUM_SaveConfig();
}


/*******************************************************************************
* Function Name: Comp_SUM_Wakeup
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
*  Comp_SUM_backup.enableState:  Is used to restore the enable state of 
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void Comp_SUM_Wakeup(void) 
{
    Comp_SUM_RestoreConfig();
    
    if(Comp_SUM_backup.enableState == 1u)
    {
        /* Enable Comp's operation */
        Comp_SUM_Enable();

    } /* Do nothing if Comp was disabled before */ 
}


/* [] END OF FILE */
