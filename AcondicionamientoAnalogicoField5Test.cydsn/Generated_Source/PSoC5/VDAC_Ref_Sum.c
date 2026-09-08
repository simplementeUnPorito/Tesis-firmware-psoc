/*******************************************************************************
* File Name: VDAC_Ref_Sum.c
* Version 2.0
*
* Description:
*  This file provides the source code to the API for the 8-bit Current 
*  DAC (IDAC8) User Module.
*
* Note:
*  None
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#include "cytypes.h"
#include "VDAC_Ref_Sum.h"   

#if (CY_PSOC5A)
    #include <CyLib.h>
#endif /* CY_PSOC5A */


uint8 VDAC_Ref_Sum_initVar = 0u;


#if (CY_PSOC5A)
    static VDAC_Ref_Sum_LOWPOWER_BACKUP_STRUCT  VDAC_Ref_Sum_lowPowerBackup;
#endif /* CY_PSOC5A */

/* Variable to decide whether or not to restore control register in Enable()
   API. This valid only for PSoC5A */
#if (CY_PSOC5A)
    static uint8 VDAC_Ref_Sum_restoreReg = 0u;
#endif /* CY_PSOC5A */


/*******************************************************************************
* Function Name: VDAC_Ref_Sum_Init
********************************************************************************
* Summary:
*  Initialize to the schematic state.
* 
* Parameters:
*  void:
*
* Return:
*  (void)
*
* Theory:
*
* Side Effects:
*
*******************************************************************************/
void VDAC_Ref_Sum_Init(void) 
{
    VDAC_Ref_Sum_CR0 = (VDAC_Ref_Sum_MODE_I | VDAC_Ref_Sum_DEFAULT_RANGE );

    /* Set default data source */
    #if(VDAC_Ref_Sum_DEFAULT_DATA_SRC != 0u )    
        VDAC_Ref_Sum_CR1 = (VDAC_Ref_Sum_DEFAULT_CNTL | VDAC_Ref_Sum_DACBUS_ENABLE);
    #else
        VDAC_Ref_Sum_CR1 = (VDAC_Ref_Sum_DEFAULT_CNTL | VDAC_Ref_Sum_DACBUS_DISABLE);
    #endif /* (VDAC_Ref_Sum_DEFAULT_DATA_SRC != 0u ) */
    
    /*Controls polarity from UDB Control*/
    #if(VDAC_Ref_Sum_DEFAULT_POLARITY == VDAC_Ref_Sum_HARDWARE_CONTROLLED)
        VDAC_Ref_Sum_CR1 |= VDAC_Ref_Sum_IDIR_SRC_UDB;
    #else
        VDAC_Ref_Sum_CR1 |= VDAC_Ref_Sum_DEFAULT_POLARITY;
    #endif/* (VDAC_Ref_Sum_DEFAULT_POLARITY == VDAC_Ref_Sum_HARDWARE_CONTROLLED) */
    /*Controls Current Source from UDB Control*/
    #if(VDAC_Ref_Sum_HARDWARE_ENABLE != 0u ) 
        VDAC_Ref_Sum_CR1 |= VDAC_Ref_Sum_IDIR_CTL_UDB;
    #endif /* (VDAC_Ref_Sum_HARDWARE_ENABLE != 0u ) */ 
    
    /* Set default strobe mode */
    #if(VDAC_Ref_Sum_DEFAULT_STRB != 0u)
        VDAC_Ref_Sum_Strobe |= VDAC_Ref_Sum_STRB_EN;
    #endif /* (VDAC_Ref_Sum_DEFAULT_STRB != 0u) */
    
    /* Set default speed */
    VDAC_Ref_Sum_SetSpeed(VDAC_Ref_Sum_DEFAULT_SPEED);
    
    /* Set proper DAC trim */
    VDAC_Ref_Sum_DacTrim();
    
}


/*******************************************************************************
* Function Name: VDAC_Ref_Sum_Enable
********************************************************************************
* Summary:
*  Enable the IDAC8
* 
* Parameters:
*  void:
*
* Return:
*  (void)
*
* Theory:
*
* Side Effects:
*
*******************************************************************************/
void VDAC_Ref_Sum_Enable(void) 
{
    VDAC_Ref_Sum_PWRMGR |= VDAC_Ref_Sum_ACT_PWR_EN;
    VDAC_Ref_Sum_STBY_PWRMGR |= VDAC_Ref_Sum_STBY_PWR_EN;

    /* This is to restore the value of register CR0 which is saved 
      in prior to the modification in stop() API */
    #if (CY_PSOC5A)
        if(VDAC_Ref_Sum_restoreReg == 1u)
        {
            VDAC_Ref_Sum_CR0 = VDAC_Ref_Sum_lowPowerBackup.DACCR0Reg;

            /* Clear the flag */
            VDAC_Ref_Sum_restoreReg = 0u;
        }
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: VDAC_Ref_Sum_Start
********************************************************************************
* Summary:
*  Set power level then turn on IDAC8.
*
* Parameters:  
*  power: Sets power level between off (0) and (3) high power
*
* Return:
*  (void)
*
* Global variables:
*  VDAC_Ref_Sum_initVar: Is modified when this function is called for 
*   the first time. Is used to ensure that initialization happens only once.
*
*******************************************************************************/
void VDAC_Ref_Sum_Start(void) 
{
    /* Hardware initiazation only needs to occur the first time */
    if ( VDAC_Ref_Sum_initVar == 0u)  
    {
        VDAC_Ref_Sum_Init();
        
        VDAC_Ref_Sum_initVar = 1u;
    }
   
    /* Enable power to DAC */
    VDAC_Ref_Sum_Enable();

    /* Set default value */
    VDAC_Ref_Sum_SetValue(VDAC_Ref_Sum_DEFAULT_DATA);

}


/*******************************************************************************
* Function Name: VDAC_Ref_Sum_Stop
********************************************************************************
* Summary:
*  Powers down IDAC8 to lowest power state.
*
* Parameters:
*  (void)
*
* Return:
*  (void)
*
* Theory:
*
* Side Effects:
*
*******************************************************************************/
void VDAC_Ref_Sum_Stop(void) 
{
    /* Disble power to DAC */
    VDAC_Ref_Sum_PWRMGR &= (uint8)(~VDAC_Ref_Sum_ACT_PWR_EN);
    VDAC_Ref_Sum_STBY_PWRMGR &= (uint8)(~VDAC_Ref_Sum_STBY_PWR_EN);
    
    #if (CY_PSOC5A)
    
        /* Set the global variable  */
        VDAC_Ref_Sum_restoreReg = 1u;

        /* Save the control register and then Clear it. */
        VDAC_Ref_Sum_lowPowerBackup.DACCR0Reg = VDAC_Ref_Sum_CR0;
        VDAC_Ref_Sum_CR0 = (VDAC_Ref_Sum_MODE_I | VDAC_Ref_Sum_RANGE_3 | VDAC_Ref_Sum_HS_HIGHSPEED);
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: VDAC_Ref_Sum_SetSpeed
********************************************************************************
* Summary:
*  Set DAC speed
*
* Parameters:
*  power: Sets speed value
*
* Return:
*  (void)
*
* Theory:
*
* Side Effects:
*
*******************************************************************************/
void VDAC_Ref_Sum_SetSpeed(uint8 speed) 
{
    /* Clear power mask then write in new value */
    VDAC_Ref_Sum_CR0 &= (uint8)(~VDAC_Ref_Sum_HS_MASK);
    VDAC_Ref_Sum_CR0 |=  ( speed & VDAC_Ref_Sum_HS_MASK);
}


/*******************************************************************************
* Function Name: VDAC_Ref_Sum_SetPolarity
********************************************************************************
* Summary:
*  Sets IDAC to Sink or Source current.
*  
* Parameters:
*  Polarity: Sets the IDAC to Sink or Source 
*  0x00 - Source
*  0x04 - Sink
*
* Return:
*  (void)
*
* Theory:
*
* Side Effects:
*
*******************************************************************************/
#if(VDAC_Ref_Sum_DEFAULT_POLARITY != VDAC_Ref_Sum_HARDWARE_CONTROLLED)
    void VDAC_Ref_Sum_SetPolarity(uint8 polarity) 
    {
        VDAC_Ref_Sum_CR1 &= (uint8)(~VDAC_Ref_Sum_IDIR_MASK);                /* clear polarity bit */
        VDAC_Ref_Sum_CR1 |= (polarity & VDAC_Ref_Sum_IDIR_MASK);             /* set new value */
    
        VDAC_Ref_Sum_DacTrim();
    }
#endif/*(VDAC_Ref_Sum_DEFAULT_POLARITY != VDAC_Ref_Sum_HARDWARE_CONTROLLED)*/


/*******************************************************************************
* Function Name: VDAC_Ref_Sum_SetRange
********************************************************************************
* Summary:
*  Set current range
*
* Parameters:
*  Range: Sets on of four valid ranges.
*
* Return:
*  (void)
*
* Theory:
*
* Side Effects:
*
*******************************************************************************/
void VDAC_Ref_Sum_SetRange(uint8 range) 
{
    VDAC_Ref_Sum_CR0 &= (uint8)(~VDAC_Ref_Sum_RANGE_MASK);       /* Clear existing mode */
    VDAC_Ref_Sum_CR0 |= ( range & VDAC_Ref_Sum_RANGE_MASK );     /*  Set Range  */
    VDAC_Ref_Sum_DacTrim();
}


/*******************************************************************************
* Function Name: VDAC_Ref_Sum_SetValue
********************************************************************************
* Summary:
*  Set DAC value
*
* Parameters:
*  value: Sets DAC value between 0 and 255.
*
* Return:
*  (void)
*
* Theory:
*
* Side Effects:
*
*******************************************************************************/
void VDAC_Ref_Sum_SetValue(uint8 value) 
{

    #if (CY_PSOC5A)
        uint8 VDAC_Ref_Sum_intrStatus = CyEnterCriticalSection();
    #endif /* CY_PSOC5A */

    VDAC_Ref_Sum_Data = value;         /*  Set Value  */
    
    /* PSOC5A silicons require a double write */
    #if (CY_PSOC5A)
        VDAC_Ref_Sum_Data = value;
        CyExitCriticalSection(VDAC_Ref_Sum_intrStatus);
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: VDAC_Ref_Sum_DacTrim
********************************************************************************
* Summary:
*  Set the trim value for the given range.
*
* Parameters:
*  None
*
* Return:
*  (void) 
*
* Theory:
*  Trim values for the IDAC blocks are stored in the "Customer Table" area in 
*  Row 1 of the Hidden Flash.  There are 8 bytes of trim data for each 
*  IDAC block.
*  The values are:
*       I Gain offset, min range, Sourcing
*       I Gain offset, min range, Sinking
*       I Gain offset, med range, Sourcing
*       I Gain offset, med range, Sinking
*       I Gain offset, max range, Sourcing
*       I Gain offset, max range, Sinking
*       V Gain offset, 1V range
*       V Gain offset, 4V range
*
* Side Effects:
*
*******************************************************************************/
void VDAC_Ref_Sum_DacTrim(void) 
{
    uint8 mode;

    mode = ((VDAC_Ref_Sum_CR0 & VDAC_Ref_Sum_RANGE_MASK) >> 1u);
    
    if((VDAC_Ref_Sum_IDIR_MASK & VDAC_Ref_Sum_CR1) == VDAC_Ref_Sum_IDIR_SINK)
    {
        mode++;
    }

    VDAC_Ref_Sum_TR = CY_GET_XTND_REG8((uint8 *)(VDAC_Ref_Sum_DAC_TRIM_BASE + mode));
}


/* [] END OF FILE */
