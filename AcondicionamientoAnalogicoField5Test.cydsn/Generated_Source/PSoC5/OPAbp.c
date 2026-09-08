/*******************************************************************************
* File Name: OPAbp.c
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

#include "OPAbp.h"

uint8 OPAbp_initVar = 0u;


/*******************************************************************************   
* Function Name: OPAbp_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  OPAbp_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void OPAbp_Init(void) 
{
    OPAbp_SetPower(OPAbp_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: OPAbp_Enable
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
void OPAbp_Enable(void) 
{
    /* Enable negative charge pumps in ANIF */
    OPAbp_PUMP_CR1_REG  |= (OPAbp_PUMP_CR1_CLKSEL | OPAbp_PUMP_CR1_FORCE);

    /* Enable power to buffer in active mode */
    OPAbp_PM_ACT_CFG_REG |= OPAbp_ACT_PWR_EN;

    /* Enable power to buffer in alternative active mode */
    OPAbp_PM_STBY_CFG_REG |= OPAbp_STBY_PWR_EN;
}


/*******************************************************************************
* Function Name:   OPAbp_Start
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
*  OPAbp_initVar: Used to check the initial configuration, modified 
*  when this function is called for the first time.
*
*******************************************************************************/
void OPAbp_Start(void) 
{
    if(OPAbp_initVar == 0u)
    {
        OPAbp_initVar = 1u;
        OPAbp_Init();
    }

    OPAbp_Enable();
}


/*******************************************************************************
* Function Name: OPAbp_Stop
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
void OPAbp_Stop(void) 
{
    /* Disable power to buffer in active mode template */
    OPAbp_PM_ACT_CFG_REG &= (uint8)(~OPAbp_ACT_PWR_EN);

    /* Disable power to buffer in alternative active mode template */
    OPAbp_PM_STBY_CFG_REG &= (uint8)(~OPAbp_STBY_PWR_EN);
    
    /* Disable negative charge pumps for ANIF only if all ABuf is turned OFF */
    if(OPAbp_PM_ACT_CFG_REG == 0u)
    {
        OPAbp_PUMP_CR1_REG &= (uint8)(~(OPAbp_PUMP_CR1_CLKSEL | OPAbp_PUMP_CR1_FORCE));
    }
}


/*******************************************************************************
* Function Name: OPAbp_SetPower
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
void OPAbp_SetPower(uint8 power) 
{
    #if (CY_PSOC3 || CY_PSOC5LP)
        OPAbp_CR_REG &= (uint8)(~OPAbp_PWR_MASK);
        OPAbp_CR_REG |= power & OPAbp_PWR_MASK;      /* Set device power */
    #else
        CYASSERT(OPAbp_HIGHPOWER == power);
    #endif /* CY_PSOC3 || CY_PSOC5LP */
}


/* [] END OF FILE */
