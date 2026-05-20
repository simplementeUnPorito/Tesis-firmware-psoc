/*******************************************************************************
* File Name: Opa_ref.c
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

#include "Opa_ref.h"

uint8 Opa_ref_initVar = 0u;


/*******************************************************************************   
* Function Name: Opa_ref_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  Opa_ref_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void Opa_ref_Init(void) 
{
    Opa_ref_SetPower(Opa_ref_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: Opa_ref_Enable
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
void Opa_ref_Enable(void) 
{
    /* Enable negative charge pumps in ANIF */
    Opa_ref_PUMP_CR1_REG  |= (Opa_ref_PUMP_CR1_CLKSEL | Opa_ref_PUMP_CR1_FORCE);

    /* Enable power to buffer in active mode */
    Opa_ref_PM_ACT_CFG_REG |= Opa_ref_ACT_PWR_EN;

    /* Enable power to buffer in alternative active mode */
    Opa_ref_PM_STBY_CFG_REG |= Opa_ref_STBY_PWR_EN;
}


/*******************************************************************************
* Function Name:   Opa_ref_Start
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
*  Opa_ref_initVar: Used to check the initial configuration, modified 
*  when this function is called for the first time.
*
*******************************************************************************/
void Opa_ref_Start(void) 
{
    if(Opa_ref_initVar == 0u)
    {
        Opa_ref_initVar = 1u;
        Opa_ref_Init();
    }

    Opa_ref_Enable();
}


/*******************************************************************************
* Function Name: Opa_ref_Stop
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
void Opa_ref_Stop(void) 
{
    /* Disable power to buffer in active mode template */
    Opa_ref_PM_ACT_CFG_REG &= (uint8)(~Opa_ref_ACT_PWR_EN);

    /* Disable power to buffer in alternative active mode template */
    Opa_ref_PM_STBY_CFG_REG &= (uint8)(~Opa_ref_STBY_PWR_EN);
    
    /* Disable negative charge pumps for ANIF only if all ABuf is turned OFF */
    if(Opa_ref_PM_ACT_CFG_REG == 0u)
    {
        Opa_ref_PUMP_CR1_REG &= (uint8)(~(Opa_ref_PUMP_CR1_CLKSEL | Opa_ref_PUMP_CR1_FORCE));
    }
}


/*******************************************************************************
* Function Name: Opa_ref_SetPower
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
void Opa_ref_SetPower(uint8 power) 
{
    #if (CY_PSOC3 || CY_PSOC5LP)
        Opa_ref_CR_REG &= (uint8)(~Opa_ref_PWR_MASK);
        Opa_ref_CR_REG |= power & Opa_ref_PWR_MASK;      /* Set device power */
    #else
        CYASSERT(Opa_ref_HIGHPOWER == power);
    #endif /* CY_PSOC3 || CY_PSOC5LP */
}


/* [] END OF FILE */
