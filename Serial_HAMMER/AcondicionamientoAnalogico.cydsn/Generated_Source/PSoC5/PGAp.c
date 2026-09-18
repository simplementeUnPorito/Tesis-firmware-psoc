/*******************************************************************************
* File Name: PGAp.c  
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

#include "PGAp.h"

#if (!CY_PSOC5A)
    #if (CYDEV_VARIABLE_VDDA == 1u)
        #include "CyScBoostClk.h"
    #endif /* (CYDEV_VARIABLE_VDDA == 1u) */
#endif /* (!CY_PSOC5A) */

#if (CY_PSOC5A)
    static PGAp_BACKUP_STRUCT  PGAp_P5backup;
#endif /* (CY_ PSOC5A) */

uint8 PGAp_initVar = 0u;


/*******************************************************************************   
* Function Name: PGAp_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  PGAp_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void PGAp_Init(void) 
{
    /* Set PGA mode */
    PGAp_CR0_REG = PGAp_MODE_PGA;      
    /* Set non-inverting PGA mode and reference mode */
    PGAp_CR1_REG |= PGAp_PGA_NINV;  
    /* Set default gain and ref mode */
    PGAp_CR2_REG = PGAp_VREF_MODE;
    /* Set gain and compensation */
    PGAp_SetGain(PGAp_DEFAULT_GAIN);
    /* Set power */
    PGAp_SetPower(PGAp_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: PGAp_Enable
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
void PGAp_Enable(void) 
{
    /* This is to restore the value of register CR1 and CR2 which is saved 
      in prior to the modifications in stop() API */
    #if (CY_PSOC5A)
        if(PGAp_P5backup.enableState == 1u)
        {
            PGAp_CR1_REG = PGAp_P5backup.scCR1Reg;
            PGAp_CR2_REG = PGAp_P5backup.scCR2Reg;
            PGAp_P5backup.enableState = 0u;
        }
    #endif /* CY_PSOC5A */   

    /* Enable power to the Amp in Active mode*/
    PGAp_PM_ACT_CFG_REG |= PGAp_ACT_PWR_EN;

    /* Enable power to the Amp in Alternative Active mode*/
    PGAp_PM_STBY_CFG_REG |= PGAp_STBY_PWR_EN;
    
    PGAp_PUMP_CR1_REG |= PGAp_PUMP_CR1_SC_CLKSEL;
    
    #if (!CY_PSOC5A)
        #if (CYDEV_VARIABLE_VDDA == 1u)
            if(CyScPumpEnabled == 1u)
            {
                PGAp_BSTCLK_REG &= (uint8)(~PGAp_BST_CLK_INDEX_MASK);
                PGAp_BSTCLK_REG |= PGAp_BST_CLK_EN | CyScBoostClk__INDEX;
                PGAp_SC_MISC_REG |= PGAp_PUMP_FORCE;
                CyScBoostClk_Start();
            }
            else
            {
                PGAp_BSTCLK_REG &= (uint8)(~PGAp_BST_CLK_EN);
                PGAp_SC_MISC_REG &= (uint8)(~PGAp_PUMP_FORCE);
            }
        #endif /* (CYDEV_VARIABLE_VDDA == 1u) */
    #endif /* (!CY_PSOC5A) */
}


/*******************************************************************************
* Function Name: PGAp_Start
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
void PGAp_Start(void) 
{

    /* This is to restore the value of register CR1 and CR2 which is saved 
      in prior to the modification in stop() API */

    if(PGAp_initVar == 0u)
    {
        PGAp_Init();
        PGAp_initVar = 1u;
    }

    PGAp_Enable();
}


/*******************************************************************************
* Function Name: PGAp_Stop
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
void PGAp_Stop(void) 
{ 
    /* Disble power to the Amp in Active mode template */
    PGAp_PM_ACT_CFG_REG &= (uint8)(~PGAp_ACT_PWR_EN);

    /* Disble power to the Amp in Alternative Active mode template */
    PGAp_PM_STBY_CFG_REG &= (uint8)(~PGAp_STBY_PWR_EN);

    #if (!CY_PSOC5A)
        #if (CYDEV_VARIABLE_VDDA == 1u)
            PGAp_BSTCLK_REG &= (uint8)(~PGAp_BST_CLK_EN);
            /* Disable pumps only if there aren't any SC block in use */
            if ((PGAp_PM_ACT_CFG_REG & PGAp_PM_ACT_CFG_MASK) == 0u)
            {
                PGAp_SC_MISC_REG &= (uint8)(~PGAp_PUMP_FORCE);
                PGAp_PUMP_CR1_REG &= (uint8)(~PGAp_PUMP_CR1_SC_CLKSEL);
                CyScBoostClk_Stop();
            }
        #endif /* CYDEV_VARIABLE_VDDA == 1u */
    #endif /* (CY_PSOC3 || CY_PSOC5LP) */

    /* This sets PGA in zero current mode and output routes are valid */
    #if (CY_PSOC5A)
        PGAp_P5backup.scCR1Reg = PGAp_CR1_REG;
        PGAp_P5backup.scCR2Reg = PGAp_CR2_REG;
        PGAp_CR1_REG = 0x00u;
        PGAp_CR2_REG = 0x00u;
        PGAp_P5backup.enableState = 1u;
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: PGAp_SetPower
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
void PGAp_SetPower(uint8 power) 
{
    uint8 tmpCR;

    tmpCR = PGAp_CR1_REG & (uint8)(~PGAp_DRIVE_MASK);
    tmpCR |= (power & PGAp_DRIVE_MASK);
    PGAp_CR1_REG = tmpCR;  
}


/*******************************************************************************
* Function Name: PGAp_SetGain
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
void PGAp_SetGain(uint8 gain) 
{
    /* Constant array for gain settings */
    const uint8 PGAp_GainArray[9] = { 
        (PGAp_RVAL_0K   | PGAp_R20_40B_40K | PGAp_BIAS_LOW), /* G=1  */
        (PGAp_RVAL_40K  | PGAp_R20_40B_40K | PGAp_BIAS_LOW), /* G=2  */
        (PGAp_RVAL_120K | PGAp_R20_40B_40K | PGAp_BIAS_LOW), /* G=4  */
        (PGAp_RVAL_280K | PGAp_R20_40B_40K | PGAp_BIAS_LOW), /* G=8  */
        (PGAp_RVAL_600K | PGAp_R20_40B_40K | PGAp_BIAS_LOW), /* G=16 */
        (PGAp_RVAL_460K | PGAp_R20_40B_40K | PGAp_BIAS_LOW), /* G=24, Sets Rin as 20k */
        (PGAp_RVAL_620K | PGAp_R20_40B_20K | PGAp_BIAS_LOW), /* G=32 */
        (PGAp_RVAL_470K | PGAp_R20_40B_20K | PGAp_BIAS_LOW), /* G=48, Sets Rin as 10k */
        (PGAp_RVAL_490K | PGAp_R20_40B_20K | PGAp_BIAS_LOW)  /* G=50, Sets Rin as 10k */
    };
    
    /* Constant array for gain compenstion settings */
    const uint8 PGAp_GainComp[9] = { 
        ( PGAp_COMP_4P35PF  | (uint8)( PGAp_REDC_00 >> 2 )), /* G=1  */
        ( PGAp_COMP_4P35PF  | (uint8)( PGAp_REDC_01 >> 2 )), /* G=2  */
        ( PGAp_COMP_3P0PF   | (uint8)( PGAp_REDC_01 >> 2 )), /* G=4  */
        ( PGAp_COMP_3P0PF   | (uint8)( PGAp_REDC_01 >> 2 )), /* G=8  */
        ( PGAp_COMP_3P6PF   | (uint8)( PGAp_REDC_01 >> 2 )), /* G=16 */
        ( PGAp_COMP_3P6PF   | (uint8)( PGAp_REDC_11 >> 2 )), /* G=24 */
        ( PGAp_COMP_3P6PF   | (uint8)( PGAp_REDC_11 >> 2 )), /* G=32 */
        ( PGAp_COMP_3P6PF   | (uint8)( PGAp_REDC_00 >> 2 )), /* G=48 */
        ( PGAp_COMP_3P6PF   | (uint8)( PGAp_REDC_00 >> 2 ))  /* G=50 */
    };
    
    /* Only set new gain if it is a valid gain */
    if( gain <= PGAp_GAIN_MAX)
    {
        /* Clear resistors, redc, and bias */
        PGAp_CR2_REG &= (uint8)(~(PGAp_RVAL_MASK | PGAp_R20_40B_MASK | 
                                PGAp_REDC_MASK | PGAp_BIAS_MASK ));

        /* Set gain value resistors, redc comp, and bias */
        PGAp_CR2_REG |= (PGAp_GainArray[gain] |
                                ((uint8)(PGAp_GainComp[gain] << 2 ) & PGAp_REDC_MASK));

        /* Clear sc_comp  */
        PGAp_CR1_REG &= (uint8)(~PGAp_COMP_MASK);
        /* Set sc_comp  */
        PGAp_CR1_REG |= ( PGAp_GainComp[gain] | PGAp_COMP_MASK );
    }
}


/* [] END OF FILE */
