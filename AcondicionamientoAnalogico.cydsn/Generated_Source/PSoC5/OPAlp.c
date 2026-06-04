/*******************************************************************************
* File Name: OPAlp.c
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

#include "OPAlp.h"

uint8 OPAlp_initVar = 0u;


/*******************************************************************************   
* Function Name: OPAlp_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  OPAlp_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void OPAlp_Init(void) 
{
    OPAlp_SetPower(OPAlp_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: OPAlp_Enable
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
void OPAlp_Enable(void) 
{
    /* Enable negative charge pumps in ANIF */
    OPAlp_PUMP_CR1_REG  |= (OPAlp_PUMP_CR1_CLKSEL | OPAlp_PUMP_CR1_FORCE);

    /* Enable power to buffer in active mode */
    OPAlp_PM_ACT_CFG_REG |= OPAlp_ACT_PWR_EN;

    /* Enable power to buffer in alternative active mode */
    OPAlp_PM_STBY_CFG_REG |= OPAlp_STBY_PWR_EN;
}


/*******************************************************************************
* Function Name:   OPAlp_Start
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
*  OPAlp_initVar: Used to check the initial configuration, modified 
*  when this function is called for the first time.
*
*******************************************************************************/
void OPAlp_Start(void) 
{
    if(OPAlp_initVar == 0u)
    {
        OPAlp_initVar = 1u;
        OPAlp_Init();
    }

    OPAlp_Enable();
}


/*******************************************************************************
* Function Name: OPAlp_Stop
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
void OPAlp_Stop(void) 
{
    /* Disable power to buffer in active mode template */
    OPAlp_PM_ACT_CFG_REG &= (uint8)(~OPAlp_ACT_PWR_EN);

    /* Disable power to buffer in alternative active mode template */
    OPAlp_PM_STBY_CFG_REG &= (uint8)(~OPAlp_STBY_PWR_EN);
    
    /* Disable negative charge pumps for ANIF only if all ABuf is turned OFF */
    if(OPAlp_PM_ACT_CFG_REG == 0u)
    {
        OPAlp_PUMP_CR1_REG &= (uint8)(~(OPAlp_PUMP_CR1_CLKSEL | OPAlp_PUMP_CR1_FORCE));
    }
}


/*******************************************************************************
* Function Name: OPAlp_SetPower
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
void OPAlp_SetPower(uint8 power) 
{
    #if (CY_PSOC3 || CY_PSOC5LP)
        OPAlp_CR_REG &= (uint8)(~OPAlp_PWR_MASK);
        OPAlp_CR_REG |= power & OPAlp_PWR_MASK;      /* Set device power */
    #else
        CYASSERT(OPAlp_HIGHPOWER == power);
    #endif /* CY_PSOC3 || CY_PSOC5LP */
}


/* [] END OF FILE */
