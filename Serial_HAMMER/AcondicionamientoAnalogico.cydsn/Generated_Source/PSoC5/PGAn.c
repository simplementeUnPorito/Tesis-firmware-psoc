/*******************************************************************************
* File Name: PGAn.c  
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

#include "PGAn.h"

#if (!CY_PSOC5A)
    #if (CYDEV_VARIABLE_VDDA == 1u)
        #include "CyScBoostClk.h"
    #endif /* (CYDEV_VARIABLE_VDDA == 1u) */
#endif /* (!CY_PSOC5A) */

#if (CY_PSOC5A)
    static PGAn_BACKUP_STRUCT  PGAn_P5backup;
#endif /* (CY_ PSOC5A) */

uint8 PGAn_initVar = 0u;


/*******************************************************************************   
* Function Name: PGAn_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  PGAn_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void PGAn_Init(void) 
{
    /* Set PGA mode */
    PGAn_CR0_REG = PGAn_MODE_PGA;      
    /* Set non-inverting PGA mode and reference mode */
    PGAn_CR1_REG |= PGAn_PGA_NINV;  
    /* Set default gain and ref mode */
    PGAn_CR2_REG = PGAn_VREF_MODE;
    /* Set gain and compensation */
    PGAn_SetGain(PGAn_DEFAULT_GAIN);
    /* Set power */
    PGAn_SetPower(PGAn_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: PGAn_Enable
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
void PGAn_Enable(void) 
{
    /* This is to restore the value of register CR1 and CR2 which is saved 
      in prior to the modifications in stop() API */
    #if (CY_PSOC5A)
        if(PGAn_P5backup.enableState == 1u)
        {
            PGAn_CR1_REG = PGAn_P5backup.scCR1Reg;
            PGAn_CR2_REG = PGAn_P5backup.scCR2Reg;
            PGAn_P5backup.enableState = 0u;
        }
    #endif /* CY_PSOC5A */   

    /* Enable power to the Amp in Active mode*/
    PGAn_PM_ACT_CFG_REG |= PGAn_ACT_PWR_EN;

    /* Enable power to the Amp in Alternative Active mode*/
    PGAn_PM_STBY_CFG_REG |= PGAn_STBY_PWR_EN;
    
    PGAn_PUMP_CR1_REG |= PGAn_PUMP_CR1_SC_CLKSEL;
    
    #if (!CY_PSOC5A)
        #if (CYDEV_VARIABLE_VDDA == 1u)
            if(CyScPumpEnabled == 1u)
            {
                PGAn_BSTCLK_REG &= (uint8)(~PGAn_BST_CLK_INDEX_MASK);
                PGAn_BSTCLK_REG |= PGAn_BST_CLK_EN | CyScBoostClk__INDEX;
                PGAn_SC_MISC_REG |= PGAn_PUMP_FORCE;
                CyScBoostClk_Start();
            }
            else
            {
                PGAn_BSTCLK_REG &= (uint8)(~PGAn_BST_CLK_EN);
                PGAn_SC_MISC_REG &= (uint8)(~PGAn_PUMP_FORCE);
            }
        #endif /* (CYDEV_VARIABLE_VDDA == 1u) */
    #endif /* (!CY_PSOC5A) */
}


/*******************************************************************************
* Function Name: PGAn_Start
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
void PGAn_Start(void) 
{

    /* This is to restore the value of register CR1 and CR2 which is saved 
      in prior to the modification in stop() API */

    if(PGAn_initVar == 0u)
    {
        PGAn_Init();
        PGAn_initVar = 1u;
    }

    PGAn_Enable();
}


/*******************************************************************************
* Function Name: PGAn_Stop
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
void PGAn_Stop(void) 
{ 
    /* Disble power to the Amp in Active mode template */
    PGAn_PM_ACT_CFG_REG &= (uint8)(~PGAn_ACT_PWR_EN);

    /* Disble power to the Amp in Alternative Active mode template */
    PGAn_PM_STBY_CFG_REG &= (uint8)(~PGAn_STBY_PWR_EN);

    #if (!CY_PSOC5A)
        #if (CYDEV_VARIABLE_VDDA == 1u)
            PGAn_BSTCLK_REG &= (uint8)(~PGAn_BST_CLK_EN);
            /* Disable pumps only if there aren't any SC block in use */
            if ((PGAn_PM_ACT_CFG_REG & PGAn_PM_ACT_CFG_MASK) == 0u)
            {
                PGAn_SC_MISC_REG &= (uint8)(~PGAn_PUMP_FORCE);
                PGAn_PUMP_CR1_REG &= (uint8)(~PGAn_PUMP_CR1_SC_CLKSEL);
                CyScBoostClk_Stop();
            }
        #endif /* CYDEV_VARIABLE_VDDA == 1u */
    #endif /* (CY_PSOC3 || CY_PSOC5LP) */

    /* This sets PGA in zero current mode and output routes are valid */
    #if (CY_PSOC5A)
        PGAn_P5backup.scCR1Reg = PGAn_CR1_REG;
        PGAn_P5backup.scCR2Reg = PGAn_CR2_REG;
        PGAn_CR1_REG = 0x00u;
        PGAn_CR2_REG = 0x00u;
        PGAn_P5backup.enableState = 1u;
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: PGAn_SetPower
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
void PGAn_SetPower(uint8 power) 
{
    uint8 tmpCR;

    tmpCR = PGAn_CR1_REG & (uint8)(~PGAn_DRIVE_MASK);
    tmpCR |= (power & PGAn_DRIVE_MASK);
    PGAn_CR1_REG = tmpCR;  
}


/*******************************************************************************
* Function Name: PGAn_SetGain
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
void PGAn_SetGain(uint8 gain) 
{
    /* Constant array for gain settings */
    const uint8 PGAn_GainArray[9] = { 
        (PGAn_RVAL_0K   | PGAn_R20_40B_40K | PGAn_BIAS_LOW), /* G=1  */
        (PGAn_RVAL_40K  | PGAn_R20_40B_40K | PGAn_BIAS_LOW), /* G=2  */
        (PGAn_RVAL_120K | PGAn_R20_40B_40K | PGAn_BIAS_LOW), /* G=4  */
        (PGAn_RVAL_280K | PGAn_R20_40B_40K | PGAn_BIAS_LOW), /* G=8  */
        (PGAn_RVAL_600K | PGAn_R20_40B_40K | PGAn_BIAS_LOW), /* G=16 */
        (PGAn_RVAL_460K | PGAn_R20_40B_40K | PGAn_BIAS_LOW), /* G=24, Sets Rin as 20k */
        (PGAn_RVAL_620K | PGAn_R20_40B_20K | PGAn_BIAS_LOW), /* G=32 */
        (PGAn_RVAL_470K | PGAn_R20_40B_20K | PGAn_BIAS_LOW), /* G=48, Sets Rin as 10k */
        (PGAn_RVAL_490K | PGAn_R20_40B_20K | PGAn_BIAS_LOW)  /* G=50, Sets Rin as 10k */
    };
    
    /* Constant array for gain compenstion settings */
    const uint8 PGAn_GainComp[9] = { 
        ( PGAn_COMP_4P35PF  | (uint8)( PGAn_REDC_00 >> 2 )), /* G=1  */
        ( PGAn_COMP_4P35PF  | (uint8)( PGAn_REDC_01 >> 2 )), /* G=2  */
        ( PGAn_COMP_3P0PF   | (uint8)( PGAn_REDC_01 >> 2 )), /* G=4  */
        ( PGAn_COMP_3P0PF   | (uint8)( PGAn_REDC_01 >> 2 )), /* G=8  */
        ( PGAn_COMP_3P6PF   | (uint8)( PGAn_REDC_01 >> 2 )), /* G=16 */
        ( PGAn_COMP_3P6PF   | (uint8)( PGAn_REDC_11 >> 2 )), /* G=24 */
        ( PGAn_COMP_3P6PF   | (uint8)( PGAn_REDC_11 >> 2 )), /* G=32 */
        ( PGAn_COMP_3P6PF   | (uint8)( PGAn_REDC_00 >> 2 )), /* G=48 */
        ( PGAn_COMP_3P6PF   | (uint8)( PGAn_REDC_00 >> 2 ))  /* G=50 */
    };
    
    /* Only set new gain if it is a valid gain */
    if( gain <= PGAn_GAIN_MAX)
    {
        /* Clear resistors, redc, and bias */
        PGAn_CR2_REG &= (uint8)(~(PGAn_RVAL_MASK | PGAn_R20_40B_MASK | 
                                PGAn_REDC_MASK | PGAn_BIAS_MASK ));

        /* Set gain value resistors, redc comp, and bias */
        PGAn_CR2_REG |= (PGAn_GainArray[gain] |
                                ((uint8)(PGAn_GainComp[gain] << 2 ) & PGAn_REDC_MASK));

        /* Clear sc_comp  */
        PGAn_CR1_REG &= (uint8)(~PGAn_COMP_MASK);
        /* Set sc_comp  */
        PGAn_CR1_REG |= ( PGAn_GainComp[gain] | PGAn_COMP_MASK );
    }
}


/* [] END OF FILE */
