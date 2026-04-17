/*******************************************************************************
* File Name: INA_PGA_m.c  
* Version 2.0
*
* Description:
*  This file provides the source code to the API for the PGA 
*  User Module.
*
* Note:
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#include "INA_PGA_m.h"

#if (!CY_PSOC5A)
    #if (CYDEV_VARIABLE_VDDA == 1u)
        #include "CyScBoostClk.h"
    #endif /* (CYDEV_VARIABLE_VDDA == 1u) */
#endif /* (!CY_PSOC5A) */

#if (CY_PSOC5A)
    static INA_PGA_m_BACKUP_STRUCT  INA_PGA_m_P5backup;
#endif /* (CY_ PSOC5A) */

uint8 INA_PGA_m_initVar = 0u;


/*******************************************************************************   
* Function Name: INA_PGA_m_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  INA_PGA_m_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void INA_PGA_m_Init(void) 
{
    /* Set PGA mode */
    INA_PGA_m_CR0_REG = INA_PGA_m_MODE_PGA;      
    /* Set non-inverting PGA mode and reference mode */
    INA_PGA_m_CR1_REG |= INA_PGA_m_PGA_NINV;  
    /* Set default gain and ref mode */
    INA_PGA_m_CR2_REG = INA_PGA_m_VREF_MODE;
    /* Set gain and compensation */
    INA_PGA_m_SetGain(INA_PGA_m_DEFAULT_GAIN);
    /* Set power */
    INA_PGA_m_SetPower(INA_PGA_m_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: INA_PGA_m_Enable
********************************************************************************
*
* Summary:
*  Enables the PGA block operation.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void INA_PGA_m_Enable(void) 
{
    /* This is to restore the value of register CR1 and CR2 which is saved 
      in prior to the modifications in stop() API */
    #if (CY_PSOC5A)
        if(INA_PGA_m_P5backup.enableState == 1u)
        {
            INA_PGA_m_CR1_REG = INA_PGA_m_P5backup.scCR1Reg;
            INA_PGA_m_CR2_REG = INA_PGA_m_P5backup.scCR2Reg;
            INA_PGA_m_P5backup.enableState = 0u;
        }
    #endif /* CY_PSOC5A */   

    /* Enable power to the Amp in Active mode*/
    INA_PGA_m_PM_ACT_CFG_REG |= INA_PGA_m_ACT_PWR_EN;

    /* Enable power to the Amp in Alternative Active mode*/
    INA_PGA_m_PM_STBY_CFG_REG |= INA_PGA_m_STBY_PWR_EN;
    
    INA_PGA_m_PUMP_CR1_REG |= INA_PGA_m_PUMP_CR1_SC_CLKSEL;
    
    #if (!CY_PSOC5A)
        #if (CYDEV_VARIABLE_VDDA == 1u)
            if(CyScPumpEnabled == 1u)
            {
                INA_PGA_m_BSTCLK_REG &= (uint8)(~INA_PGA_m_BST_CLK_INDEX_MASK);
                INA_PGA_m_BSTCLK_REG |= INA_PGA_m_BST_CLK_EN | CyScBoostClk__INDEX;
                INA_PGA_m_SC_MISC_REG |= INA_PGA_m_PUMP_FORCE;
                CyScBoostClk_Start();
            }
            else
            {
                INA_PGA_m_BSTCLK_REG &= (uint8)(~INA_PGA_m_BST_CLK_EN);
                INA_PGA_m_SC_MISC_REG &= (uint8)(~INA_PGA_m_PUMP_FORCE);
            }
        #endif /* (CYDEV_VARIABLE_VDDA == 1u) */
    #endif /* (!CY_PSOC5A) */
}


/*******************************************************************************
* Function Name: INA_PGA_m_Start
********************************************************************************
*
* Summary:
*  The start function initializes the PGA with the default values and sets
*  the power to the given level. A power level of 0, is same as executing
*  the stop function.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void INA_PGA_m_Start(void) 
{

    /* This is to restore the value of register CR1 and CR2 which is saved 
      in prior to the modification in stop() API */

    if(INA_PGA_m_initVar == 0u)
    {
        INA_PGA_m_Init();
        INA_PGA_m_initVar = 1u;
    }

    INA_PGA_m_Enable();
}


/*******************************************************************************
* Function Name: INA_PGA_m_Stop
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
void INA_PGA_m_Stop(void) 
{ 
    /* Disble power to the Amp in Active mode template */
    INA_PGA_m_PM_ACT_CFG_REG &= (uint8)(~INA_PGA_m_ACT_PWR_EN);

    /* Disble power to the Amp in Alternative Active mode template */
    INA_PGA_m_PM_STBY_CFG_REG &= (uint8)(~INA_PGA_m_STBY_PWR_EN);

    #if (!CY_PSOC5A)
        #if (CYDEV_VARIABLE_VDDA == 1u)
            INA_PGA_m_BSTCLK_REG &= (uint8)(~INA_PGA_m_BST_CLK_EN);
            /* Disable pumps only if there aren't any SC block in use */
            if ((INA_PGA_m_PM_ACT_CFG_REG & INA_PGA_m_PM_ACT_CFG_MASK) == 0u)
            {
                INA_PGA_m_SC_MISC_REG &= (uint8)(~INA_PGA_m_PUMP_FORCE);
                INA_PGA_m_PUMP_CR1_REG &= (uint8)(~INA_PGA_m_PUMP_CR1_SC_CLKSEL);
                CyScBoostClk_Stop();
            }
        #endif /* CYDEV_VARIABLE_VDDA == 1u */
    #endif /* (CY_PSOC3 || CY_PSOC5LP) */

    /* This sets PGA in zero current mode and output routes are valid */
    #if (CY_PSOC5A)
        INA_PGA_m_P5backup.scCR1Reg = INA_PGA_m_CR1_REG;
        INA_PGA_m_P5backup.scCR2Reg = INA_PGA_m_CR2_REG;
        INA_PGA_m_CR1_REG = 0x00u;
        INA_PGA_m_CR2_REG = 0x00u;
        INA_PGA_m_P5backup.enableState = 1u;
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: INA_PGA_m_SetPower
********************************************************************************
*
* Summary:
*  Set the power of the PGA.
*
* Parameters:
*  power: Sets power level between (0) and (3) high power
*
* Return:
*  void
*
*******************************************************************************/
void INA_PGA_m_SetPower(uint8 power) 
{
    uint8 tmpCR;

    tmpCR = INA_PGA_m_CR1_REG & (uint8)(~INA_PGA_m_DRIVE_MASK);
    tmpCR |= (power & INA_PGA_m_DRIVE_MASK);
    INA_PGA_m_CR1_REG = tmpCR;  
}


/*******************************************************************************
* Function Name: INA_PGA_m_SetGain
********************************************************************************
*
* Summary:
*  This function sets values of the input and feedback resistors to set a 
*  specific gain of the amplifier.
*
* Parameters:
*  gain: Gain value of PGA (See header file for gain values.)
*
* Return:
*  void 
*
*******************************************************************************/
void INA_PGA_m_SetGain(uint8 gain) 
{
    /* Constant array for gain settings */
    const uint8 INA_PGA_m_GainArray[9] = { 
        (INA_PGA_m_RVAL_0K   | INA_PGA_m_R20_40B_40K | INA_PGA_m_BIAS_LOW), /* G=1  */
        (INA_PGA_m_RVAL_40K  | INA_PGA_m_R20_40B_40K | INA_PGA_m_BIAS_LOW), /* G=2  */
        (INA_PGA_m_RVAL_120K | INA_PGA_m_R20_40B_40K | INA_PGA_m_BIAS_LOW), /* G=4  */
        (INA_PGA_m_RVAL_280K | INA_PGA_m_R20_40B_40K | INA_PGA_m_BIAS_LOW), /* G=8  */
        (INA_PGA_m_RVAL_600K | INA_PGA_m_R20_40B_40K | INA_PGA_m_BIAS_LOW), /* G=16 */
        (INA_PGA_m_RVAL_460K | INA_PGA_m_R20_40B_40K | INA_PGA_m_BIAS_LOW), /* G=24, Sets Rin as 20k */
        (INA_PGA_m_RVAL_620K | INA_PGA_m_R20_40B_20K | INA_PGA_m_BIAS_LOW), /* G=32 */
        (INA_PGA_m_RVAL_470K | INA_PGA_m_R20_40B_20K | INA_PGA_m_BIAS_LOW), /* G=48, Sets Rin as 10k */
        (INA_PGA_m_RVAL_490K | INA_PGA_m_R20_40B_20K | INA_PGA_m_BIAS_LOW)  /* G=50, Sets Rin as 10k */
    };
    
    /* Constant array for gain compenstion settings */
    const uint8 INA_PGA_m_GainComp[9] = { 
        ( INA_PGA_m_COMP_4P35PF  | (uint8)( INA_PGA_m_REDC_00 >> 2 )), /* G=1  */
        ( INA_PGA_m_COMP_4P35PF  | (uint8)( INA_PGA_m_REDC_01 >> 2 )), /* G=2  */
        ( INA_PGA_m_COMP_3P0PF   | (uint8)( INA_PGA_m_REDC_01 >> 2 )), /* G=4  */
        ( INA_PGA_m_COMP_3P0PF   | (uint8)( INA_PGA_m_REDC_01 >> 2 )), /* G=8  */
        ( INA_PGA_m_COMP_3P6PF   | (uint8)( INA_PGA_m_REDC_01 >> 2 )), /* G=16 */
        ( INA_PGA_m_COMP_3P6PF   | (uint8)( INA_PGA_m_REDC_11 >> 2 )), /* G=24 */
        ( INA_PGA_m_COMP_3P6PF   | (uint8)( INA_PGA_m_REDC_11 >> 2 )), /* G=32 */
        ( INA_PGA_m_COMP_3P6PF   | (uint8)( INA_PGA_m_REDC_00 >> 2 )), /* G=48 */
        ( INA_PGA_m_COMP_3P6PF   | (uint8)( INA_PGA_m_REDC_00 >> 2 ))  /* G=50 */
    };
    
    /* Only set new gain if it is a valid gain */
    if( gain <= INA_PGA_m_GAIN_MAX)
    {
        /* Clear resistors, redc, and bias */
        INA_PGA_m_CR2_REG &= (uint8)(~(INA_PGA_m_RVAL_MASK | INA_PGA_m_R20_40B_MASK | 
                                INA_PGA_m_REDC_MASK | INA_PGA_m_BIAS_MASK ));

        /* Set gain value resistors, redc comp, and bias */
        INA_PGA_m_CR2_REG |= (INA_PGA_m_GainArray[gain] |
                                ((uint8)(INA_PGA_m_GainComp[gain] << 2 ) & INA_PGA_m_REDC_MASK));

        /* Clear sc_comp  */
        INA_PGA_m_CR1_REG &= (uint8)(~INA_PGA_m_COMP_MASK);
        /* Set sc_comp  */
        INA_PGA_m_CR1_REG |= ( INA_PGA_m_GainComp[gain] | INA_PGA_m_COMP_MASK );
    }
}


/* [] END OF FILE */
