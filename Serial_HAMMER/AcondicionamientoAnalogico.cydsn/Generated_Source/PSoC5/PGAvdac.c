/*******************************************************************************
* File Name: PGAvdac.c  
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

#include "PGAvdac.h"

#if (!CY_PSOC5A)
    #if (CYDEV_VARIABLE_VDDA == 1u)
        #include "CyScBoostClk.h"
    #endif /* (CYDEV_VARIABLE_VDDA == 1u) */
#endif /* (!CY_PSOC5A) */

#if (CY_PSOC5A)
    static PGAvdac_BACKUP_STRUCT  PGAvdac_P5backup;
#endif /* (CY_ PSOC5A) */

uint8 PGAvdac_initVar = 0u;


/*******************************************************************************   
* Function Name: PGAvdac_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  PGAvdac_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void PGAvdac_Init(void) 
{
    /* Set PGA mode */
    PGAvdac_CR0_REG = PGAvdac_MODE_PGA;      
    /* Set non-inverting PGA mode and reference mode */
    PGAvdac_CR1_REG |= PGAvdac_PGA_NINV;  
    /* Set default gain and ref mode */
    PGAvdac_CR2_REG = PGAvdac_VREF_MODE;
    /* Set gain and compensation */
    PGAvdac_SetGain(PGAvdac_DEFAULT_GAIN);
    /* Set power */
    PGAvdac_SetPower(PGAvdac_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: PGAvdac_Enable
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
void PGAvdac_Enable(void) 
{
    /* This is to restore the value of register CR1 and CR2 which is saved 
      in prior to the modifications in stop() API */
    #if (CY_PSOC5A)
        if(PGAvdac_P5backup.enableState == 1u)
        {
            PGAvdac_CR1_REG = PGAvdac_P5backup.scCR1Reg;
            PGAvdac_CR2_REG = PGAvdac_P5backup.scCR2Reg;
            PGAvdac_P5backup.enableState = 0u;
        }
    #endif /* CY_PSOC5A */   

    /* Enable power to the Amp in Active mode*/
    PGAvdac_PM_ACT_CFG_REG |= PGAvdac_ACT_PWR_EN;

    /* Enable power to the Amp in Alternative Active mode*/
    PGAvdac_PM_STBY_CFG_REG |= PGAvdac_STBY_PWR_EN;
    
    PGAvdac_PUMP_CR1_REG |= PGAvdac_PUMP_CR1_SC_CLKSEL;
    
    #if (!CY_PSOC5A)
        #if (CYDEV_VARIABLE_VDDA == 1u)
            if(CyScPumpEnabled == 1u)
            {
                PGAvdac_BSTCLK_REG &= (uint8)(~PGAvdac_BST_CLK_INDEX_MASK);
                PGAvdac_BSTCLK_REG |= PGAvdac_BST_CLK_EN | CyScBoostClk__INDEX;
                PGAvdac_SC_MISC_REG |= PGAvdac_PUMP_FORCE;
                CyScBoostClk_Start();
            }
            else
            {
                PGAvdac_BSTCLK_REG &= (uint8)(~PGAvdac_BST_CLK_EN);
                PGAvdac_SC_MISC_REG &= (uint8)(~PGAvdac_PUMP_FORCE);
            }
        #endif /* (CYDEV_VARIABLE_VDDA == 1u) */
    #endif /* (!CY_PSOC5A) */
}


/*******************************************************************************
* Function Name: PGAvdac_Start
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
void PGAvdac_Start(void) 
{

    /* This is to restore the value of register CR1 and CR2 which is saved 
      in prior to the modification in stop() API */

    if(PGAvdac_initVar == 0u)
    {
        PGAvdac_Init();
        PGAvdac_initVar = 1u;
    }

    PGAvdac_Enable();
}


/*******************************************************************************
* Function Name: PGAvdac_Stop
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
void PGAvdac_Stop(void) 
{ 
    /* Disble power to the Amp in Active mode template */
    PGAvdac_PM_ACT_CFG_REG &= (uint8)(~PGAvdac_ACT_PWR_EN);

    /* Disble power to the Amp in Alternative Active mode template */
    PGAvdac_PM_STBY_CFG_REG &= (uint8)(~PGAvdac_STBY_PWR_EN);

    #if (!CY_PSOC5A)
        #if (CYDEV_VARIABLE_VDDA == 1u)
            PGAvdac_BSTCLK_REG &= (uint8)(~PGAvdac_BST_CLK_EN);
            /* Disable pumps only if there aren't any SC block in use */
            if ((PGAvdac_PM_ACT_CFG_REG & PGAvdac_PM_ACT_CFG_MASK) == 0u)
            {
                PGAvdac_SC_MISC_REG &= (uint8)(~PGAvdac_PUMP_FORCE);
                PGAvdac_PUMP_CR1_REG &= (uint8)(~PGAvdac_PUMP_CR1_SC_CLKSEL);
                CyScBoostClk_Stop();
            }
        #endif /* CYDEV_VARIABLE_VDDA == 1u */
    #endif /* (CY_PSOC3 || CY_PSOC5LP) */

    /* This sets PGA in zero current mode and output routes are valid */
    #if (CY_PSOC5A)
        PGAvdac_P5backup.scCR1Reg = PGAvdac_CR1_REG;
        PGAvdac_P5backup.scCR2Reg = PGAvdac_CR2_REG;
        PGAvdac_CR1_REG = 0x00u;
        PGAvdac_CR2_REG = 0x00u;
        PGAvdac_P5backup.enableState = 1u;
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: PGAvdac_SetPower
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
void PGAvdac_SetPower(uint8 power) 
{
    uint8 tmpCR;

    tmpCR = PGAvdac_CR1_REG & (uint8)(~PGAvdac_DRIVE_MASK);
    tmpCR |= (power & PGAvdac_DRIVE_MASK);
    PGAvdac_CR1_REG = tmpCR;  
}


/*******************************************************************************
* Function Name: PGAvdac_SetGain
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
void PGAvdac_SetGain(uint8 gain) 
{
    /* Constant array for gain settings */
    const uint8 PGAvdac_GainArray[9] = { 
        (PGAvdac_RVAL_0K   | PGAvdac_R20_40B_40K | PGAvdac_BIAS_LOW), /* G=1  */
        (PGAvdac_RVAL_40K  | PGAvdac_R20_40B_40K | PGAvdac_BIAS_LOW), /* G=2  */
        (PGAvdac_RVAL_120K | PGAvdac_R20_40B_40K | PGAvdac_BIAS_LOW), /* G=4  */
        (PGAvdac_RVAL_280K | PGAvdac_R20_40B_40K | PGAvdac_BIAS_LOW), /* G=8  */
        (PGAvdac_RVAL_600K | PGAvdac_R20_40B_40K | PGAvdac_BIAS_LOW), /* G=16 */
        (PGAvdac_RVAL_460K | PGAvdac_R20_40B_40K | PGAvdac_BIAS_LOW), /* G=24, Sets Rin as 20k */
        (PGAvdac_RVAL_620K | PGAvdac_R20_40B_20K | PGAvdac_BIAS_LOW), /* G=32 */
        (PGAvdac_RVAL_470K | PGAvdac_R20_40B_20K | PGAvdac_BIAS_LOW), /* G=48, Sets Rin as 10k */
        (PGAvdac_RVAL_490K | PGAvdac_R20_40B_20K | PGAvdac_BIAS_LOW)  /* G=50, Sets Rin as 10k */
    };
    
    /* Constant array for gain compenstion settings */
    const uint8 PGAvdac_GainComp[9] = { 
        ( PGAvdac_COMP_4P35PF  | (uint8)( PGAvdac_REDC_00 >> 2 )), /* G=1  */
        ( PGAvdac_COMP_4P35PF  | (uint8)( PGAvdac_REDC_01 >> 2 )), /* G=2  */
        ( PGAvdac_COMP_3P0PF   | (uint8)( PGAvdac_REDC_01 >> 2 )), /* G=4  */
        ( PGAvdac_COMP_3P0PF   | (uint8)( PGAvdac_REDC_01 >> 2 )), /* G=8  */
        ( PGAvdac_COMP_3P6PF   | (uint8)( PGAvdac_REDC_01 >> 2 )), /* G=16 */
        ( PGAvdac_COMP_3P6PF   | (uint8)( PGAvdac_REDC_11 >> 2 )), /* G=24 */
        ( PGAvdac_COMP_3P6PF   | (uint8)( PGAvdac_REDC_11 >> 2 )), /* G=32 */
        ( PGAvdac_COMP_3P6PF   | (uint8)( PGAvdac_REDC_00 >> 2 )), /* G=48 */
        ( PGAvdac_COMP_3P6PF   | (uint8)( PGAvdac_REDC_00 >> 2 ))  /* G=50 */
    };
    
    /* Only set new gain if it is a valid gain */
    if( gain <= PGAvdac_GAIN_MAX)
    {
        /* Clear resistors, redc, and bias */
        PGAvdac_CR2_REG &= (uint8)(~(PGAvdac_RVAL_MASK | PGAvdac_R20_40B_MASK | 
                                PGAvdac_REDC_MASK | PGAvdac_BIAS_MASK ));

        /* Set gain value resistors, redc comp, and bias */
        PGAvdac_CR2_REG |= (PGAvdac_GainArray[gain] |
                                ((uint8)(PGAvdac_GainComp[gain] << 2 ) & PGAvdac_REDC_MASK));

        /* Clear sc_comp  */
        PGAvdac_CR1_REG &= (uint8)(~PGAvdac_COMP_MASK);
        /* Set sc_comp  */
        PGAvdac_CR1_REG |= ( PGAvdac_GainComp[gain] | PGAvdac_COMP_MASK );
    }
}


/* [] END OF FILE */
