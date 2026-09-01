/*******************************************************************************
* File Name: PGAIn.c  
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

#include "PGAIn.h"

#if (!CY_PSOC5A)
    #if (CYDEV_VARIABLE_VDDA == 1u)
        #include "CyScBoostClk.h"
    #endif /* (CYDEV_VARIABLE_VDDA == 1u) */
#endif /* (!CY_PSOC5A) */

#if (CY_PSOC5A)
    static PGAIn_BACKUP_STRUCT  PGAIn_P5backup;
#endif /* (CY_ PSOC5A) */

uint8 PGAIn_initVar = 0u;


/*******************************************************************************   
* Function Name: PGAIn_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  PGAIn_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void PGAIn_Init(void) 
{
    /* Set PGA mode */
    PGAIn_CR0_REG = PGAIn_MODE_PGA;      
    /* Set non-inverting PGA mode and reference mode */
    PGAIn_CR1_REG |= PGAIn_PGA_NINV;  
    /* Set default gain and ref mode */
    PGAIn_CR2_REG = PGAIn_VREF_MODE;
    /* Set gain and compensation */
    PGAIn_SetGain(PGAIn_DEFAULT_GAIN);
    /* Set power */
    PGAIn_SetPower(PGAIn_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: PGAIn_Enable
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
void PGAIn_Enable(void) 
{
    /* This is to restore the value of register CR1 and CR2 which is saved 
      in prior to the modifications in stop() API */
    #if (CY_PSOC5A)
        if(PGAIn_P5backup.enableState == 1u)
        {
            PGAIn_CR1_REG = PGAIn_P5backup.scCR1Reg;
            PGAIn_CR2_REG = PGAIn_P5backup.scCR2Reg;
            PGAIn_P5backup.enableState = 0u;
        }
    #endif /* CY_PSOC5A */   

    /* Enable power to the Amp in Active mode*/
    PGAIn_PM_ACT_CFG_REG |= PGAIn_ACT_PWR_EN;

    /* Enable power to the Amp in Alternative Active mode*/
    PGAIn_PM_STBY_CFG_REG |= PGAIn_STBY_PWR_EN;
    
    PGAIn_PUMP_CR1_REG |= PGAIn_PUMP_CR1_SC_CLKSEL;
    
    #if (!CY_PSOC5A)
        #if (CYDEV_VARIABLE_VDDA == 1u)
            if(CyScPumpEnabled == 1u)
            {
                PGAIn_BSTCLK_REG &= (uint8)(~PGAIn_BST_CLK_INDEX_MASK);
                PGAIn_BSTCLK_REG |= PGAIn_BST_CLK_EN | CyScBoostClk__INDEX;
                PGAIn_SC_MISC_REG |= PGAIn_PUMP_FORCE;
                CyScBoostClk_Start();
            }
            else
            {
                PGAIn_BSTCLK_REG &= (uint8)(~PGAIn_BST_CLK_EN);
                PGAIn_SC_MISC_REG &= (uint8)(~PGAIn_PUMP_FORCE);
            }
        #endif /* (CYDEV_VARIABLE_VDDA == 1u) */
    #endif /* (!CY_PSOC5A) */
}


/*******************************************************************************
* Function Name: PGAIn_Start
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
void PGAIn_Start(void) 
{

    /* This is to restore the value of register CR1 and CR2 which is saved 
      in prior to the modification in stop() API */

    if(PGAIn_initVar == 0u)
    {
        PGAIn_Init();
        PGAIn_initVar = 1u;
    }

    PGAIn_Enable();
}


/*******************************************************************************
* Function Name: PGAIn_Stop
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
void PGAIn_Stop(void) 
{ 
    /* Disble power to the Amp in Active mode template */
    PGAIn_PM_ACT_CFG_REG &= (uint8)(~PGAIn_ACT_PWR_EN);

    /* Disble power to the Amp in Alternative Active mode template */
    PGAIn_PM_STBY_CFG_REG &= (uint8)(~PGAIn_STBY_PWR_EN);

    #if (!CY_PSOC5A)
        #if (CYDEV_VARIABLE_VDDA == 1u)
            PGAIn_BSTCLK_REG &= (uint8)(~PGAIn_BST_CLK_EN);
            /* Disable pumps only if there aren't any SC block in use */
            if ((PGAIn_PM_ACT_CFG_REG & PGAIn_PM_ACT_CFG_MASK) == 0u)
            {
                PGAIn_SC_MISC_REG &= (uint8)(~PGAIn_PUMP_FORCE);
                PGAIn_PUMP_CR1_REG &= (uint8)(~PGAIn_PUMP_CR1_SC_CLKSEL);
                CyScBoostClk_Stop();
            }
        #endif /* CYDEV_VARIABLE_VDDA == 1u */
    #endif /* (CY_PSOC3 || CY_PSOC5LP) */

    /* This sets PGA in zero current mode and output routes are valid */
    #if (CY_PSOC5A)
        PGAIn_P5backup.scCR1Reg = PGAIn_CR1_REG;
        PGAIn_P5backup.scCR2Reg = PGAIn_CR2_REG;
        PGAIn_CR1_REG = 0x00u;
        PGAIn_CR2_REG = 0x00u;
        PGAIn_P5backup.enableState = 1u;
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: PGAIn_SetPower
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
void PGAIn_SetPower(uint8 power) 
{
    uint8 tmpCR;

    tmpCR = PGAIn_CR1_REG & (uint8)(~PGAIn_DRIVE_MASK);
    tmpCR |= (power & PGAIn_DRIVE_MASK);
    PGAIn_CR1_REG = tmpCR;  
}


/*******************************************************************************
* Function Name: PGAIn_SetGain
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
void PGAIn_SetGain(uint8 gain) 
{
    /* Constant array for gain settings */
    const uint8 PGAIn_GainArray[9] = { 
        (PGAIn_RVAL_0K   | PGAIn_R20_40B_40K | PGAIn_BIAS_LOW), /* G=1  */
        (PGAIn_RVAL_40K  | PGAIn_R20_40B_40K | PGAIn_BIAS_LOW), /* G=2  */
        (PGAIn_RVAL_120K | PGAIn_R20_40B_40K | PGAIn_BIAS_LOW), /* G=4  */
        (PGAIn_RVAL_280K | PGAIn_R20_40B_40K | PGAIn_BIAS_LOW), /* G=8  */
        (PGAIn_RVAL_600K | PGAIn_R20_40B_40K | PGAIn_BIAS_LOW), /* G=16 */
        (PGAIn_RVAL_460K | PGAIn_R20_40B_40K | PGAIn_BIAS_LOW), /* G=24, Sets Rin as 20k */
        (PGAIn_RVAL_620K | PGAIn_R20_40B_20K | PGAIn_BIAS_LOW), /* G=32 */
        (PGAIn_RVAL_470K | PGAIn_R20_40B_20K | PGAIn_BIAS_LOW), /* G=48, Sets Rin as 10k */
        (PGAIn_RVAL_490K | PGAIn_R20_40B_20K | PGAIn_BIAS_LOW)  /* G=50, Sets Rin as 10k */
    };
    
    /* Constant array for gain compenstion settings */
    const uint8 PGAIn_GainComp[9] = { 
        ( PGAIn_COMP_4P35PF  | (uint8)( PGAIn_REDC_00 >> 2 )), /* G=1  */
        ( PGAIn_COMP_4P35PF  | (uint8)( PGAIn_REDC_01 >> 2 )), /* G=2  */
        ( PGAIn_COMP_3P0PF   | (uint8)( PGAIn_REDC_01 >> 2 )), /* G=4  */
        ( PGAIn_COMP_3P0PF   | (uint8)( PGAIn_REDC_01 >> 2 )), /* G=8  */
        ( PGAIn_COMP_3P6PF   | (uint8)( PGAIn_REDC_01 >> 2 )), /* G=16 */
        ( PGAIn_COMP_3P6PF   | (uint8)( PGAIn_REDC_11 >> 2 )), /* G=24 */
        ( PGAIn_COMP_3P6PF   | (uint8)( PGAIn_REDC_11 >> 2 )), /* G=32 */
        ( PGAIn_COMP_3P6PF   | (uint8)( PGAIn_REDC_00 >> 2 )), /* G=48 */
        ( PGAIn_COMP_3P6PF   | (uint8)( PGAIn_REDC_00 >> 2 ))  /* G=50 */
    };
    
    /* Only set new gain if it is a valid gain */
    if( gain <= PGAIn_GAIN_MAX)
    {
        /* Clear resistors, redc, and bias */
        PGAIn_CR2_REG &= (uint8)(~(PGAIn_RVAL_MASK | PGAIn_R20_40B_MASK | 
                                PGAIn_REDC_MASK | PGAIn_BIAS_MASK ));

        /* Set gain value resistors, redc comp, and bias */
        PGAIn_CR2_REG |= (PGAIn_GainArray[gain] |
                                ((uint8)(PGAIn_GainComp[gain] << 2 ) & PGAIn_REDC_MASK));

        /* Clear sc_comp  */
        PGAIn_CR1_REG &= (uint8)(~PGAIn_COMP_MASK);
        /* Set sc_comp  */
        PGAIn_CR1_REG |= ( PGAIn_GainComp[gain] | PGAIn_COMP_MASK );
    }
}


/* [] END OF FILE */
