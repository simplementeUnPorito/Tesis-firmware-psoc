/*******************************************************************************
* File Name: TIA_n.c  
* Version 2.0
*
* Description:
*  This file provides the source code to the API for the TIA User Module.
*
* Note:
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#include "TIA_n.h"

#if (!CY_PSOC5A)
    #if (CYDEV_VARIABLE_VDDA == 1u)
        #include "CyScBoostClk.h"
    #endif /* (CYDEV_VARIABLE_VDDA == 1u) */
#endif /* (!CY_PSOC5A) */

/* Fixed configuration of SC Block only performed once */
uint8 TIA_n_initVar = 0u;

#if (CY_PSOC5A)
    static TIA_n_BACKUP_STRUCT  TIA_n_P5backup;
#endif /* (CY_PSOC5A) */


/*******************************************************************************   
* Function Name: TIA_n_Init
********************************************************************************
*
* Summary:
*  Initialize component's parameters to the parameters set by user in the 
*  customizer of the component placed onto schematic. Usually called in 
*  TIA_n_Start().
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void TIA_n_Init(void) 
{
    /* Set SC Block Mode to TIA - SCxx_CR0[3:1] */
    TIA_n_CR0_REG = TIA_n_MODE_TIA;
    
    /*    Set SC Block CR1 Control Register fields to support TIA mode 
            drive field set by SetPower() API
            compensation Cap - 5.1 pF
            div2 - disable, n/a for TIA mode
            Mixer/PGA Gain - 0db, n/a for TIA mode   */

    TIA_n_CR1_REG = (TIA_n_COMP_5P1PF |
                                TIA_n_DIV2_DISABLE |
                                TIA_n_GAIN_0DB);


    /*    Set SC Block CR2 Control Register fields to support TIA mode
            Bias - Low
            r20_r40 - 40K, n/a for TIA mode
            redc - set by SetCapFB() API
            rval - set by SetResFB() API
            PGA Ground Ref - disable, n/a for TIA mode    */

    TIA_n_CR2_REG = (TIA_n_BIAS_LOW |
                                TIA_n_R20_40B_40K |
                                TIA_n_GNDVREF_DI);
                          
    /* Set default resistive feedback value */
    TIA_n_SetResFB(TIA_n_INIT_RES_FEEDBACK);
      
    /* Set default capacitive feedback value */
    TIA_n_SetCapFB(TIA_n_INIT_CAP_FEEDBACK);

    /* Set default power */
    TIA_n_SetPower(TIA_n_INIT_POWER);
}


/*******************************************************************************   
* Function Name: TIA_n_Enable
********************************************************************************
*
* Summary:
*  Enables the TIA block operation
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void TIA_n_Enable(void) 
{
	/* This is to restore the value of register CR1 and CR2 which is saved 
      in prior to the modifications in stop() API */
	#if (CY_PSOC5A)
        if(TIA_n_P5backup.enableState == 1u)
        {
			TIA_n_CR1_REG = TIA_n_P5backup.scCR1Reg;
            TIA_n_CR2_REG =TIA_n_P5backup.scCR2Reg;
            TIA_n_P5backup.enableState = 0u;
        }
    #endif
    /* Enable power to Amp in Active mode */
    TIA_n_PM_ACT_CFG_REG |= TIA_n_ACT_PWR_EN;

    /* Enable the power to Amp in Alternative active mode*/
    TIA_n_PM_STBY_CFG_REG |= TIA_n_STBY_PWR_EN;
    
    TIA_n_PUMP_CR1_REG |= TIA_n_PUMP_CR1_SC_CLKSEL;
    
    #if (!CY_PSOC5A)
        #if (CYDEV_VARIABLE_VDDA == 1u)
            if(CyScPumpEnabled == 1u)
            {
                TIA_n_BSTCLK_REG &= (uint8)(~TIA_n_BST_CLK_INDEX_MASK);
                TIA_n_BSTCLK_REG |= TIA_n_BST_CLK_EN | CyScBoostClk__INDEX;
                TIA_n_SC_MISC_REG |= TIA_n_PUMP_FORCE;
                CyScBoostClk_Start();
            }
            else
            {
                TIA_n_BSTCLK_REG &= (uint8)(~TIA_n_BST_CLK_EN);
                TIA_n_SC_MISC_REG &= (uint8)(~TIA_n_PUMP_FORCE);
            }
        #endif /* (CYDEV_VARIABLE_VDDA == 1u) */
    #endif /* (!CY_PSOC5A) */
}


/*******************************************************************************
* Function Name: TIA_n_Start
********************************************************************************
*
* Summary:
*  The start function initializes the TIA with the default values and enables
*  power to the SC block.
*
* Parameters:
*  void
*
* Return:
*  void 
*
* Global variables:
*  TIA_n_initVar: Used to check the initial configuration, modified 
*  when this function is called for the first time.
*
* Theory:
*  Full initialization of the SC Block configuration registers is only perfomed
*  the first time the routine executes - global variable  is used to
*  TIA_n_initVar is used to indicate that the static configuration 
*  has been completed.
*
*******************************************************************************/
void TIA_n_Start(void) 
{
    if(TIA_n_initVar == 0u)
    {
        TIA_n_initVar = 1u;  
        TIA_n_Init();
    }

    TIA_n_Enable();
}


/*******************************************************************************
* Function Name: TIA_n_Stop
********************************************************************************
*
* Summary: 
*  Disables power to SC block.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void TIA_n_Stop(void) 
{
     /* Disble power to the Amp in Active mode template*/
    TIA_n_PM_ACT_CFG_REG &= (uint8)(~TIA_n_ACT_PWR_EN);

    /* Disble power to the Amp in Alternative active mode template*/
    TIA_n_PM_STBY_CFG_REG &= (uint8)(~TIA_n_STBY_PWR_EN);
    
    #if (!CY_PSOC5A)
        #if (CYDEV_VARIABLE_VDDA == 1u)
            TIA_n_BSTCLK_REG &= (uint8)(~TIA_n_BST_CLK_EN);
            /* Disable pumps only if there aren't any SC block in use */
            if ((TIA_n_PM_ACT_CFG_REG & TIA_n_PM_ACT_CFG_MASK) == 0u)
            {
                TIA_n_SC_MISC_REG &= (uint8)(~TIA_n_PUMP_FORCE);
                TIA_n_PUMP_CR1_REG &= (uint8)(~TIA_n_PUMP_CR1_SC_CLKSEL);
                CyScBoostClk_Stop();
            }
        #endif /* CYDEV_VARIABLE_VDDA == 1u */
    #endif /* (CY_PSOC3 || CY_PSOC5LP) */
        
    /* This sets TIA in zero current mode and output routes are valid */
    #if (CY_PSOC5A)
        TIA_n_P5backup.scCR1Reg = TIA_n_CR1_REG;
        TIA_n_P5backup.scCR2Reg = TIA_n_CR2_REG;
        TIA_n_CR1_REG = 0x00u;
        TIA_n_CR2_REG = 0x00u;
        TIA_n_P5backup.enableState = 1u;
    #endif /* (CY_PSOC5A) */
}


/*******************************************************************************
* Function Name: TIA_n_SetPower
********************************************************************************
*
* Summary:
*  Set the drive power of the TIA.
*
* Parameters:
*  Power: Sets power level between (0) and (3) high power.
*
* Return:
*  void
*
*******************************************************************************/
void TIA_n_SetPower(uint8 power) 
{
    uint8 tmpCR;

    /* Sets drive bits in SC Block Control Register 1:  SCxx_CR[1:0] */
    tmpCR = TIA_n_CR1_REG & (uint8)(~TIA_n_DRIVE_MASK);
    tmpCR |= (power & TIA_n_DRIVE_MASK);
    TIA_n_CR1_REG = tmpCR;  
}


/*******************************************************************************
* Function Name: TIA_n_SetResFB
********************************************************************************
*
* Summary:
*  This function sets value for the resistive feedback of the amplifier.
*
* Parameters:
*  res_feedback: specifies resistive feedback value (see header file for gain 
*                values.)
*
* Return:
*  void
*
*******************************************************************************/
void TIA_n_SetResFB(uint8 res_feedback) 
{
    /* Only set new gain if it is a valid gain */
    if( res_feedback <= TIA_n_RES_FEEDBACK_MAX)
    {
        /* Clear SCxx_CR2 rval bits - SCxx_CR2[6:4] */
        TIA_n_CR2_REG &= (uint8)(~TIA_n_RVAL_MASK);

        /* Set resistive feedback value */
        TIA_n_CR2_REG |= (uint8)(res_feedback << 4);
    }
}


/*******************************************************************************
* Function Name: TIA_n_SetCapFB
********************************************************************************
*
* Summary:
*  This function sets the value for the capacitive feedback of the amplifier.
*
* Parameters:
*  cap_feedback: specifies capacitive feedback value (see header file for gain 
*                values.)
*
* Return:
*  void
*
*******************************************************************************/
void TIA_n_SetCapFB(uint8 cap_feedback) 
{
    /* Only set new gain if it is a valid gain */
    if( cap_feedback <= TIA_n_CAP_FEEDBACK_MAX)
    {
        /* Clear SCxx_CR2 redc bits -  - CR2[3:2] */
        TIA_n_CR2_REG &= (uint8)(~TIA_n_REDC_MASK);

        /* Set redc capacitive feedback value */
        TIA_n_CR2_REG |= (uint8)(cap_feedback << 2);
    }
}


/* [] END OF FILE */
