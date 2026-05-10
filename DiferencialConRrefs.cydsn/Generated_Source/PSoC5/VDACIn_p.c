/*******************************************************************************
* File Name: VDACIn_p.c  
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
#include "VDACIn_p.h"

#if (CY_PSOC5A)
#include <CyLib.h>
#endif /* CY_PSOC5A */

uint8 VDACIn_p_initVar = 0u;

#if (CY_PSOC5A)
    static uint8 VDACIn_p_restoreVal = 0u;
#endif /* CY_PSOC5A */

#if (CY_PSOC5A)
    static VDACIn_p_backupStruct VDACIn_p_backup;
#endif /* CY_PSOC5A */


/*******************************************************************************
* Function Name: VDACIn_p_Init
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
void VDACIn_p_Init(void) 
{
    VDACIn_p_CR0 = (VDACIn_p_MODE_V );

    /* Set default data source */
    #if(VDACIn_p_DEFAULT_DATA_SRC != 0 )
        VDACIn_p_CR1 = (VDACIn_p_DEFAULT_CNTL | VDACIn_p_DACBUS_ENABLE) ;
    #else
        VDACIn_p_CR1 = (VDACIn_p_DEFAULT_CNTL | VDACIn_p_DACBUS_DISABLE) ;
    #endif /* (VDACIn_p_DEFAULT_DATA_SRC != 0 ) */

    /* Set default strobe mode */
    #if(VDACIn_p_DEFAULT_STRB != 0)
        VDACIn_p_Strobe |= VDACIn_p_STRB_EN ;
    #endif/* (VDACIn_p_DEFAULT_STRB != 0) */

    /* Set default range */
    VDACIn_p_SetRange(VDACIn_p_DEFAULT_RANGE); 

    /* Set default speed */
    VDACIn_p_SetSpeed(VDACIn_p_DEFAULT_SPEED);
}


/*******************************************************************************
* Function Name: VDACIn_p_Enable
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
void VDACIn_p_Enable(void) 
{
    VDACIn_p_PWRMGR |= VDACIn_p_ACT_PWR_EN;
    VDACIn_p_STBY_PWRMGR |= VDACIn_p_STBY_PWR_EN;

    /*This is to restore the value of register CR0 ,
    which is modified  in Stop API , this prevents misbehaviour of VDAC */
    #if (CY_PSOC5A)
        if(VDACIn_p_restoreVal == 1u) 
        {
             VDACIn_p_CR0 = VDACIn_p_backup.data_value;
             VDACIn_p_restoreVal = 0u;
        }
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: VDACIn_p_Start
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
*  VDACIn_p_initVar: Is modified when this function is called for the 
*  first time. Is used to ensure that initialization happens only once.
*
*******************************************************************************/
void VDACIn_p_Start(void)  
{
    /* Hardware initiazation only needs to occure the first time */
    if(VDACIn_p_initVar == 0u)
    { 
        VDACIn_p_Init();
        VDACIn_p_initVar = 1u;
    }

    /* Enable power to DAC */
    VDACIn_p_Enable();

    /* Set default value */
    VDACIn_p_SetValue(VDACIn_p_DEFAULT_DATA); 
}


/*******************************************************************************
* Function Name: VDACIn_p_Stop
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
void VDACIn_p_Stop(void) 
{
    /* Disble power to DAC */
    VDACIn_p_PWRMGR &= (uint8)(~VDACIn_p_ACT_PWR_EN);
    VDACIn_p_STBY_PWRMGR &= (uint8)(~VDACIn_p_STBY_PWR_EN);

    /* This is a work around for PSoC5A  ,
    this sets VDAC to current mode with output off */
    #if (CY_PSOC5A)
        VDACIn_p_backup.data_value = VDACIn_p_CR0;
        VDACIn_p_CR0 = VDACIn_p_CUR_MODE_OUT_OFF;
        VDACIn_p_restoreVal = 1u;
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: VDACIn_p_SetSpeed
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
void VDACIn_p_SetSpeed(uint8 speed) 
{
    /* Clear power mask then write in new value */
    VDACIn_p_CR0 &= (uint8)(~VDACIn_p_HS_MASK);
    VDACIn_p_CR0 |=  (speed & VDACIn_p_HS_MASK);
}


/*******************************************************************************
* Function Name: VDACIn_p_SetRange
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
void VDACIn_p_SetRange(uint8 range) 
{
    VDACIn_p_CR0 &= (uint8)(~VDACIn_p_RANGE_MASK);      /* Clear existing mode */
    VDACIn_p_CR0 |= (range & VDACIn_p_RANGE_MASK);      /*  Set Range  */
    VDACIn_p_DacTrim();
}


/*******************************************************************************
* Function Name: VDACIn_p_SetValue
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
void VDACIn_p_SetValue(uint8 value) 
{
    #if (CY_PSOC5A)
        uint8 VDACIn_p_intrStatus = CyEnterCriticalSection();
    #endif /* CY_PSOC5A */

    VDACIn_p_Data = value;                /*  Set Value  */

    /* PSOC5A requires a double write */
    /* Exit Critical Section */
    #if (CY_PSOC5A)
        VDACIn_p_Data = value;
        CyExitCriticalSection(VDACIn_p_intrStatus);
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: VDACIn_p_DacTrim
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
void VDACIn_p_DacTrim(void) 
{
    uint8 mode;

    mode = (uint8)((VDACIn_p_CR0 & VDACIn_p_RANGE_MASK) >> 2) + VDACIn_p_TRIM_M7_1V_RNG_OFFSET;
    VDACIn_p_TR = CY_GET_XTND_REG8((uint8 *)(VDACIn_p_DAC_TRIM_BASE + mode));
}


/* [] END OF FILE */
