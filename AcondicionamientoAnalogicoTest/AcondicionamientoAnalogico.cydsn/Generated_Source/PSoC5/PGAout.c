/*******************************************************************************
* File Name: PGAout.c  
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

#include "PGAout.h"

#if (!CY_PSOC5A)
    #if (CYDEV_VARIABLE_VDDA == 1u)
        #include "CyScBoostClk.h"
    #endif /* (CYDEV_VARIABLE_VDDA == 1u) */
#endif /* (!CY_PSOC5A) */

#if (CY_PSOC5A)
    static PGAout_BACKUP_STRUCT  PGAout_P5backup;
#endif /* (CY_ PSOC5A) */

uint8 PGAout_initVar = 0u;


/*******************************************************************************   
* Function Name: PGAout_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  PGAout_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void PGAout_Init(void) 
{
    /* Set PGA mode */
    PGAout_CR0_REG = PGAout_MODE_PGA;      
    /* Set non-inverting PGA mode and reference mode */
    PGAout_CR1_REG |= PGAout_PGA_NINV;  
    /* Set default gain and ref mode */
    PGAout_CR2_REG = PGAout_VREF_MODE;
    /* Set gain and compensation */
    PGAout_SetGain(PGAout_DEFAULT_GAIN);
    /* Set power */
    PGAout_SetPower(PGAout_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: PGAout_Enable
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
void PGAout_Enable(void) 
{
    /* This is to restore the value of register CR1 and CR2 which is saved 
      in prior to the modifications in stop() API */
    #if (CY_PSOC5A)
        if(PGAout_P5backup.enableState == 1u)
        {
            PGAout_CR1_REG = PGAout_P5backup.scCR1Reg;
            PGAout_CR2_REG = PGAout_P5backup.scCR2Reg;
            PGAout_P5backup.enableState = 0u;
        }
    #endif /* CY_PSOC5A */   

    /* Enable power to the Amp in Active mode*/
    PGAout_PM_ACT_CFG_REG |= PGAout_ACT_PWR_EN;

    /* Enable power to the Amp in Alternative Active mode*/
    PGAout_PM_STBY_CFG_REG |= PGAout_STBY_PWR_EN;
    
    PGAout_PUMP_CR1_REG |= PGAout_PUMP_CR1_SC_CLKSEL;
    
    #if (!CY_PSOC5A)
        #if (CYDEV_VARIABLE_VDDA == 1u)
            if(CyScPumpEnabled == 1u)
            {
                PGAout_BSTCLK_REG &= (uint8)(~PGAout_BST_CLK_INDEX_MASK);
                PGAout_BSTCLK_REG |= PGAout_BST_CLK_EN | CyScBoostClk__INDEX;
                PGAout_SC_MISC_REG |= PGAout_PUMP_FORCE;
                CyScBoostClk_Start();
            }
            else
            {
                PGAout_BSTCLK_REG &= (uint8)(~PGAout_BST_CLK_EN);
                PGAout_SC_MISC_REG &= (uint8)(~PGAout_PUMP_FORCE);
            }
        #endif /* (CYDEV_VARIABLE_VDDA == 1u) */
    #endif /* (!CY_PSOC5A) */
}


/*******************************************************************************
* Function Name: PGAout_Start
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
void PGAout_Start(void) 
{

    /* This is to restore the value of register CR1 and CR2 which is saved 
      in prior to the modification in stop() API */

    if(PGAout_initVar == 0u)
    {
        PGAout_Init();
        PGAout_initVar = 1u;
    }

    PGAout_Enable();
}


/*******************************************************************************
* Function Name: PGAout_Stop
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
void PGAout_Stop(void) 
{ 
    /* Disble power to the Amp in Active mode template */
    PGAout_PM_ACT_CFG_REG &= (uint8)(~PGAout_ACT_PWR_EN);

    /* Disble power to the Amp in Alternative Active mode template */
    PGAout_PM_STBY_CFG_REG &= (uint8)(~PGAout_STBY_PWR_EN);

    #if (!CY_PSOC5A)
        #if (CYDEV_VARIABLE_VDDA == 1u)
            PGAout_BSTCLK_REG &= (uint8)(~PGAout_BST_CLK_EN);
            /* Disable pumps only if there aren't any SC block in use */
            if ((PGAout_PM_ACT_CFG_REG & PGAout_PM_ACT_CFG_MASK) == 0u)
            {
                PGAout_SC_MISC_REG &= (uint8)(~PGAout_PUMP_FORCE);
                PGAout_PUMP_CR1_REG &= (uint8)(~PGAout_PUMP_CR1_SC_CLKSEL);
                CyScBoostClk_Stop();
            }
        #endif /* CYDEV_VARIABLE_VDDA == 1u */
    #endif /* (CY_PSOC3 || CY_PSOC5LP) */

    /* This sets PGA in zero current mode and output routes are valid */
    #if (CY_PSOC5A)
        PGAout_P5backup.scCR1Reg = PGAout_CR1_REG;
        PGAout_P5backup.scCR2Reg = PGAout_CR2_REG;
        PGAout_CR1_REG = 0x00u;
        PGAout_CR2_REG = 0x00u;
        PGAout_P5backup.enableState = 1u;
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: PGAout_SetPower
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
void PGAout_SetPower(uint8 power) 
{
    uint8 tmpCR;

    tmpCR = PGAout_CR1_REG & (uint8)(~PGAout_DRIVE_MASK);
    tmpCR |= (power & PGAout_DRIVE_MASK);
    PGAout_CR1_REG = tmpCR;  
}


/*******************************************************************************
* Function Name: PGAout_SetGain
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
void PGAout_SetGain(uint8 gain) 
{
    /* Constant array for gain settings */
    const uint8 PGAout_GainArray[9] = { 
        (PGAout_RVAL_0K   | PGAout_R20_40B_40K | PGAout_BIAS_LOW), /* G=1  */
        (PGAout_RVAL_40K  | PGAout_R20_40B_40K | PGAout_BIAS_LOW), /* G=2  */
        (PGAout_RVAL_120K | PGAout_R20_40B_40K | PGAout_BIAS_LOW), /* G=4  */
        (PGAout_RVAL_280K | PGAout_R20_40B_40K | PGAout_BIAS_LOW), /* G=8  */
        (PGAout_RVAL_600K | PGAout_R20_40B_40K | PGAout_BIAS_LOW), /* G=16 */
        (PGAout_RVAL_460K | PGAout_R20_40B_40K | PGAout_BIAS_LOW), /* G=24, Sets Rin as 20k */
        (PGAout_RVAL_620K | PGAout_R20_40B_20K | PGAout_BIAS_LOW), /* G=32 */
        (PGAout_RVAL_470K | PGAout_R20_40B_20K | PGAout_BIAS_LOW), /* G=48, Sets Rin as 10k */
        (PGAout_RVAL_490K | PGAout_R20_40B_20K | PGAout_BIAS_LOW)  /* G=50, Sets Rin as 10k */
    };
    
    /* Constant array for gain compenstion settings */
    const uint8 PGAout_GainComp[9] = { 
        ( PGAout_COMP_4P35PF  | (uint8)( PGAout_REDC_00 >> 2 )), /* G=1  */
        ( PGAout_COMP_4P35PF  | (uint8)( PGAout_REDC_01 >> 2 )), /* G=2  */
        ( PGAout_COMP_3P0PF   | (uint8)( PGAout_REDC_01 >> 2 )), /* G=4  */
        ( PGAout_COMP_3P0PF   | (uint8)( PGAout_REDC_01 >> 2 )), /* G=8  */
        ( PGAout_COMP_3P6PF   | (uint8)( PGAout_REDC_01 >> 2 )), /* G=16 */
        ( PGAout_COMP_3P6PF   | (uint8)( PGAout_REDC_11 >> 2 )), /* G=24 */
        ( PGAout_COMP_3P6PF   | (uint8)( PGAout_REDC_11 >> 2 )), /* G=32 */
        ( PGAout_COMP_3P6PF   | (uint8)( PGAout_REDC_00 >> 2 )), /* G=48 */
        ( PGAout_COMP_3P6PF   | (uint8)( PGAout_REDC_00 >> 2 ))  /* G=50 */
    };
    
    /* Only set new gain if it is a valid gain */
    if( gain <= PGAout_GAIN_MAX)
    {
        /* Clear resistors, redc, and bias */
        PGAout_CR2_REG &= (uint8)(~(PGAout_RVAL_MASK | PGAout_R20_40B_MASK | 
                                PGAout_REDC_MASK | PGAout_BIAS_MASK ));

        /* Set gain value resistors, redc comp, and bias */
        PGAout_CR2_REG |= (PGAout_GainArray[gain] |
                                ((uint8)(PGAout_GainComp[gain] << 2 ) & PGAout_REDC_MASK));

        /* Clear sc_comp  */
        PGAout_CR1_REG &= (uint8)(~PGAout_COMP_MASK);
        /* Set sc_comp  */
        PGAout_CR1_REG |= ( PGAout_GainComp[gain] | PGAout_COMP_MASK );
    }
}


/* [] END OF FILE */
