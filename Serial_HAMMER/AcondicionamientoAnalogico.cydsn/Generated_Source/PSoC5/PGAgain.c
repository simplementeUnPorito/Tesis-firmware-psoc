/*******************************************************************************
* File Name: PGAgain.c  
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

#include "PGAgain.h"

#if (!CY_PSOC5A)
    #if (CYDEV_VARIABLE_VDDA == 1u)
        #include "CyScBoostClk.h"
    #endif /* (CYDEV_VARIABLE_VDDA == 1u) */
#endif /* (!CY_PSOC5A) */

#if (CY_PSOC5A)
    static PGAgain_BACKUP_STRUCT  PGAgain_P5backup;
#endif /* (CY_ PSOC5A) */

uint8 PGAgain_initVar = 0u;


/*******************************************************************************   
* Function Name: PGAgain_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  PGAgain_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void PGAgain_Init(void) 
{
    /* Set PGA mode */
    PGAgain_CR0_REG = PGAgain_MODE_PGA;      
    /* Set non-inverting PGA mode and reference mode */
    PGAgain_CR1_REG |= PGAgain_PGA_NINV;  
    /* Set default gain and ref mode */
    PGAgain_CR2_REG = PGAgain_VREF_MODE;
    /* Set gain and compensation */
    PGAgain_SetGain(PGAgain_DEFAULT_GAIN);
    /* Set power */
    PGAgain_SetPower(PGAgain_DEFAULT_POWER);
}


/*******************************************************************************   
* Function Name: PGAgain_Enable
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
void PGAgain_Enable(void) 
{
    /* This is to restore the value of register CR1 and CR2 which is saved 
      in prior to the modifications in stop() API */
    #if (CY_PSOC5A)
        if(PGAgain_P5backup.enableState == 1u)
        {
            PGAgain_CR1_REG = PGAgain_P5backup.scCR1Reg;
            PGAgain_CR2_REG = PGAgain_P5backup.scCR2Reg;
            PGAgain_P5backup.enableState = 0u;
        }
    #endif /* CY_PSOC5A */   

    /* Enable power to the Amp in Active mode*/
    PGAgain_PM_ACT_CFG_REG |= PGAgain_ACT_PWR_EN;

    /* Enable power to the Amp in Alternative Active mode*/
    PGAgain_PM_STBY_CFG_REG |= PGAgain_STBY_PWR_EN;
    
    PGAgain_PUMP_CR1_REG |= PGAgain_PUMP_CR1_SC_CLKSEL;
    
    #if (!CY_PSOC5A)
        #if (CYDEV_VARIABLE_VDDA == 1u)
            if(CyScPumpEnabled == 1u)
            {
                PGAgain_BSTCLK_REG &= (uint8)(~PGAgain_BST_CLK_INDEX_MASK);
                PGAgain_BSTCLK_REG |= PGAgain_BST_CLK_EN | CyScBoostClk__INDEX;
                PGAgain_SC_MISC_REG |= PGAgain_PUMP_FORCE;
                CyScBoostClk_Start();
            }
            else
            {
                PGAgain_BSTCLK_REG &= (uint8)(~PGAgain_BST_CLK_EN);
                PGAgain_SC_MISC_REG &= (uint8)(~PGAgain_PUMP_FORCE);
            }
        #endif /* (CYDEV_VARIABLE_VDDA == 1u) */
    #endif /* (!CY_PSOC5A) */
}


/*******************************************************************************
* Function Name: PGAgain_Start
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
void PGAgain_Start(void) 
{

    /* This is to restore the value of register CR1 and CR2 which is saved 
      in prior to the modification in stop() API */

    if(PGAgain_initVar == 0u)
    {
        PGAgain_Init();
        PGAgain_initVar = 1u;
    }

    PGAgain_Enable();
}


/*******************************************************************************
* Function Name: PGAgain_Stop
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
void PGAgain_Stop(void) 
{ 
    /* Disble power to the Amp in Active mode template */
    PGAgain_PM_ACT_CFG_REG &= (uint8)(~PGAgain_ACT_PWR_EN);

    /* Disble power to the Amp in Alternative Active mode template */
    PGAgain_PM_STBY_CFG_REG &= (uint8)(~PGAgain_STBY_PWR_EN);

    #if (!CY_PSOC5A)
        #if (CYDEV_VARIABLE_VDDA == 1u)
            PGAgain_BSTCLK_REG &= (uint8)(~PGAgain_BST_CLK_EN);
            /* Disable pumps only if there aren't any SC block in use */
            if ((PGAgain_PM_ACT_CFG_REG & PGAgain_PM_ACT_CFG_MASK) == 0u)
            {
                PGAgain_SC_MISC_REG &= (uint8)(~PGAgain_PUMP_FORCE);
                PGAgain_PUMP_CR1_REG &= (uint8)(~PGAgain_PUMP_CR1_SC_CLKSEL);
                CyScBoostClk_Stop();
            }
        #endif /* CYDEV_VARIABLE_VDDA == 1u */
    #endif /* (CY_PSOC3 || CY_PSOC5LP) */

    /* This sets PGA in zero current mode and output routes are valid */
    #if (CY_PSOC5A)
        PGAgain_P5backup.scCR1Reg = PGAgain_CR1_REG;
        PGAgain_P5backup.scCR2Reg = PGAgain_CR2_REG;
        PGAgain_CR1_REG = 0x00u;
        PGAgain_CR2_REG = 0x00u;
        PGAgain_P5backup.enableState = 1u;
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: PGAgain_SetPower
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
void PGAgain_SetPower(uint8 power) 
{
    uint8 tmpCR;

    tmpCR = PGAgain_CR1_REG & (uint8)(~PGAgain_DRIVE_MASK);
    tmpCR |= (power & PGAgain_DRIVE_MASK);
    PGAgain_CR1_REG = tmpCR;  
}


/*******************************************************************************
* Function Name: PGAgain_SetGain
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
void PGAgain_SetGain(uint8 gain) 
{
    /* Constant array for gain settings */
    const uint8 PGAgain_GainArray[9] = { 
        (PGAgain_RVAL_0K   | PGAgain_R20_40B_40K | PGAgain_BIAS_LOW), /* G=1  */
        (PGAgain_RVAL_40K  | PGAgain_R20_40B_40K | PGAgain_BIAS_LOW), /* G=2  */
        (PGAgain_RVAL_120K | PGAgain_R20_40B_40K | PGAgain_BIAS_LOW), /* G=4  */
        (PGAgain_RVAL_280K | PGAgain_R20_40B_40K | PGAgain_BIAS_LOW), /* G=8  */
        (PGAgain_RVAL_600K | PGAgain_R20_40B_40K | PGAgain_BIAS_LOW), /* G=16 */
        (PGAgain_RVAL_460K | PGAgain_R20_40B_40K | PGAgain_BIAS_LOW), /* G=24, Sets Rin as 20k */
        (PGAgain_RVAL_620K | PGAgain_R20_40B_20K | PGAgain_BIAS_LOW), /* G=32 */
        (PGAgain_RVAL_470K | PGAgain_R20_40B_20K | PGAgain_BIAS_LOW), /* G=48, Sets Rin as 10k */
        (PGAgain_RVAL_490K | PGAgain_R20_40B_20K | PGAgain_BIAS_LOW)  /* G=50, Sets Rin as 10k */
    };
    
    /* Constant array for gain compenstion settings */
    const uint8 PGAgain_GainComp[9] = { 
        ( PGAgain_COMP_4P35PF  | (uint8)( PGAgain_REDC_00 >> 2 )), /* G=1  */
        ( PGAgain_COMP_4P35PF  | (uint8)( PGAgain_REDC_01 >> 2 )), /* G=2  */
        ( PGAgain_COMP_3P0PF   | (uint8)( PGAgain_REDC_01 >> 2 )), /* G=4  */
        ( PGAgain_COMP_3P0PF   | (uint8)( PGAgain_REDC_01 >> 2 )), /* G=8  */
        ( PGAgain_COMP_3P6PF   | (uint8)( PGAgain_REDC_01 >> 2 )), /* G=16 */
        ( PGAgain_COMP_3P6PF   | (uint8)( PGAgain_REDC_11 >> 2 )), /* G=24 */
        ( PGAgain_COMP_3P6PF   | (uint8)( PGAgain_REDC_11 >> 2 )), /* G=32 */
        ( PGAgain_COMP_3P6PF   | (uint8)( PGAgain_REDC_00 >> 2 )), /* G=48 */
        ( PGAgain_COMP_3P6PF   | (uint8)( PGAgain_REDC_00 >> 2 ))  /* G=50 */
    };
    
    /* Only set new gain if it is a valid gain */
    if( gain <= PGAgain_GAIN_MAX)
    {
        /* Clear resistors, redc, and bias */
        PGAgain_CR2_REG &= (uint8)(~(PGAgain_RVAL_MASK | PGAgain_R20_40B_MASK | 
                                PGAgain_REDC_MASK | PGAgain_BIAS_MASK ));

        /* Set gain value resistors, redc comp, and bias */
        PGAgain_CR2_REG |= (PGAgain_GainArray[gain] |
                                ((uint8)(PGAgain_GainComp[gain] << 2 ) & PGAgain_REDC_MASK));

        /* Clear sc_comp  */
        PGAgain_CR1_REG &= (uint8)(~PGAgain_COMP_MASK);
        /* Set sc_comp  */
        PGAgain_CR1_REG |= ( PGAgain_GainComp[gain] | PGAgain_COMP_MASK );
    }
}


/* [] END OF FILE */
