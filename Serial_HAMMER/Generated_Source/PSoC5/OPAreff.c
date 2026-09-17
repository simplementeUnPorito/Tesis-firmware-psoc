/*******************************************************************************
* File Name: OPAreff.c
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

#include "OPAreff.h"

uint8 OPAreff_initVar = 0u;


/*******************************************************************************   
* Function Name: OPAreff_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  OPAreff_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void OPAreff_Init(void) 
{
    OPAreff_SetPower(OPAreff_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: OPAreff_Enable
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
void OPAreff_Enable(void) 
{
    /* Enable negative charge pumps in ANIF */
    OPAreff_PUMP_CR1_REG  |= (OPAreff_PUMP_CR1_CLKSEL | OPAreff_PUMP_CR1_FORCE);

    /* Enable power to buffer in active mode */
    OPAreff_PM_ACT_CFG_REG |= OPAreff_ACT_PWR_EN;

    /* Enable power to buffer in alternative active mode */
    OPAreff_PM_STBY_CFG_REG |= OPAreff_STBY_PWR_EN;
}


/*******************************************************************************
* Function Name:   OPAreff_Start
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
*  OPAreff_initVar: Used to check the initial configuration, modified 
*  when this function is called for the first time.
*
*******************************************************************************/
void OPAreff_Start(void) 
{
    if(OPAreff_initVar == 0u)
    {
        OPAreff_initVar = 1u;
        OPAreff_Init();
    }

    OPAreff_Enable();
}


/*******************************************************************************
* Function Name: OPAreff_Stop
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
void OPAreff_Stop(void) 
{
    /* Disable power to buffer in active mode template */
    OPAreff_PM_ACT_CFG_REG &= (uint8)(~OPAreff_ACT_PWR_EN);

    /* Disable power to buffer in alternative active mode template */
    OPAreff_PM_STBY_CFG_REG &= (uint8)(~OPAreff_STBY_PWR_EN);
    
    /* Disable negative charge pumps for ANIF only if all ABuf is turned OFF */
    if(OPAreff_PM_ACT_CFG_REG == 0u)
    {
        OPAreff_PUMP_CR1_REG &= (uint8)(~(OPAreff_PUMP_CR1_CLKSEL | OPAreff_PUMP_CR1_FORCE));
    }
}


/*******************************************************************************
* Function Name: OPAreff_SetPower
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
void OPAreff_SetPower(uint8 power) 
{
    #if (CY_PSOC3 || CY_PSOC5LP)
        OPAreff_CR_REG &= (uint8)(~OPAreff_PWR_MASK);
        OPAreff_CR_REG |= power & OPAreff_PWR_MASK;      /* Set device power */
    #else
        CYASSERT(OPAreff_HIGHPOWER == power);
    #endif /* CY_PSOC3 || CY_PSOC5LP */
}


/* [] END OF FILE */
