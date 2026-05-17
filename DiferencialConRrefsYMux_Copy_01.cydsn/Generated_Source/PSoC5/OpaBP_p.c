/*******************************************************************************
* File Name: OpaBP_p.c
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

#include "OpaBP_p.h"

uint8 OpaBP_p_initVar = 0u;


/*******************************************************************************   
* Function Name: OpaBP_p_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  OpaBP_p_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void OpaBP_p_Init(void) 
{
    OpaBP_p_SetPower(OpaBP_p_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: OpaBP_p_Enable
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
void OpaBP_p_Enable(void) 
{
    /* Enable negative charge pumps in ANIF */
    OpaBP_p_PUMP_CR1_REG  |= (OpaBP_p_PUMP_CR1_CLKSEL | OpaBP_p_PUMP_CR1_FORCE);

    /* Enable power to buffer in active mode */
    OpaBP_p_PM_ACT_CFG_REG |= OpaBP_p_ACT_PWR_EN;

    /* Enable power to buffer in alternative active mode */
    OpaBP_p_PM_STBY_CFG_REG |= OpaBP_p_STBY_PWR_EN;
}


/*******************************************************************************
* Function Name:   OpaBP_p_Start
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
*  OpaBP_p_initVar: Used to check the initial configuration, modified 
*  when this function is called for the first time.
*
*******************************************************************************/
void OpaBP_p_Start(void) 
{
    if(OpaBP_p_initVar == 0u)
    {
        OpaBP_p_initVar = 1u;
        OpaBP_p_Init();
    }

    OpaBP_p_Enable();
}


/*******************************************************************************
* Function Name: OpaBP_p_Stop
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
void OpaBP_p_Stop(void) 
{
    /* Disable power to buffer in active mode template */
    OpaBP_p_PM_ACT_CFG_REG &= (uint8)(~OpaBP_p_ACT_PWR_EN);

    /* Disable power to buffer in alternative active mode template */
    OpaBP_p_PM_STBY_CFG_REG &= (uint8)(~OpaBP_p_STBY_PWR_EN);
    
    /* Disable negative charge pumps for ANIF only if all ABuf is turned OFF */
    if(OpaBP_p_PM_ACT_CFG_REG == 0u)
    {
        OpaBP_p_PUMP_CR1_REG &= (uint8)(~(OpaBP_p_PUMP_CR1_CLKSEL | OpaBP_p_PUMP_CR1_FORCE));
    }
}


/*******************************************************************************
* Function Name: OpaBP_p_SetPower
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
void OpaBP_p_SetPower(uint8 power) 
{
    #if (CY_PSOC3 || CY_PSOC5LP)
        OpaBP_p_CR_REG &= (uint8)(~OpaBP_p_PWR_MASK);
        OpaBP_p_CR_REG |= power & OpaBP_p_PWR_MASK;      /* Set device power */
    #else
        CYASSERT(OpaBP_p_HIGHPOWER == power);
    #endif /* CY_PSOC3 || CY_PSOC5LP */
}


/* [] END OF FILE */
