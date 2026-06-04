/*******************************************************************************
* File Name: OPAadder.c
* Version 1.90
*
* Description:
*  This file provides the source code to the API for OpAmp (Analog Buffer) 
*  Component.
*
* Note:
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#include "OPAadder.h"

uint8 OPAadder_initVar = 0u;


/*******************************************************************************   
* Function Name: OPAadder_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  OPAadder_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void OPAadder_Init(void) 
{
    OPAadder_SetPower(OPAadder_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: OPAadder_Enable
********************************************************************************
*
* Summary:
*  Enables the OpAmp block operation
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void OPAadder_Enable(void) 
{
    /* Enable negative charge pumps in ANIF */
    OPAadder_PUMP_CR1_REG  |= (OPAadder_PUMP_CR1_CLKSEL | OPAadder_PUMP_CR1_FORCE);

    /* Enable power to buffer in active mode */
    OPAadder_PM_ACT_CFG_REG |= OPAadder_ACT_PWR_EN;

    /* Enable power to buffer in alternative active mode */
    OPAadder_PM_STBY_CFG_REG |= OPAadder_STBY_PWR_EN;
}


/*******************************************************************************
* Function Name:   OPAadder_Start
********************************************************************************
*
* Summary:
*  The start function initializes the Analog Buffer with the default values and 
*  sets the power to the given level. A power level of 0, is same as 
*  executing the stop function.
*
* Parameters:
*  void
*
* Return:
*  void
*
* Global variables:
*  OPAadder_initVar: Used to check the initial configuration, modified 
*  when this function is called for the first time.
*
*******************************************************************************/
void OPAadder_Start(void) 
{
    if(OPAadder_initVar == 0u)
    {
        OPAadder_initVar = 1u;
        OPAadder_Init();
    }

    OPAadder_Enable();
}


/*******************************************************************************
* Function Name: OPAadder_Stop
********************************************************************************
*
* Summary:
*  Powers down amplifier to lowest power state.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void OPAadder_Stop(void) 
{
    /* Disable power to buffer in active mode template */
    OPAadder_PM_ACT_CFG_REG &= (uint8)(~OPAadder_ACT_PWR_EN);

    /* Disable power to buffer in alternative active mode template */
    OPAadder_PM_STBY_CFG_REG &= (uint8)(~OPAadder_STBY_PWR_EN);
    
    /* Disable negative charge pumps for ANIF only if all ABuf is turned OFF */
    if(OPAadder_PM_ACT_CFG_REG == 0u)
    {
        OPAadder_PUMP_CR1_REG &= (uint8)(~(OPAadder_PUMP_CR1_CLKSEL | OPAadder_PUMP_CR1_FORCE));
    }
}


/*******************************************************************************
* Function Name: OPAadder_SetPower
********************************************************************************
*
* Summary:
*  Sets power level of Analog buffer.
*
* Parameters: 
*  power: PSoC3: Sets power level between low (1) and high power (3).
*         PSoC5: Sets power level High (0)
*
* Return:
*  void
*
**********************************************************************************/
void OPAadder_SetPower(uint8 power) 
{
    #if (CY_PSOC3 || CY_PSOC5LP)
        OPAadder_CR_REG &= (uint8)(~OPAadder_PWR_MASK);
        OPAadder_CR_REG |= power & OPAadder_PWR_MASK;      /* Set device power */
    #else
        CYASSERT(OPAadder_HIGHPOWER == power);
    #endif /* CY_PSOC3 || CY_PSOC5LP */
}


/* [] END OF FILE */
