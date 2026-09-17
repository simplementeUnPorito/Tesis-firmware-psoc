/*******************************************************************************
* File Name: Reg_Select_PM.c
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

#include "Reg_Select.h"

/* Check for removal by optimization */
#if !defined(Reg_Select_Sync_ctrl_reg__REMOVED)

static Reg_Select_BACKUP_STRUCT  Reg_Select_backup = {0u};

    
/*******************************************************************************
* Function Name: Reg_Select_SaveConfig
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
void Reg_Select_SaveConfig(void) 
{
    Reg_Select_backup.controlState = Reg_Select_Control;
}


/*******************************************************************************
* Function Name: Reg_Select_RestoreConfig
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
void Reg_Select_RestoreConfig(void) 
{
     Reg_Select_Control = Reg_Select_backup.controlState;
}


/*******************************************************************************
* Function Name: Reg_Select_Sleep
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
void Reg_Select_Sleep(void) 
{
    Reg_Select_SaveConfig();
}


/*******************************************************************************
* Function Name: Reg_Select_Wakeup
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
void Reg_Select_Wakeup(void)  
{
    Reg_Select_RestoreConfig();
}

#endif /* End check for removal by optimization */


/* [] END OF FILE */
