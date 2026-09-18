/*******************************************************************************
* File Name: OPAsum.c
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

#include "OPAsum.h"

uint8 OPAsum_initVar = 0u;


/*******************************************************************************   
* Function Name: OPAsum_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  OPAsum_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void OPAsum_Init(void) 
{
    OPAsum_SetPower(OPAsum_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: OPAsum_Enable
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
void OPAsum_Enable(void) 
{
    /* Enable negative charge pumps in ANIF */
    OPAsum_PUMP_CR1_REG  |= (OPAsum_PUMP_CR1_CLKSEL | OPAsum_PUMP_CR1_FORCE);

    /* Enable power to buffer in active mode */
    OPAsum_PM_ACT_CFG_REG |= OPAsum_ACT_PWR_EN;

    /* Enable power to buffer in alternative active mode */
    OPAsum_PM_STBY_CFG_REG |= OPAsum_STBY_PWR_EN;
}


/*******************************************************************************
* Function Name:   OPAsum_Start
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
*  OPAsum_initVar: Used to check the initial configuration, modified 
*  when this function is called for the first time.
*
*******************************************************************************/
void OPAsum_Start(void) 
{
    if(OPAsum_initVar == 0u)
    {
        OPAsum_initVar = 1u;
        OPAsum_Init();
    }

    OPAsum_Enable();
}


/*******************************************************************************
* Function Name: OPAsum_Stop
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
void OPAsum_Stop(void) 
{
    /* Disable power to buffer in active mode template */
    OPAsum_PM_ACT_CFG_REG &= (uint8)(~OPAsum_ACT_PWR_EN);

    /* Disable power to buffer in alternative active mode template */
    OPAsum_PM_STBY_CFG_REG &= (uint8)(~OPAsum_STBY_PWR_EN);
    
    /* Disable negative charge pumps for ANIF only if all ABuf is turned OFF */
    if(OPAsum_PM_ACT_CFG_REG == 0u)
    {
        OPAsum_PUMP_CR1_REG &= (uint8)(~(OPAsum_PUMP_CR1_CLKSEL | OPAsum_PUMP_CR1_FORCE));
    }
}


/*******************************************************************************
* Function Name: OPAsum_SetPower
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
void OPAsum_SetPower(uint8 power) 
{
    #if (CY_PSOC3 || CY_PSOC5LP)
        OPAsum_CR_REG &= (uint8)(~OPAsum_PWR_MASK);
        OPAsum_CR_REG |= power & OPAsum_PWR_MASK;      /* Set device power */
    #else
        CYASSERT(OPAsum_HIGHPOWER == power);
    #endif /* CY_PSOC3 || CY_PSOC5LP */
}


/* [] END OF FILE */
