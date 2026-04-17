/*******************************************************************************
* File Name: INA_Opamp.c
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

#include "INA_Opamp.h"

uint8 INA_Opamp_initVar = 0u;


/*******************************************************************************   
* Function Name: INA_Opamp_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  INA_Opamp_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void INA_Opamp_Init(void) 
{
    INA_Opamp_SetPower(INA_Opamp_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: INA_Opamp_Enable
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
void INA_Opamp_Enable(void) 
{
    /* Enable negative charge pumps in ANIF */
    INA_Opamp_PUMP_CR1_REG  |= (INA_Opamp_PUMP_CR1_CLKSEL | INA_Opamp_PUMP_CR1_FORCE);

    /* Enable power to buffer in active mode */
    INA_Opamp_PM_ACT_CFG_REG |= INA_Opamp_ACT_PWR_EN;

    /* Enable power to buffer in alternative active mode */
    INA_Opamp_PM_STBY_CFG_REG |= INA_Opamp_STBY_PWR_EN;
}


/*******************************************************************************
* Function Name:   INA_Opamp_Start
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
*  INA_Opamp_initVar: Used to check the initial configuration, modified 
*  when this function is called for the first time.
*
*******************************************************************************/
void INA_Opamp_Start(void) 
{
    if(INA_Opamp_initVar == 0u)
    {
        INA_Opamp_initVar = 1u;
        INA_Opamp_Init();
    }

    INA_Opamp_Enable();
}


/*******************************************************************************
* Function Name: INA_Opamp_Stop
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
void INA_Opamp_Stop(void) 
{
    /* Disable power to buffer in active mode template */
    INA_Opamp_PM_ACT_CFG_REG &= (uint8)(~INA_Opamp_ACT_PWR_EN);

    /* Disable power to buffer in alternative active mode template */
    INA_Opamp_PM_STBY_CFG_REG &= (uint8)(~INA_Opamp_STBY_PWR_EN);
    
    /* Disable negative charge pumps for ANIF only if all ABuf is turned OFF */
    if(INA_Opamp_PM_ACT_CFG_REG == 0u)
    {
        INA_Opamp_PUMP_CR1_REG &= (uint8)(~(INA_Opamp_PUMP_CR1_CLKSEL | INA_Opamp_PUMP_CR1_FORCE));
    }
}


/*******************************************************************************
* Function Name: INA_Opamp_SetPower
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
void INA_Opamp_SetPower(uint8 power) 
{
    #if (CY_PSOC3 || CY_PSOC5LP)
        INA_Opamp_CR_REG &= (uint8)(~INA_Opamp_PWR_MASK);
        INA_Opamp_CR_REG |= power & INA_Opamp_PWR_MASK;      /* Set device power */
    #else
        CYASSERT(INA_Opamp_HIGHPOWER == power);
    #endif /* CY_PSOC3 || CY_PSOC5LP */
}


/* [] END OF FILE */
