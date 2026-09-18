/*******************************************************************************
* File Name: cfg_PM.c
* Version 1.80
*
* Description:
*  This file contains the setup, control, and status commands to support 
*  the component operation in the low power mode. 
*
* Note:
*
********************************************************************************
* Copyright 2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#include "cfg.h"

/* Check for removal by optimization */
#if !defined(cfg_Sync_ctrl_reg__REMOVED)

static cfg_BACKUP_STRUCT  cfg_backup = {0u};

    
/*******************************************************************************
* Function Name: cfg_SaveConfig
********************************************************************************
*
* Summary:
*  Saves the control register value.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void cfg_SaveConfig(void) 
{
    cfg_backup.controlState = cfg_Control;
}


/*******************************************************************************
* Function Name: cfg_RestoreConfig
********************************************************************************
*
* Summary:
*  Restores the control register value.
*
* Parameters:
*  None
*
* Return:
*  None
*
*
*******************************************************************************/
void cfg_RestoreConfig(void) 
{
     cfg_Control = cfg_backup.controlState;
}


/*******************************************************************************
* Function Name: cfg_Sleep
********************************************************************************
*
* Summary:
*  Prepares the component for entering the low power mode.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void cfg_Sleep(void) 
{
    cfg_SaveConfig();
}


/*******************************************************************************
* Function Name: cfg_Wakeup
********************************************************************************
*
* Summary:
*  Restores the component after waking up from the low power mode.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void cfg_Wakeup(void)  
{
    cfg_RestoreConfig();
}

#endif /* End check for removal by optimization */


/* [] END OF FILE */
