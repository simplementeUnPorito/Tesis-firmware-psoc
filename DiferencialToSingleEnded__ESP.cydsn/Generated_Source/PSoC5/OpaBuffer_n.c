/*******************************************************************************
* File Name: OpaBuffer_n.c
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

#include "OpaBuffer_n.h"

uint8 OpaBuffer_n_initVar = 0u;


/*******************************************************************************   
* Function Name: OpaBuffer_n_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  OpaBuffer_n_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void OpaBuffer_n_Init(void) 
{
    OpaBuffer_n_SetPower(OpaBuffer_n_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: OpaBuffer_n_Enable
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
void OpaBuffer_n_Enable(void) 
{
    /* Enable negative charge pumps in ANIF */
    OpaBuffer_n_PUMP_CR1_REG  |= (OpaBuffer_n_PUMP_CR1_CLKSEL | OpaBuffer_n_PUMP_CR1_FORCE);

    /* Enable power to buffer in active mode */
    OpaBuffer_n_PM_ACT_CFG_REG |= OpaBuffer_n_ACT_PWR_EN;

    /* Enable power to buffer in alternative active mode */
    OpaBuffer_n_PM_STBY_CFG_REG |= OpaBuffer_n_STBY_PWR_EN;
}


/*******************************************************************************
* Function Name:   OpaBuffer_n_Start
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
*  OpaBuffer_n_initVar: Used to check the initial configuration, modified 
*  when this function is called for the first time.
*
*******************************************************************************/
void OpaBuffer_n_Start(void) 
{
    if(OpaBuffer_n_initVar == 0u)
    {
        OpaBuffer_n_initVar = 1u;
        OpaBuffer_n_Init();
    }

    OpaBuffer_n_Enable();
}


/*******************************************************************************
* Function Name: OpaBuffer_n_Stop
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
void OpaBuffer_n_Stop(void) 
{
    /* Disable power to buffer in active mode template */
    OpaBuffer_n_PM_ACT_CFG_REG &= (uint8)(~OpaBuffer_n_ACT_PWR_EN);

    /* Disable power to buffer in alternative active mode template */
    OpaBuffer_n_PM_STBY_CFG_REG &= (uint8)(~OpaBuffer_n_STBY_PWR_EN);
    
    /* Disable negative charge pumps for ANIF only if all ABuf is turned OFF */
    if(OpaBuffer_n_PM_ACT_CFG_REG == 0u)
    {
        OpaBuffer_n_PUMP_CR1_REG &= (uint8)(~(OpaBuffer_n_PUMP_CR1_CLKSEL | OpaBuffer_n_PUMP_CR1_FORCE));
    }
}


/*******************************************************************************
* Function Name: OpaBuffer_n_SetPower
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
void OpaBuffer_n_SetPower(uint8 power) 
{
    #if (CY_PSOC3 || CY_PSOC5LP)
        OpaBuffer_n_CR_REG &= (uint8)(~OpaBuffer_n_PWR_MASK);
        OpaBuffer_n_CR_REG |= power & OpaBuffer_n_PWR_MASK;      /* Set device power */
    #else
        CYASSERT(OpaBuffer_n_HIGHPOWER == power);
    #endif /* CY_PSOC3 || CY_PSOC5LP */
}


/* [] END OF FILE */
