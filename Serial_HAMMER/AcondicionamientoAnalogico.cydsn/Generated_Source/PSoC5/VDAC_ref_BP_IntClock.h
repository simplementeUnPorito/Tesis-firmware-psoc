/*******************************************************************************
* File Name: VDAC_ref_BP_IntClock.h
* Version 2.20
*
*  Description:
*   Provides the function and constant definitions for the clock component.
*
*  Note:
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_CLOCK_VDAC_ref_BP_IntClock_H)
#define CY_CLOCK_VDAC_ref_BP_IntClock_H

#include <cytypes.h>
#include <cyfitter.h>


/***************************************
* Conditional Compilation Parameters
***************************************/

/* Check to see if required defines such as CY_PSOC5LP are available */
/* They are defined starting with cy_boot v3.0 */
#if !defined (CY_PSOC5LP)
    #error Component cy_clock_v2_20 requires cy_boot v3.0 or later
#endif /* (CY_PSOC5LP) */


/***************************************
*        Function Prototypes
***************************************/

void VDAC_ref_BP_IntClock_Start(void) ;
void VDAC_ref_BP_IntClock_Stop(void) ;

#if(CY_PSOC3 || CY_PSOC5LP)
void VDAC_ref_BP_IntClock_StopBlock(void) ;
#endif /* (CY_PSOC3 || CY_PSOC5LP) */

void VDAC_ref_BP_IntClock_StandbyPower(uint8 state) ;
void VDAC_ref_BP_IntClock_SetDividerRegister(uint16 clkDivider, uint8 restart) 
                                ;
uint16 VDAC_ref_BP_IntClock_GetDividerRegister(void) ;
void VDAC_ref_BP_IntClock_SetModeRegister(uint8 modeBitMask) ;
void VDAC_ref_BP_IntClock_ClearModeRegister(uint8 modeBitMask) ;
uint8 VDAC_ref_BP_IntClock_GetModeRegister(void) ;
void VDAC_ref_BP_IntClock_SetSourceRegister(uint8 clkSource) ;
uint8 VDAC_ref_BP_IntClock_GetSourceRegister(void) ;
#if defined(VDAC_ref_BP_IntClock__CFG3)
void VDAC_ref_BP_IntClock_SetPhaseRegister(uint8 clkPhase) ;
uint8 VDAC_ref_BP_IntClock_GetPhaseRegister(void) ;
#endif /* defined(VDAC_ref_BP_IntClock__CFG3) */

#define VDAC_ref_BP_IntClock_Enable()                       VDAC_ref_BP_IntClock_Start()
#define VDAC_ref_BP_IntClock_Disable()                      VDAC_ref_BP_IntClock_Stop()
#define VDAC_ref_BP_IntClock_SetDivider(clkDivider)         VDAC_ref_BP_IntClock_SetDividerRegister(clkDivider, 1u)
#define VDAC_ref_BP_IntClock_SetDividerValue(clkDivider)    VDAC_ref_BP_IntClock_SetDividerRegister((clkDivider) - 1u, 1u)
#define VDAC_ref_BP_IntClock_SetMode(clkMode)               VDAC_ref_BP_IntClock_SetModeRegister(clkMode)
#define VDAC_ref_BP_IntClock_SetSource(clkSource)           VDAC_ref_BP_IntClock_SetSourceRegister(clkSource)
#if defined(VDAC_ref_BP_IntClock__CFG3)
#define VDAC_ref_BP_IntClock_SetPhase(clkPhase)             VDAC_ref_BP_IntClock_SetPhaseRegister(clkPhase)
#define VDAC_ref_BP_IntClock_SetPhaseValue(clkPhase)        VDAC_ref_BP_IntClock_SetPhaseRegister((clkPhase) + 1u)
#endif /* defined(VDAC_ref_BP_IntClock__CFG3) */


/***************************************
*             Registers
***************************************/

/* Register to enable or disable the clock */
#define VDAC_ref_BP_IntClock_CLKEN              (* (reg8 *) VDAC_ref_BP_IntClock__PM_ACT_CFG)
#define VDAC_ref_BP_IntClock_CLKEN_PTR          ((reg8 *) VDAC_ref_BP_IntClock__PM_ACT_CFG)

/* Register to enable or disable the clock */
#define VDAC_ref_BP_IntClock_CLKSTBY            (* (reg8 *) VDAC_ref_BP_IntClock__PM_STBY_CFG)
#define VDAC_ref_BP_IntClock_CLKSTBY_PTR        ((reg8 *) VDAC_ref_BP_IntClock__PM_STBY_CFG)

/* Clock LSB divider configuration register. */
#define VDAC_ref_BP_IntClock_DIV_LSB            (* (reg8 *) VDAC_ref_BP_IntClock__CFG0)
#define VDAC_ref_BP_IntClock_DIV_LSB_PTR        ((reg8 *) VDAC_ref_BP_IntClock__CFG0)
#define VDAC_ref_BP_IntClock_DIV_PTR            ((reg16 *) VDAC_ref_BP_IntClock__CFG0)

/* Clock MSB divider configuration register. */
#define VDAC_ref_BP_IntClock_DIV_MSB            (* (reg8 *) VDAC_ref_BP_IntClock__CFG1)
#define VDAC_ref_BP_IntClock_DIV_MSB_PTR        ((reg8 *) VDAC_ref_BP_IntClock__CFG1)

/* Mode and source configuration register */
#define VDAC_ref_BP_IntClock_MOD_SRC            (* (reg8 *) VDAC_ref_BP_IntClock__CFG2)
#define VDAC_ref_BP_IntClock_MOD_SRC_PTR        ((reg8 *) VDAC_ref_BP_IntClock__CFG2)

#if defined(VDAC_ref_BP_IntClock__CFG3)
/* Analog clock phase configuration register */
#define VDAC_ref_BP_IntClock_PHASE              (* (reg8 *) VDAC_ref_BP_IntClock__CFG3)
#define VDAC_ref_BP_IntClock_PHASE_PTR          ((reg8 *) VDAC_ref_BP_IntClock__CFG3)
#endif /* defined(VDAC_ref_BP_IntClock__CFG3) */


/**************************************
*       Register Constants
**************************************/

/* Power manager register masks */
#define VDAC_ref_BP_IntClock_CLKEN_MASK         VDAC_ref_BP_IntClock__PM_ACT_MSK
#define VDAC_ref_BP_IntClock_CLKSTBY_MASK       VDAC_ref_BP_IntClock__PM_STBY_MSK

/* CFG2 field masks */
#define VDAC_ref_BP_IntClock_SRC_SEL_MSK        VDAC_ref_BP_IntClock__CFG2_SRC_SEL_MASK
#define VDAC_ref_BP_IntClock_MODE_MASK          (~(VDAC_ref_BP_IntClock_SRC_SEL_MSK))

#if defined(VDAC_ref_BP_IntClock__CFG3)
/* CFG3 phase mask */
#define VDAC_ref_BP_IntClock_PHASE_MASK         VDAC_ref_BP_IntClock__CFG3_PHASE_DLY_MASK
#endif /* defined(VDAC_ref_BP_IntClock__CFG3) */

#endif /* CY_CLOCK_VDAC_ref_BP_IntClock_H */


/* [] END OF FILE */
