/*******************************************************************************
* File Name: Tmr_CaptureWatchdog_PM.c
* Version 2.80
*
*  Description:
*     This file provides the power management source code to API for the
*     Timer.
*
*   Note:
*     None
*
*******************************************************************************
* Copyright 2008-2017, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
********************************************************************************/

#include "Tmr_CaptureWatchdog.h"

static Tmr_CaptureWatchdog_backupStruct Tmr_CaptureWatchdog_backup;


/*******************************************************************************
* Function Name: Tmr_CaptureWatchdog_SaveConfig
********************************************************************************
*
* Summary:
*     Save the current user configuration
*
* Parameters:
*  void
*
* Return:
*  void
*
* Global variables:
*  Tmr_CaptureWatchdog_backup:  Variables of this global structure are modified to
*  store the values of non retention configuration registers when Sleep() API is
*  called.
*
*******************************************************************************/
void Tmr_CaptureWatchdog_SaveConfig(void) 
{
    #if (!Tmr_CaptureWatchdog_UsingFixedFunction)
        Tmr_CaptureWatchdog_backup.TimerUdb = Tmr_CaptureWatchdog_ReadCounter();
        Tmr_CaptureWatchdog_backup.InterruptMaskValue = Tmr_CaptureWatchdog_STATUS_MASK;
        #if (Tmr_CaptureWatchdog_UsingHWCaptureCounter)
            Tmr_CaptureWatchdog_backup.TimerCaptureCounter = Tmr_CaptureWatchdog_ReadCaptureCount();
        #endif /* Back Up capture counter register  */

        #if(!Tmr_CaptureWatchdog_UDB_CONTROL_REG_REMOVED)
            Tmr_CaptureWatchdog_backup.TimerControlRegister = Tmr_CaptureWatchdog_ReadControlRegister();
        #endif /* Backup the enable state of the Timer component */
    #endif /* Backup non retention registers in UDB implementation. All fixed function registers are retention */
}


/*******************************************************************************
* Function Name: Tmr_CaptureWatchdog_RestoreConfig
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
* Global variables:
*  Tmr_CaptureWatchdog_backup:  Variables of this global structure are used to
*  restore the values of non retention registers on wakeup from sleep mode.
*
*******************************************************************************/
void Tmr_CaptureWatchdog_RestoreConfig(void) 
{   
    #if (!Tmr_CaptureWatchdog_UsingFixedFunction)

        Tmr_CaptureWatchdog_WriteCounter(Tmr_CaptureWatchdog_backup.TimerUdb);
        Tmr_CaptureWatchdog_STATUS_MASK =Tmr_CaptureWatchdog_backup.InterruptMaskValue;
        #if (Tmr_CaptureWatchdog_UsingHWCaptureCounter)
            Tmr_CaptureWatchdog_SetCaptureCount(Tmr_CaptureWatchdog_backup.TimerCaptureCounter);
        #endif /* Restore Capture counter register*/

        #if(!Tmr_CaptureWatchdog_UDB_CONTROL_REG_REMOVED)
            Tmr_CaptureWatchdog_WriteControlRegister(Tmr_CaptureWatchdog_backup.TimerControlRegister);
        #endif /* Restore the enable state of the Timer component */
    #endif /* Restore non retention registers in the UDB implementation only */
}


/*******************************************************************************
* Function Name: Tmr_CaptureWatchdog_Sleep
********************************************************************************
*
* Summary:
*     Stop and Save the user configuration
*
* Parameters:
*  void
*
* Return:
*  void
*
* Global variables:
*  Tmr_CaptureWatchdog_backup.TimerEnableState:  Is modified depending on the
*  enable state of the block before entering sleep mode.
*
*******************************************************************************/
void Tmr_CaptureWatchdog_Sleep(void) 
{
    #if(!Tmr_CaptureWatchdog_UDB_CONTROL_REG_REMOVED)
        /* Save Counter's enable state */
        if(Tmr_CaptureWatchdog_CTRL_ENABLE == (Tmr_CaptureWatchdog_CONTROL & Tmr_CaptureWatchdog_CTRL_ENABLE))
        {
            /* Timer is enabled */
            Tmr_CaptureWatchdog_backup.TimerEnableState = 1u;
        }
        else
        {
            /* Timer is disabled */
            Tmr_CaptureWatchdog_backup.TimerEnableState = 0u;
        }
    #endif /* Back up enable state from the Timer control register */
    Tmr_CaptureWatchdog_Stop();
    Tmr_CaptureWatchdog_SaveConfig();
}


/*******************************************************************************
* Function Name: Tmr_CaptureWatchdog_Wakeup
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
*  Tmr_CaptureWatchdog_backup.enableState:  Is used to restore the enable state of
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void Tmr_CaptureWatchdog_Wakeup(void) 
{
    Tmr_CaptureWatchdog_RestoreConfig();
    #if(!Tmr_CaptureWatchdog_UDB_CONTROL_REG_REMOVED)
        if(Tmr_CaptureWatchdog_backup.TimerEnableState == 1u)
        {     /* Enable Timer's operation */
                Tmr_CaptureWatchdog_Enable();
        } /* Do nothing if Timer was disabled before */
    #endif /* Remove this code section if Control register is removed */
}


/* [] END OF FILE */
