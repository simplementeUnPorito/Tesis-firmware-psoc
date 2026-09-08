/*******************************************************************************
* File Name: VDAC_ref_BP.c
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
#include "VDAC_ref_BP.h"   

#if (CY_PSOC5A)
    #include <CyLib.h>
#endif /* CY_PSOC5A */


uint8 VDAC_ref_BP_initVar = 0u;


#if (CY_PSOC5A)
    static VDAC_ref_BP_LOWPOWER_BACKUP_STRUCT  VDAC_ref_BP_lowPowerBackup;
#endif /* CY_PSOC5A */

/* Variable to decide whether or not to restore control register in Enable()
   API. This valid only for PSoC5A */
#if (CY_PSOC5A)
    static uint8 VDAC_ref_BP_restoreReg = 0u;
#endif /* CY_PSOC5A */


/*******************************************************************************
* Function Name: VDAC_ref_BP_Init
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
void VDAC_ref_BP_Init(void) 
{
    VDAC_ref_BP_CR0 = (VDAC_ref_BP_MODE_I | VDAC_ref_BP_DEFAULT_RANGE );

    /* Set default data source */
    #if(VDAC_ref_BP_DEFAULT_DATA_SRC != 0u )    
        VDAC_ref_BP_CR1 = (VDAC_ref_BP_DEFAULT_CNTL | VDAC_ref_BP_DACBUS_ENABLE);
    #else
        VDAC_ref_BP_CR1 = (VDAC_ref_BP_DEFAULT_CNTL | VDAC_ref_BP_DACBUS_DISABLE);
    #endif /* (VDAC_ref_BP_DEFAULT_DATA_SRC != 0u ) */
    
    /*Controls polarity from UDB Control*/
    #if(VDAC_ref_BP_DEFAULT_POLARITY == VDAC_ref_BP_HARDWARE_CONTROLLED)
        VDAC_ref_BP_CR1 |= VDAC_ref_BP_IDIR_SRC_UDB;
    #else
        VDAC_ref_BP_CR1 |= VDAC_ref_BP_DEFAULT_POLARITY;
    #endif/* (VDAC_ref_BP_DEFAULT_POLARITY == VDAC_ref_BP_HARDWARE_CONTROLLED) */
    /*Controls Current Source from UDB Control*/
    #if(VDAC_ref_BP_HARDWARE_ENABLE != 0u ) 
        VDAC_ref_BP_CR1 |= VDAC_ref_BP_IDIR_CTL_UDB;
    #endif /* (VDAC_ref_BP_HARDWARE_ENABLE != 0u ) */ 
    
    /* Set default strobe mode */
    #if(VDAC_ref_BP_DEFAULT_STRB != 0u)
        VDAC_ref_BP_Strobe |= VDAC_ref_BP_STRB_EN;
    #endif /* (VDAC_ref_BP_DEFAULT_STRB != 0u) */
    
    /* Set default speed */
    VDAC_ref_BP_SetSpeed(VDAC_ref_BP_DEFAULT_SPEED);
    
    /* Set proper DAC trim */
    VDAC_ref_BP_DacTrim();
    
}


/*******************************************************************************
* Function Name: VDAC_ref_BP_Enable
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
void VDAC_ref_BP_Enable(void) 
{
    VDAC_ref_BP_PWRMGR |= VDAC_ref_BP_ACT_PWR_EN;
    VDAC_ref_BP_STBY_PWRMGR |= VDAC_ref_BP_STBY_PWR_EN;

    /* This is to restore the value of register CR0 which is saved 
      in prior to the modification in stop() API */
    #if (CY_PSOC5A)
        if(VDAC_ref_BP_restoreReg == 1u)
        {
            VDAC_ref_BP_CR0 = VDAC_ref_BP_lowPowerBackup.DACCR0Reg;

            /* Clear the flag */
            VDAC_ref_BP_restoreReg = 0u;
        }
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: VDAC_ref_BP_Start
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
*  VDAC_ref_BP_initVar: Is modified when this function is called for 
*   the first time. Is used to ensure that initialization happens only once.
*
*******************************************************************************/
void VDAC_ref_BP_Start(void) 
{
    /* Hardware initiazation only needs to occur the first time */
    if ( VDAC_ref_BP_initVar == 0u)  
    {
        VDAC_ref_BP_Init();
        
        VDAC_ref_BP_initVar = 1u;
    }
   
    /* Enable power to DAC */
    VDAC_ref_BP_Enable();

    /* Set default value */
    VDAC_ref_BP_SetValue(VDAC_ref_BP_DEFAULT_DATA);

}


/*******************************************************************************
* Function Name: VDAC_ref_BP_Stop
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
void VDAC_ref_BP_Stop(void) 
{
    /* Disble power to DAC */
    VDAC_ref_BP_PWRMGR &= (uint8)(~VDAC_ref_BP_ACT_PWR_EN);
    VDAC_ref_BP_STBY_PWRMGR &= (uint8)(~VDAC_ref_BP_STBY_PWR_EN);
    
    #if (CY_PSOC5A)
    
        /* Set the global variable  */
        VDAC_ref_BP_restoreReg = 1u;

        /* Save the control register and then Clear it. */
        VDAC_ref_BP_lowPowerBackup.DACCR0Reg = VDAC_ref_BP_CR0;
        VDAC_ref_BP_CR0 = (VDAC_ref_BP_MODE_I | VDAC_ref_BP_RANGE_3 | VDAC_ref_BP_HS_HIGHSPEED);
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: VDAC_ref_BP_SetSpeed
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
void VDAC_ref_BP_SetSpeed(uint8 speed) 
{
    /* Clear power mask then write in new value */
    VDAC_ref_BP_CR0 &= (uint8)(~VDAC_ref_BP_HS_MASK);
    VDAC_ref_BP_CR0 |=  ( speed & VDAC_ref_BP_HS_MASK);
}


/*******************************************************************************
* Function Name: VDAC_ref_BP_SetPolarity
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
#if(VDAC_ref_BP_DEFAULT_POLARITY != VDAC_ref_BP_HARDWARE_CONTROLLED)
    void VDAC_ref_BP_SetPolarity(uint8 polarity) 
    {
        VDAC_ref_BP_CR1 &= (uint8)(~VDAC_ref_BP_IDIR_MASK);                /* clear polarity bit */
        VDAC_ref_BP_CR1 |= (polarity & VDAC_ref_BP_IDIR_MASK);             /* set new value */
    
        VDAC_ref_BP_DacTrim();
    }
#endif/*(VDAC_ref_BP_DEFAULT_POLARITY != VDAC_ref_BP_HARDWARE_CONTROLLED)*/


/*******************************************************************************
* Function Name: VDAC_ref_BP_SetRange
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
void VDAC_ref_BP_SetRange(uint8 range) 
{
    VDAC_ref_BP_CR0 &= (uint8)(~VDAC_ref_BP_RANGE_MASK);       /* Clear existing mode */
    VDAC_ref_BP_CR0 |= ( range & VDAC_ref_BP_RANGE_MASK );     /*  Set Range  */
    VDAC_ref_BP_DacTrim();
}


/*******************************************************************************
* Function Name: VDAC_ref_BP_SetValue
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
void VDAC_ref_BP_SetValue(uint8 value) 
{

    #if (CY_PSOC5A)
        uint8 VDAC_ref_BP_intrStatus = CyEnterCriticalSection();
    #endif /* CY_PSOC5A */

    VDAC_ref_BP_Data = value;         /*  Set Value  */
    
    /* PSOC5A silicons require a double write */
    #if (CY_PSOC5A)
        VDAC_ref_BP_Data = value;
        CyExitCriticalSection(VDAC_ref_BP_intrStatus);
    #endif /* CY_PSOC5A */
}


/*******************************************************************************
* Function Name: VDAC_ref_BP_DacTrim
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
void VDAC_ref_BP_DacTrim(void) 
{
    uint8 mode;

    mode = ((VDAC_ref_BP_CR0 & VDAC_ref_BP_RANGE_MASK) >> 1u);
    
    if((VDAC_ref_BP_IDIR_MASK & VDAC_ref_BP_CR1) == VDAC_ref_BP_IDIR_SINK)
    {
        mode++;
    }

    VDAC_ref_BP_TR = CY_GET_XTND_REG8((uint8 *)(VDAC_ref_BP_DAC_TRIM_BASE + mode));
}


/* [] END OF FILE */
