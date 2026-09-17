/*******************************************************************************
* File Name: PGAshield.c  
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

#include "PGAshield.h"

#if (!CY_PSOC5A)
    #if (CYDEV_VARIABLE_VDDA == 1u)
        #include "CyScBoostClk.h"
    #endif /* (CYDEV_VARIABLE_VDDA == 1u) */
#endif /* (!CY_PSOC5A) */

#if (CY_PSOC5A)
    static PGAshield_BACKUP_STRUCT  PGAshield_P5backup;
#endif /* (CY_ PSOC5A) */

uint8 PGAshield_initVar = 0u;


/*******************************************************************************   
* Function Name: PGAshield_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  PGAshield_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void PGAshield_Init(void) 
{
    /* Set PGA mode */
    PGAshield_CR0_REG = PGAshield_MODE_PGA;      
    /* Set non-inverting PGA mode and reference mode */
    PGAshield_CR1_REG |= PGAshield_PGA_NINV;  
    /* Set default gain and ref mode */
    PGAshield_CR2_REG = PGAshield_VREF_MODE;
    /* Set gain and compensation */
    PGAshield_SetGain(PGAshield_DEFAULT_GAIN);
    /* Set power */
    PGAshield_SetPower(PGAshield_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: PGAshield_Enable
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
void PGAshield_Enable(void) 
{
    /* This is to restore the value of register CR1 and CR2 which is saved 
      in prior to the modifications in stop() API */
    #if (CY_PSOC5A)
        if(PGAshield_P5backup.enableState == 1u)
        {
            PGAshield_CR1_REG = PGAshield_P5backup.scCR1Reg;
            PGAshield_CR2_REG = PGAshield_P5backup.scCR2Reg;
            PGAshield_P5backup.enableState = 0u;
        }
    #endif /* CY_PSOC5A */   

    /* Enable power to the Amp in Active mode*/
    PGAshield_PM_ACT_CFG_REG |= PGAshield_ACT_PWR_EN;

    /* Enable power to the Amp in Alternative Active mode*/
    PGAshield_PM_STBY_CFG_REG |= PGAshield_STBY_PWR_EN;
    
    PGAshield_PUMP_CR1_REG |= PGAshield_PUMP_CR1_SC_CLKSEL;
    
    #if (!CY_PSOC5A)
        #if (CYDEV_VARIABLE_VDDA == 1u)
            if(CyScPumpEnabled == 1u)
            {
                PGAshield_BSTCLK_REG &= (uint8)(~PGAshield_BST_CLK_INDEX_MASK);
                PGAshield_BSTCLK_REG |= PGAshield_BST_CLK_EN | CyScBoostClk__INDEX;
                PGAshield_SC_MISC_REG |= PGAshield_PUMP_FORCE;
                CyScBoostClk_Start();
            }
            else
            {
                PGAshield_BSTCLK_REG &= (uint8)(~PGAshield_BST_CLK_EN);
                PGAshield_SC_MISC_REG &= (uint8)(~PGAshield_PUMP_FORCE);
            }
        #endif /* (CYDEV_VARIABLE_VDDA == 1u) */
    #endif /* (!CY_PSOC5A) */
}


/*******************************************************************************
* Function Name: PGAshield_Start
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
void PGAshield_Start(void) 
{

    /* This is to restore the value of register CR1 and CR2 which is saved 
      in prior to the modification in stop() API */

    if(PGAshield_initVar == 0u)
    {
        PGAshield_Init();
        PGAshield_initVar = 1u;
    }

    PGAshield_Enable();
}


/*******************************************************************************
* Function Name: PGAshield_Stop
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
void PGAshield_Stop(void) 
{ 
    /* Disble power to the Amp in Active mode template */
    PGAshield_PM_ACT_CFG_REG &= (uint8)(~PGAshield_ACT_PWR_EN);

    /* Disble power to the Amp in Alternative Active mode template */
    PGAshield_PM_STBY_CFG_REG &= (uint8)(~PGAshield_STBY_PWR_EN);

    #if (!CY_PSOC5A)
        #if (CYDEV_VARIABLE_VDDA == 1u)
            PGAshield_BSTCLK_REG &= (uint8)(~PGAshield_BST_CLK_EN);
            /* Disable pumps only if there aren't any SC block in use */
            if ((PGAshield_PM_ACT_CFG_REG & PGAshield_PM_ACT_CFG_MASK) == 0u)
            {
                PGAshield_SC_MISC_REG &= (uint8)(~PGAshield_PUMP_FORCE);
                PGAshield_PUMP_CR1_REG &= (uint8)(~PGAshield_PUMP_CR1_SC_CLKSEL);
                CyScBoostClk_Stop();
            }
        #endif /* CYDEV_VARIABLE_VDDA == 1u */
    #endif /* (CY_PSOC3 || CY_PSOC5LP) */

    /* This sets PGA in zero current mode and output routes are valid */
    #if (CY_PSOC5A)
        PGAshield_P5backup.scCR1Reg = PGAshield_CR1_REG;
        PGAshield_P5backup.scCR2Reg = PGAshield_CR2_REG;
        PGAshield_CR1_REG = 0x00u;
        PGAshield_CR2_REG = 0x00u;
        PGAshield_P5backup.enableState = 1u;
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: PGAshield_SetPower
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
void PGAshield_SetPower(uint8 power) 
{
    uint8 tmpCR;

    tmpCR = PGAshield_CR1_REG & (uint8)(~PGAshield_DRIVE_MASK);
    tmpCR |= (power & PGAshield_DRIVE_MASK);
    PGAshield_CR1_REG = tmpCR;  
}


/*******************************************************************************
* Function Name: PGAshield_SetGain
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
void PGAshield_SetGain(uint8 gain) 
{
    /* Constant array for gain settings */
    const uint8 PGAshield_GainArray[9] = { 
        (PGAshield_RVAL_0K   | PGAshield_R20_40B_40K | PGAshield_BIAS_LOW), /* G=1  */
        (PGAshield_RVAL_40K  | PGAshield_R20_40B_40K | PGAshield_BIAS_LOW), /* G=2  */
        (PGAshield_RVAL_120K | PGAshield_R20_40B_40K | PGAshield_BIAS_LOW), /* G=4  */
        (PGAshield_RVAL_280K | PGAshield_R20_40B_40K | PGAshield_BIAS_LOW), /* G=8  */
        (PGAshield_RVAL_600K | PGAshield_R20_40B_40K | PGAshield_BIAS_LOW), /* G=16 */
        (PGAshield_RVAL_460K | PGAshield_R20_40B_40K | PGAshield_BIAS_LOW), /* G=24, Sets Rin as 20k */
        (PGAshield_RVAL_620K | PGAshield_R20_40B_20K | PGAshield_BIAS_LOW), /* G=32 */
        (PGAshield_RVAL_470K | PGAshield_R20_40B_20K | PGAshield_BIAS_LOW), /* G=48, Sets Rin as 10k */
        (PGAshield_RVAL_490K | PGAshield_R20_40B_20K | PGAshield_BIAS_LOW)  /* G=50, Sets Rin as 10k */
    };
    
    /* Constant array for gain compenstion settings */
    const uint8 PGAshield_GainComp[9] = { 
        ( PGAshield_COMP_4P35PF  | (uint8)( PGAshield_REDC_00 >> 2 )), /* G=1  */
        ( PGAshield_COMP_4P35PF  | (uint8)( PGAshield_REDC_01 >> 2 )), /* G=2  */
        ( PGAshield_COMP_3P0PF   | (uint8)( PGAshield_REDC_01 >> 2 )), /* G=4  */
        ( PGAshield_COMP_3P0PF   | (uint8)( PGAshield_REDC_01 >> 2 )), /* G=8  */
        ( PGAshield_COMP_3P6PF   | (uint8)( PGAshield_REDC_01 >> 2 )), /* G=16 */
        ( PGAshield_COMP_3P6PF   | (uint8)( PGAshield_REDC_11 >> 2 )), /* G=24 */
        ( PGAshield_COMP_3P6PF   | (uint8)( PGAshield_REDC_11 >> 2 )), /* G=32 */
        ( PGAshield_COMP_3P6PF   | (uint8)( PGAshield_REDC_00 >> 2 )), /* G=48 */
        ( PGAshield_COMP_3P6PF   | (uint8)( PGAshield_REDC_00 >> 2 ))  /* G=50 */
    };
    
    /* Only set new gain if it is a valid gain */
    if( gain <= PGAshield_GAIN_MAX)
    {
        /* Clear resistors, redc, and bias */
        PGAshield_CR2_REG &= (uint8)(~(PGAshield_RVAL_MASK | PGAshield_R20_40B_MASK | 
                                PGAshield_REDC_MASK | PGAshield_BIAS_MASK ));

        /* Set gain value resistors, redc comp, and bias */
        PGAshield_CR2_REG |= (PGAshield_GainArray[gain] |
                                ((uint8)(PGAshield_GainComp[gain] << 2 ) & PGAshield_REDC_MASK));

        /* Clear sc_comp  */
        PGAshield_CR1_REG &= (uint8)(~PGAshield_COMP_MASK);
        /* Set sc_comp  */
        PGAshield_CR1_REG |= ( PGAshield_GainComp[gain] | PGAshield_COMP_MASK );
    }
}


/* [] END OF FILE */
