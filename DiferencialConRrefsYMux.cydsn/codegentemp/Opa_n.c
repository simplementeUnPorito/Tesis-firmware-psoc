/*******************************************************************************
* File Name: Opa_n.c
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

#include "Opa_n.h"

uint8 Opa_n_initVar = 0u;


/*******************************************************************************   
* Function Name: Opa_n_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  Opa_n_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void Opa_n_Init(void) 
{
    Opa_n_SetPower(Opa_n_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: Opa_n_Enable
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
void Opa_n_Enable(void) 
{
    /* Enable negative charge pumps in ANIF */
    Opa_n_PUMP_CR1_REG  |= (Opa_n_PUMP_CR1_CLKSEL | Opa_n_PUMP_CR1_FORCE);

    /* Enable power to buffer in active mode */
    Opa_n_PM_ACT_CFG_REG |= Opa_n_ACT_PWR_EN;

    /* Enable power to buffer in alternative active mode */
    Opa_n_PM_STBY_CFG_REG |= Opa_n_STBY_PWR_EN;
}


/*******************************************************************************
* Function Name:   Opa_n_Start
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
*  Opa_n_initVar: Used to check the initial configuration, modified 
*  when this function is called for the first time.
*
*******************************************************************************/
void Opa_n_Start(void) 
{
    if(Opa_n_initVar == 0u)
    {
        Opa_n_initVar = 1u;
        Opa_n_Init();
    }

    Opa_n_Enable();
}


/*******************************************************************************
* Function Name: Opa_n_Stop
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
void Opa_n_Stop(void) 
{
    /* Disable power to buffer in active mode template */
    Opa_n_PM_ACT_CFG_REG &= (uint8)(~Opa_n_ACT_PWR_EN);

    /* Disable power to buffer in alternative active mode template */
    Opa_n_PM_STBY_CFG_REG &= (uint8)(~Opa_n_STBY_PWR_EN);
    
    /* Disable negative charge pumps for ANIF only if all ABuf is turned OFF */
    if(Opa_n_PM_ACT_CFG_REG == 0u)
    {
        Opa_n_PUMP_CR1_REG &= (uint8)(~(Opa_n_PUMP_CR1_CLKSEL | Opa_n_PUMP_CR1_FORCE));
    }
}


/*******************************************************************************
* Function Name: Opa_n_SetPower
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
void Opa_n_SetPower(uint8 power) 
{
    #if (CY_PSOC3 || CY_PSOC5LP)
        Opa_n_CR_REG &= (uint8)(~Opa_n_PWR_MASK);
        Opa_n_CR_REG |= power & Opa_n_PWR_MASK;      /* Set device power */
    #else
        CYASSERT(Opa_n_HIGHPOWER == power);
    #endif /* CY_PSOC3 || CY_PSOC5LP */
}


/* [] END OF FILE */
