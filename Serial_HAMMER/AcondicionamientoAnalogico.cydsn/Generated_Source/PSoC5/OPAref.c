/*******************************************************************************
* File Name: OPAref.c
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

#include "OPAref.h"

uint8 OPAref_initVar = 0u;


/*******************************************************************************   
* Function Name: OPAref_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  OPAref_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void OPAref_Init(void) 
{
    OPAref_SetPower(OPAref_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: OPAref_Enable
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
void OPAref_Enable(void) 
{
    /* Enable negative charge pumps in ANIF */
    OPAref_PUMP_CR1_REG  |= (OPAref_PUMP_CR1_CLKSEL | OPAref_PUMP_CR1_FORCE);

    /* Enable power to buffer in active mode */
    OPAref_PM_ACT_CFG_REG |= OPAref_ACT_PWR_EN;

    /* Enable power to buffer in alternative active mode */
    OPAref_PM_STBY_CFG_REG |= OPAref_STBY_PWR_EN;
}


/*******************************************************************************
* Function Name:   OPAref_Start
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
*  OPAref_initVar: Used to check the initial configuration, modified 
*  when this function is called for the first time.
*
*******************************************************************************/
void OPAref_Start(void) 
{
    if(OPAref_initVar == 0u)
    {
        OPAref_initVar = 1u;
        OPAref_Init();
    }

    OPAref_Enable();
}


/*******************************************************************************
* Function Name: OPAref_Stop
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
void OPAref_Stop(void) 
{
    /* Disable power to buffer in active mode template */
    OPAref_PM_ACT_CFG_REG &= (uint8)(~OPAref_ACT_PWR_EN);

    /* Disable power to buffer in alternative active mode template */
    OPAref_PM_STBY_CFG_REG &= (uint8)(~OPAref_STBY_PWR_EN);
    
    /* Disable negative charge pumps for ANIF only if all ABuf is turned OFF */
    if(OPAref_PM_ACT_CFG_REG == 0u)
    {
        OPAref_PUMP_CR1_REG &= (uint8)(~(OPAref_PUMP_CR1_CLKSEL | OPAref_PUMP_CR1_FORCE));
    }
}


/*******************************************************************************
* Function Name: OPAref_SetPower
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
void OPAref_SetPower(uint8 power) 
{
    #if (CY_PSOC3 || CY_PSOC5LP)
        OPAref_CR_REG &= (uint8)(~OPAref_PWR_MASK);
        OPAref_CR_REG |= power & OPAref_PWR_MASK;      /* Set device power */
    #else
        CYASSERT(OPAref_HIGHPOWER == power);
    #endif /* CY_PSOC3 || CY_PSOC5LP */
}


/* [] END OF FILE */
