/*******************************************************************************
* File Name: Timer_VDAC_PM.c
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

#include "Timer_VDAC.h"

static Timer_VDAC_backupStruct Timer_VDAC_backup;


/*******************************************************************************
* Function Name: Timer_VDAC_SaveConfig
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
*  Timer_VDAC_backup:  Variables of this global structure are modified to
*  store the values of non retention configuration registers when Sleep() API is
*  called.
*
*******************************************************************************/
void Timer_VDAC_SaveConfig(void) 
{
    #if (!Timer_VDAC_UsingFixedFunction)
        Timer_VDAC_backup.TimerUdb = Timer_VDAC_ReadCounter();
        Timer_VDAC_backup.InterruptMaskValue = Timer_VDAC_STATUS_MASK;
        #if (Timer_VDAC_UsingHWCaptureCounter)
            Timer_VDAC_backup.TimerCaptureCounter = Timer_VDAC_ReadCaptureCount();
        #endif /* Back Up capture counter register  */

        #if(!Timer_VDAC_UDB_CONTROL_REG_REMOVED)
            Timer_VDAC_backup.TimerControlRegister = Timer_VDAC_ReadControlRegister();
        #endif /* Backup the enable state of the Timer component */
    #endif /* Backup non retention registers in UDB implementation. All fixed function registers are retention */
}


/*******************************************************************************
* Function Name: Timer_VDAC_RestoreConfig
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
*  Timer_VDAC_backup:  Variables of this global structure are used to
*  restore the values of non retention registers on wakeup from sleep mode.
*
*******************************************************************************/
void Timer_VDAC_RestoreConfig(void) 
{   
    #if (!Timer_VDAC_UsingFixedFunction)

        Timer_VDAC_WriteCounter(Timer_VDAC_backup.TimerUdb);
        Timer_VDAC_STATUS_MASK =Timer_VDAC_backup.InterruptMaskValue;
        #if (Timer_VDAC_UsingHWCaptureCounter)
            Timer_VDAC_SetCaptureCount(Timer_VDAC_backup.TimerCaptureCounter);
        #endif /* Restore Capture counter register*/

        #if(!Timer_VDAC_UDB_CONTROL_REG_REMOVED)
            Timer_VDAC_WriteControlRegister(Timer_VDAC_backup.TimerControlRegister);
        #endif /* Restore the enable state of the Timer component */
    #endif /* Restore non retention registers in the UDB implementation only */
}


/*******************************************************************************
* Function Name: Timer_VDAC_Sleep
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
*  Timer_VDAC_backup.TimerEnableState:  Is modified depending on the
*  enable state of the block before entering sleep mode.
*
*******************************************************************************/
void Timer_VDAC_Sleep(void) 
{
    #if(!Timer_VDAC_UDB_CONTROL_REG_REMOVED)
        /* Save Counter's enable state */
        if(Timer_VDAC_CTRL_ENABLE == (Timer_VDAC_CONTROL & Timer_VDAC_CTRL_ENABLE))
        {
            /* Timer is enabled */
            Timer_VDAC_backup.TimerEnableState = 1u;
        }
        else
        {
            /* Timer is disabled */
            Timer_VDAC_backup.TimerEnableState = 0u;
        }
    #endif /* Back up enable state from the Timer control register */
    Timer_VDAC_Stop();
    Timer_VDAC_SaveConfig();
}


/*******************************************************************************
* Function Name: Timer_VDAC_Wakeup
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
*  Timer_VDAC_backup.enableState:  Is used to restore the enable state of
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void Timer_VDAC_Wakeup(void) 
{
    Timer_VDAC_RestoreConfig();
    #if(!Timer_VDAC_UDB_CONTROL_REG_REMOVED)
        if(Timer_VDAC_backup.TimerEnableState == 1u)
        {     /* Enable Timer's operation */
                Timer_VDAC_Enable();
        } /* Do nothing if Timer was disabled before */
    #endif /* Remove this code section if Control register is removed */
}


/* [] END OF FILE */
