/*******************************************************************************
* File Name: Reg_Selec_PM.c
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

#include "Reg_Selec.h"

/* Check for removal by optimization */
#if !defined(Reg_Selec_Sync_ctrl_reg__REMOVED)

static Reg_Selec_BACKUP_STRUCT  Reg_Selec_backup = {0u};

    
/*******************************************************************************
* Function Name: Reg_Selec_SaveConfig
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
void Reg_Selec_SaveConfig(void) 
{
    Reg_Selec_backup.controlState = Reg_Selec_Control;
}


/*******************************************************************************
* Function Name: Reg_Selec_RestoreConfig
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
void Reg_Selec_RestoreConfig(void) 
{
     Reg_Selec_Control = Reg_Selec_backup.controlState;
}


/*******************************************************************************
* Function Name: Reg_Selec_Sleep
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
void Reg_Selec_Sleep(void) 
{
    Reg_Selec_SaveConfig();
}


/*******************************************************************************
* Function Name: Reg_Selec_Wakeup
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
void Reg_Selec_Wakeup(void)  
{
    Reg_Selec_RestoreConfig();
}

#endif /* End check for removal by optimization */


/* [] END OF FILE */
