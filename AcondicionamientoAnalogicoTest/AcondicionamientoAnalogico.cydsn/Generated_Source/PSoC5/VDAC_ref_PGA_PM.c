/*******************************************************************************
* File Name: VDAC_ref_PGA.c
* Version 2.0
*
* Description:
*  This file provides the power management source code to API for the
*  IDAC8.
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


#include "VDAC_ref_PGA.h"

static VDAC_ref_PGA_backupStruct VDAC_ref_PGA_backup;


/*******************************************************************************
* Function Name: VDAC_ref_PGA_SaveConfig
********************************************************************************
* Summary:
*  Save the current user configuration
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void VDAC_ref_PGA_SaveConfig(void) 
{
    if (!((VDAC_ref_PGA_CR1 & VDAC_ref_PGA_SRC_MASK) == VDAC_ref_PGA_SRC_UDB))
    {
        VDAC_ref_PGA_backup.data_value = VDAC_ref_PGA_Data;
    }
}


/*******************************************************************************
* Function Name: VDAC_ref_PGA_RestoreConfig
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
*******************************************************************************/
void VDAC_ref_PGA_RestoreConfig(void) 
{
    if (!((VDAC_ref_PGA_CR1 & VDAC_ref_PGA_SRC_MASK) == VDAC_ref_PGA_SRC_UDB))
    {
        if((VDAC_ref_PGA_Strobe & VDAC_ref_PGA_STRB_MASK) == VDAC_ref_PGA_STRB_EN)
        {
            VDAC_ref_PGA_Strobe &= (uint8)(~VDAC_ref_PGA_STRB_MASK);
            VDAC_ref_PGA_Data = VDAC_ref_PGA_backup.data_value;
            VDAC_ref_PGA_Strobe |= VDAC_ref_PGA_STRB_EN;
        }
        else
        {
            VDAC_ref_PGA_Data = VDAC_ref_PGA_backup.data_value;
        }
    }
}


/*******************************************************************************
* Function Name: VDAC_ref_PGA_Sleep
********************************************************************************
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
*  VDAC_ref_PGA_backup.enableState: Is modified depending on the enable 
*  state of the block before entering sleep mode.
*
*******************************************************************************/
void VDAC_ref_PGA_Sleep(void) 
{
    if(VDAC_ref_PGA_ACT_PWR_EN == (VDAC_ref_PGA_PWRMGR & VDAC_ref_PGA_ACT_PWR_EN))
    {
        /* IDAC8 is enabled */
        VDAC_ref_PGA_backup.enableState = 1u;
    }
    else
    {
        /* IDAC8 is disabled */
        VDAC_ref_PGA_backup.enableState = 0u;
    }

    VDAC_ref_PGA_Stop();
    VDAC_ref_PGA_SaveConfig();
}


/*******************************************************************************
* Function Name: VDAC_ref_PGA_Wakeup
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
*  VDAC_ref_PGA_backup.enableState: Is used to restore the enable state of 
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void VDAC_ref_PGA_Wakeup(void) 
{
    VDAC_ref_PGA_RestoreConfig();
    
    if(VDAC_ref_PGA_backup.enableState == 1u)
    {
        /* Enable IDAC8's operation */
        VDAC_ref_PGA_Enable();
        
        /* Set the data register */
        VDAC_ref_PGA_SetValue(VDAC_ref_PGA_Data);
    } /* Do nothing if IDAC8 was disabled before */    
}


/* [] END OF FILE */
