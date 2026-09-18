/*******************************************************************************
* File Name: Tmr_PingCalTick_PM.c
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

#include "Tmr_PingCalTick.h"

static Tmr_PingCalTick_backupStruct Tmr_PingCalTick_backup;


/*******************************************************************************
* Function Name: Tmr_PingCalTick_SaveConfig
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
*  Tmr_PingCalTick_backup:  Variables of this global structure are modified to
*  store the values of non retention configuration registers when Sleep() API is
*  called.
*
*******************************************************************************/
void Tmr_PingCalTick_SaveConfig(void) 
{
    #if (!Tmr_PingCalTick_UsingFixedFunction)
        Tmr_PingCalTick_backup.TimerUdb = Tmr_PingCalTick_ReadCounter();
        Tmr_PingCalTick_backup.InterruptMaskValue = Tmr_PingCalTick_STATUS_MASK;
        #if (Tmr_PingCalTick_UsingHWCaptureCounter)
            Tmr_PingCalTick_backup.TimerCaptureCounter = Tmr_PingCalTick_ReadCaptureCount();
        #endif /* Back Up capture counter register  */

        #if(!Tmr_PingCalTick_UDB_CONTROL_REG_REMOVED)
            Tmr_PingCalTick_backup.TimerControlRegister = Tmr_PingCalTick_ReadControlRegister();
        #endif /* Backup the enable state of the Timer component */
    #endif /* Backup non retention registers in UDB implementation. All fixed function registers are retention */
}


/*******************************************************************************
* Function Name: Tmr_PingCalTick_RestoreConfig
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
*  Tmr_PingCalTick_backup:  Variables of this global structure are used to
*  restore the values of non retention registers on wakeup from sleep mode.
*
*******************************************************************************/
void Tmr_PingCalTick_RestoreConfig(void) 
{   
    #if (!Tmr_PingCalTick_UsingFixedFunction)

        Tmr_PingCalTick_WriteCounter(Tmr_PingCalTick_backup.TimerUdb);
        Tmr_PingCalTick_STATUS_MASK =Tmr_PingCalTick_backup.InterruptMaskValue;
        #if (Tmr_PingCalTick_UsingHWCaptureCounter)
            Tmr_PingCalTick_SetCaptureCount(Tmr_PingCalTick_backup.TimerCaptureCounter);
        #endif /* Restore Capture counter register*/

        #if(!Tmr_PingCalTick_UDB_CONTROL_REG_REMOVED)
            Tmr_PingCalTick_WriteControlRegister(Tmr_PingCalTick_backup.TimerControlRegister);
        #endif /* Restore the enable state of the Timer component */
    #endif /* Restore non retention registers in the UDB implementation only */
}


/*******************************************************************************
* Function Name: Tmr_PingCalTick_Sleep
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
*  Tmr_PingCalTick_backup.TimerEnableState:  Is modified depending on the
*  enable state of the block before entering sleep mode.
*
*******************************************************************************/
void Tmr_PingCalTick_Sleep(void) 
{
    #if(!Tmr_PingCalTick_UDB_CONTROL_REG_REMOVED)
        /* Save Counter's enable state */
        if(Tmr_PingCalTick_CTRL_ENABLE == (Tmr_PingCalTick_CONTROL & Tmr_PingCalTick_CTRL_ENABLE))
        {
            /* Timer is enabled */
            Tmr_PingCalTick_backup.TimerEnableState = 1u;
        }
        else
        {
            /* Timer is disabled */
            Tmr_PingCalTick_backup.TimerEnableState = 0u;
        }
    #endif /* Back up enable state from the Timer control register */
    Tmr_PingCalTick_Stop();
    Tmr_PingCalTick_SaveConfig();
}


/*******************************************************************************
* Function Name: Tmr_PingCalTick_Wakeup
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
*  Tmr_PingCalTick_backup.enableState:  Is used to restore the enable state of
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void Tmr_PingCalTick_Wakeup(void) 
{
    Tmr_PingCalTick_RestoreConfig();
    #if(!Tmr_PingCalTick_UDB_CONTROL_REG_REMOVED)
        if(Tmr_PingCalTick_backup.TimerEnableState == 1u)
        {     /* Enable Timer's operation */
                Tmr_PingCalTick_Enable();
        } /* Do nothing if Timer was disabled before */
    #endif /* Remove this code section if Control register is removed */
}


/* [] END OF FILE */
