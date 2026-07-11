/*******************************************************************************
* File Name: VDAC_Ref_Adder.c  
* Version 1.90
*
* Description:
*  This file provides the source code to the API for the 8-bit Voltage DAC 
*  (VDAC8) User Module.
*
* Note:
*  Any unusual or non-standard behavior should be noted here. Other-
*  wise, this section should remain blank.
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#include "cytypes.h"
#include "VDAC_Ref_Adder.h"

#if (CY_PSOC5A)
#include <CyLib.h>
#endif /* CY_PSOC5A */

uint8 VDAC_Ref_Adder_initVar = 0u;

#if (CY_PSOC5A)
    static uint8 VDAC_Ref_Adder_restoreVal = 0u;
#endif /* CY_PSOC5A */

#if (CY_PSOC5A)
    static VDAC_Ref_Adder_backupStruct VDAC_Ref_Adder_backup;
#endif /* CY_PSOC5A */


/*******************************************************************************
* Function Name: VDAC_Ref_Adder_Init
********************************************************************************
* Summary:
*  Initialize to the schematic state.
* 
* Parameters:
*  void:
*
* Return:
*  void
*
* Theory:
*
* Side Effects:
*
*******************************************************************************/
void VDAC_Ref_Adder_Init(void) 
{
    VDAC_Ref_Adder_CR0 = (VDAC_Ref_Adder_MODE_V );

    /* Set default data source */
    #if(VDAC_Ref_Adder_DEFAULT_DATA_SRC != 0 )
        VDAC_Ref_Adder_CR1 = (VDAC_Ref_Adder_DEFAULT_CNTL | VDAC_Ref_Adder_DACBUS_ENABLE) ;
    #else
        VDAC_Ref_Adder_CR1 = (VDAC_Ref_Adder_DEFAULT_CNTL | VDAC_Ref_Adder_DACBUS_DISABLE) ;
    #endif /* (VDAC_Ref_Adder_DEFAULT_DATA_SRC != 0 ) */

    /* Set default strobe mode */
    #if(VDAC_Ref_Adder_DEFAULT_STRB != 0)
        VDAC_Ref_Adder_Strobe |= VDAC_Ref_Adder_STRB_EN ;
    #endif/* (VDAC_Ref_Adder_DEFAULT_STRB != 0) */

    /* Set default range */
    VDAC_Ref_Adder_SetRange(VDAC_Ref_Adder_DEFAULT_RANGE); 

    /* Set default speed */
    VDAC_Ref_Adder_SetSpeed(VDAC_Ref_Adder_DEFAULT_SPEED);
}


/*******************************************************************************
* Function Name: VDAC_Ref_Adder_Enable
********************************************************************************
* Summary:
*  Enable the VDAC8
* 
* Parameters:
*  void
*
* Return:
*  void
*
* Theory:
*
* Side Effects:
*
*******************************************************************************/
void VDAC_Ref_Adder_Enable(void) 
{
    VDAC_Ref_Adder_PWRMGR |= VDAC_Ref_Adder_ACT_PWR_EN;
    VDAC_Ref_Adder_STBY_PWRMGR |= VDAC_Ref_Adder_STBY_PWR_EN;

    /*This is to restore the value of register CR0 ,
    which is modified  in Stop API , this prevents misbehaviour of VDAC */
    #if (CY_PSOC5A)
        if(VDAC_Ref_Adder_restoreVal == 1u) 
        {
             VDAC_Ref_Adder_CR0 = VDAC_Ref_Adder_backup.data_value;
             VDAC_Ref_Adder_restoreVal = 0u;
        }
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: VDAC_Ref_Adder_Start
********************************************************************************
*
* Summary:
*  The start function initializes the voltage DAC with the default values, 
*  and sets the power to the given level.  A power level of 0, is the same as
*  executing the stop function.
*
* Parameters:
*  Power: Sets power level between off (0) and (3) high power
*
* Return:
*  void 
*
* Global variables:
*  VDAC_Ref_Adder_initVar: Is modified when this function is called for the 
*  first time. Is used to ensure that initialization happens only once.
*
*******************************************************************************/
void VDAC_Ref_Adder_Start(void)  
{
    /* Hardware initiazation only needs to occure the first time */
    if(VDAC_Ref_Adder_initVar == 0u)
    { 
        VDAC_Ref_Adder_Init();
        VDAC_Ref_Adder_initVar = 1u;
    }

    /* Enable power to DAC */
    VDAC_Ref_Adder_Enable();

    /* Set default value */
    VDAC_Ref_Adder_SetValue(VDAC_Ref_Adder_DEFAULT_DATA); 
}


/*******************************************************************************
* Function Name: VDAC_Ref_Adder_Stop
********************************************************************************
*
* Summary:
*  Powers down DAC to lowest power state.
*
* Parameters:
*  void
*
* Return:
*  void
*
* Theory:
*
* Side Effects:
*
*******************************************************************************/
void VDAC_Ref_Adder_Stop(void) 
{
    /* Disble power to DAC */
    VDAC_Ref_Adder_PWRMGR &= (uint8)(~VDAC_Ref_Adder_ACT_PWR_EN);
    VDAC_Ref_Adder_STBY_PWRMGR &= (uint8)(~VDAC_Ref_Adder_STBY_PWR_EN);

    /* This is a work around for PSoC5A  ,
    this sets VDAC to current mode with output off */
    #if (CY_PSOC5A)
        VDAC_Ref_Adder_backup.data_value = VDAC_Ref_Adder_CR0;
        VDAC_Ref_Adder_CR0 = VDAC_Ref_Adder_CUR_MODE_OUT_OFF;
        VDAC_Ref_Adder_restoreVal = 1u;
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: VDAC_Ref_Adder_SetSpeed
********************************************************************************
*
* Summary:
*  Set DAC speed
*
* Parameters:
*  power: Sets speed value
*
* Return:
*  void
*
* Theory:
*
* Side Effects:
*
*******************************************************************************/
void VDAC_Ref_Adder_SetSpeed(uint8 speed) 
{
    /* Clear power mask then write in new value */
    VDAC_Ref_Adder_CR0 &= (uint8)(~VDAC_Ref_Adder_HS_MASK);
    VDAC_Ref_Adder_CR0 |=  (speed & VDAC_Ref_Adder_HS_MASK);
}


/*******************************************************************************
* Function Name: VDAC_Ref_Adder_SetRange
********************************************************************************
*
* Summary:
*  Set one of three current ranges.
*
* Parameters:
*  Range: Sets one of Three valid ranges.
*
* Return:
*  void 
*
* Theory:
*
* Side Effects:
*
*******************************************************************************/
void VDAC_Ref_Adder_SetRange(uint8 range) 
{
    VDAC_Ref_Adder_CR0 &= (uint8)(~VDAC_Ref_Adder_RANGE_MASK);      /* Clear existing mode */
    VDAC_Ref_Adder_CR0 |= (range & VDAC_Ref_Adder_RANGE_MASK);      /*  Set Range  */
    VDAC_Ref_Adder_DacTrim();
}


/*******************************************************************************
* Function Name: VDAC_Ref_Adder_SetValue
********************************************************************************
*
* Summary:
*  Set 8-bit DAC value
*
* Parameters:  
*  value:  Sets DAC value between 0 and 255.
*
* Return: 
*  void 
*
* Theory: 
*
* Side Effects:
*
*******************************************************************************/
void VDAC_Ref_Adder_SetValue(uint8 value) 
{
    #if (CY_PSOC5A)
        uint8 VDAC_Ref_Adder_intrStatus = CyEnterCriticalSection();
    #endif /* CY_PSOC5A */

    VDAC_Ref_Adder_Data = value;                /*  Set Value  */

    /* PSOC5A requires a double write */
    /* Exit Critical Section */
    #if (CY_PSOC5A)
        VDAC_Ref_Adder_Data = value;
        CyExitCriticalSection(VDAC_Ref_Adder_intrStatus);
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: VDAC_Ref_Adder_DacTrim
********************************************************************************
*
* Summary:
*  Set the trim value for the given range.
*
* Parameters:
*  range:  1V or 4V range.  See constants.
*
* Return:
*  void
*
* Theory: 
*
* Side Effects:
*
*******************************************************************************/
void VDAC_Ref_Adder_DacTrim(void) 
{
    uint8 mode;

    mode = (uint8)((VDAC_Ref_Adder_CR0 & VDAC_Ref_Adder_RANGE_MASK) >> 2) + VDAC_Ref_Adder_TRIM_M7_1V_RNG_OFFSET;
    VDAC_Ref_Adder_TR = CY_GET_XTND_REG8((uint8 *)(VDAC_Ref_Adder_DAC_TRIM_BASE + mode));
}


/* [] END OF FILE */
