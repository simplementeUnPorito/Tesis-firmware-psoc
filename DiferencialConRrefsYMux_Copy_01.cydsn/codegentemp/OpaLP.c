/*******************************************************************************
* File Name: OpaLP.c
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

#include "OpaLP.h"

uint8 OpaLP_initVar = 0u;


/*******************************************************************************   
* Function Name: OpaLP_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  OpaLP_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void OpaLP_Init(void) 
{
    OpaLP_SetPower(OpaLP_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: OpaLP_Enable
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
void OpaLP_Enable(void) 
{
    /* Enable negative charge pumps in ANIF */
    OpaLP_PUMP_CR1_REG  |= (OpaLP_PUMP_CR1_CLKSEL | OpaLP_PUMP_CR1_FORCE);

    /* Enable power to buffer in active mode */
    OpaLP_PM_ACT_CFG_REG |= OpaLP_ACT_PWR_EN;

    /* Enable power to buffer in alternative active mode */
    OpaLP_PM_STBY_CFG_REG |= OpaLP_STBY_PWR_EN;
}


/*******************************************************************************
* Function Name:   OpaLP_Start
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
*  OpaLP_initVar: Used to check the initial configuration, modified 
*  when this function is called for the first time.
*
*******************************************************************************/
void OpaLP_Start(void) 
{
    if(OpaLP_initVar == 0u)
    {
        OpaLP_initVar = 1u;
        OpaLP_Init();
    }

    OpaLP_Enable();
}


/*******************************************************************************
* Function Name: OpaLP_Stop
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
void OpaLP_Stop(void) 
{
    /* Disable power to buffer in active mode template */
    OpaLP_PM_ACT_CFG_REG &= (uint8)(~OpaLP_ACT_PWR_EN);

    /* Disable power to buffer in alternative active mode template */
    OpaLP_PM_STBY_CFG_REG &= (uint8)(~OpaLP_STBY_PWR_EN);
    
    /* Disable negative charge pumps for ANIF only if all ABuf is turned OFF */
    if(OpaLP_PM_ACT_CFG_REG == 0u)
    {
        OpaLP_PUMP_CR1_REG &= (uint8)(~(OpaLP_PUMP_CR1_CLKSEL | OpaLP_PUMP_CR1_FORCE));
    }
}


/*******************************************************************************
* Function Name: OpaLP_SetPower
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
void OpaLP_SetPower(uint8 power) 
{
    #if (CY_PSOC3 || CY_PSOC5LP)
        OpaLP_CR_REG &= (uint8)(~OpaLP_PWR_MASK);
        OpaLP_CR_REG |= power & OpaLP_PWR_MASK;      /* Set device power */
    #else
        CYASSERT(OpaLP_HIGHPOWER == power);
    #endif /* CY_PSOC3 || CY_PSOC5LP */
}


/* [] END OF FILE */
