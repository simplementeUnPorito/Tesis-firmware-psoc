/*******************************************************************************
* File Name: VDACpga.c  
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
#include "VDACpga.h"

#if (CY_PSOC5A)
#include <CyLib.h>
#endif /* CY_PSOC5A */

uint8 VDACpga_initVar = 0u;

#if (CY_PSOC5A)
    static uint8 VDACpga_restoreVal = 0u;
#endif /* CY_PSOC5A */

#if (CY_PSOC5A)
    static VDACpga_backupStruct VDACpga_backup;
#endif /* CY_PSOC5A */


/*******************************************************************************
* Function Name: VDACpga_Init
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
void VDACpga_Init(void) 
{
    VDACpga_CR0 = (VDACpga_MODE_V );

    /* Set default data source */
    #if(VDACpga_DEFAULT_DATA_SRC != 0 )
        VDACpga_CR1 = (VDACpga_DEFAULT_CNTL | VDACpga_DACBUS_ENABLE) ;
    #else
        VDACpga_CR1 = (VDACpga_DEFAULT_CNTL | VDACpga_DACBUS_DISABLE) ;
    #endif /* (VDACpga_DEFAULT_DATA_SRC != 0 ) */

    /* Set default strobe mode */
    #if(VDACpga_DEFAULT_STRB != 0)
        VDACpga_Strobe |= VDACpga_STRB_EN ;
    #endif/* (VDACpga_DEFAULT_STRB != 0) */

    /* Set default range */
    VDACpga_SetRange(VDACpga_DEFAULT_RANGE); 

    /* Set default speed */
    VDACpga_SetSpeed(VDACpga_DEFAULT_SPEED);
}


/*******************************************************************************
* Function Name: VDACpga_Enable
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
void VDACpga_Enable(void) 
{
    VDACpga_PWRMGR |= VDACpga_ACT_PWR_EN;
    VDACpga_STBY_PWRMGR |= VDACpga_STBY_PWR_EN;

    /*This is to restore the value of register CR0 ,
    which is modified  in Stop API , this prevents misbehaviour of VDAC */
    #if (CY_PSOC5A)
        if(VDACpga_restoreVal == 1u) 
        {
             VDACpga_CR0 = VDACpga_backup.data_value;
             VDACpga_restoreVal = 0u;
        }
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: VDACpga_Start
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
*  VDACpga_initVar: Is modified when this function is called for the 
*  first time. Is used to ensure that initialization happens only once.
*
*******************************************************************************/
void VDACpga_Start(void)  
{
    /* Hardware initiazation only needs to occure the first time */
    if(VDACpga_initVar == 0u)
    { 
        VDACpga_Init();
        VDACpga_initVar = 1u;
    }

    /* Enable power to DAC */
    VDACpga_Enable();

    /* Set default value */
    VDACpga_SetValue(VDACpga_DEFAULT_DATA); 
}


/*******************************************************************************
* Function Name: VDACpga_Stop
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
void VDACpga_Stop(void) 
{
    /* Disble power to DAC */
    VDACpga_PWRMGR &= (uint8)(~VDACpga_ACT_PWR_EN);
    VDACpga_STBY_PWRMGR &= (uint8)(~VDACpga_STBY_PWR_EN);

    /* This is a work around for PSoC5A  ,
    this sets VDAC to current mode with output off */
    #if (CY_PSOC5A)
        VDACpga_backup.data_value = VDACpga_CR0;
        VDACpga_CR0 = VDACpga_CUR_MODE_OUT_OFF;
        VDACpga_restoreVal = 1u;
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: VDACpga_SetSpeed
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
void VDACpga_SetSpeed(uint8 speed) 
{
    /* Clear power mask then write in new value */
    VDACpga_CR0 &= (uint8)(~VDACpga_HS_MASK);
    VDACpga_CR0 |=  (speed & VDACpga_HS_MASK);
}


/*******************************************************************************
* Function Name: VDACpga_SetRange
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
void VDACpga_SetRange(uint8 range) 
{
    VDACpga_CR0 &= (uint8)(~VDACpga_RANGE_MASK);      /* Clear existing mode */
    VDACpga_CR0 |= (range & VDACpga_RANGE_MASK);      /*  Set Range  */
    VDACpga_DacTrim();
}


/*******************************************************************************
* Function Name: VDACpga_SetValue
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
void VDACpga_SetValue(uint8 value) 
{
    #if (CY_PSOC5A)
        uint8 VDACpga_intrStatus = CyEnterCriticalSection();
    #endif /* CY_PSOC5A */

    VDACpga_Data = value;                /*  Set Value  */

    /* PSOC5A requires a double write */
    /* Exit Critical Section */
    #if (CY_PSOC5A)
        VDACpga_Data = value;
        CyExitCriticalSection(VDACpga_intrStatus);
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: VDACpga_DacTrim
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
void VDACpga_DacTrim(void) 
{
    uint8 mode;

    mode = (uint8)((VDACpga_CR0 & VDACpga_RANGE_MASK) >> 2) + VDACpga_TRIM_M7_1V_RNG_OFFSET;
    VDACpga_TR = CY_GET_XTND_REG8((uint8 *)(VDACpga_DAC_TRIM_BASE + mode));
}


/* [] END OF FILE */
