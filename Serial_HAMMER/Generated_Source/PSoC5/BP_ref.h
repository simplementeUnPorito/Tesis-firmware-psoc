/*******************************************************************************
* File Name: BP_ref.h  
* Version 2.20
*
* Description:
*  This file contains Pin function prototypes and register defines
*
* Note:
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_PINS_BP_ref_H) /* Pins BP_ref_H */
#define CY_PINS_BP_ref_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "BP_ref_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 BP_ref__PORT == 15 && ((BP_ref__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    BP_ref_Write(uint8 value);
void    BP_ref_SetDriveMode(uint8 mode);
uint8   BP_ref_ReadDataReg(void);
uint8   BP_ref_Read(void);
void    BP_ref_SetInterruptMode(uint16 position, uint16 mode);
uint8   BP_ref_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the BP_ref_SetDriveMode() function.
     *  @{
     */
        #define BP_ref_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define BP_ref_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define BP_ref_DM_RES_UP          PIN_DM_RES_UP
        #define BP_ref_DM_RES_DWN         PIN_DM_RES_DWN
        #define BP_ref_DM_OD_LO           PIN_DM_OD_LO
        #define BP_ref_DM_OD_HI           PIN_DM_OD_HI
        #define BP_ref_DM_STRONG          PIN_DM_STRONG
        #define BP_ref_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define BP_ref_MASK               BP_ref__MASK
#define BP_ref_SHIFT              BP_ref__SHIFT
#define BP_ref_WIDTH              1u

/* Interrupt constants */
#if defined(BP_ref__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in BP_ref_SetInterruptMode() function.
     *  @{
     */
        #define BP_ref_INTR_NONE      (uint16)(0x0000u)
        #define BP_ref_INTR_RISING    (uint16)(0x0001u)
        #define BP_ref_INTR_FALLING   (uint16)(0x0002u)
        #define BP_ref_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define BP_ref_INTR_MASK      (0x01u) 
#endif /* (BP_ref__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define BP_ref_PS                     (* (reg8 *) BP_ref__PS)
/* Data Register */
#define BP_ref_DR                     (* (reg8 *) BP_ref__DR)
/* Port Number */
#define BP_ref_PRT_NUM                (* (reg8 *) BP_ref__PRT) 
/* Connect to Analog Globals */                                                  
#define BP_ref_AG                     (* (reg8 *) BP_ref__AG)                       
/* Analog MUX bux enable */
#define BP_ref_AMUX                   (* (reg8 *) BP_ref__AMUX) 
/* Bidirectional Enable */                                                        
#define BP_ref_BIE                    (* (reg8 *) BP_ref__BIE)
/* Bit-mask for Aliased Register Access */
#define BP_ref_BIT_MASK               (* (reg8 *) BP_ref__BIT_MASK)
/* Bypass Enable */
#define BP_ref_BYP                    (* (reg8 *) BP_ref__BYP)
/* Port wide control signals */                                                   
#define BP_ref_CTL                    (* (reg8 *) BP_ref__CTL)
/* Drive Modes */
#define BP_ref_DM0                    (* (reg8 *) BP_ref__DM0) 
#define BP_ref_DM1                    (* (reg8 *) BP_ref__DM1)
#define BP_ref_DM2                    (* (reg8 *) BP_ref__DM2) 
/* Input Buffer Disable Override */
#define BP_ref_INP_DIS                (* (reg8 *) BP_ref__INP_DIS)
/* LCD Common or Segment Drive */
#define BP_ref_LCD_COM_SEG            (* (reg8 *) BP_ref__LCD_COM_SEG)
/* Enable Segment LCD */
#define BP_ref_LCD_EN                 (* (reg8 *) BP_ref__LCD_EN)
/* Slew Rate Control */
#define BP_ref_SLW                    (* (reg8 *) BP_ref__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define BP_ref_PRTDSI__CAPS_SEL       (* (reg8 *) BP_ref__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define BP_ref_PRTDSI__DBL_SYNC_IN    (* (reg8 *) BP_ref__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define BP_ref_PRTDSI__OE_SEL0        (* (reg8 *) BP_ref__PRTDSI__OE_SEL0) 
#define BP_ref_PRTDSI__OE_SEL1        (* (reg8 *) BP_ref__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define BP_ref_PRTDSI__OUT_SEL0       (* (reg8 *) BP_ref__PRTDSI__OUT_SEL0) 
#define BP_ref_PRTDSI__OUT_SEL1       (* (reg8 *) BP_ref__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define BP_ref_PRTDSI__SYNC_OUT       (* (reg8 *) BP_ref__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(BP_ref__SIO_CFG)
    #define BP_ref_SIO_HYST_EN        (* (reg8 *) BP_ref__SIO_HYST_EN)
    #define BP_ref_SIO_REG_HIFREQ     (* (reg8 *) BP_ref__SIO_REG_HIFREQ)
    #define BP_ref_SIO_CFG            (* (reg8 *) BP_ref__SIO_CFG)
    #define BP_ref_SIO_DIFF           (* (reg8 *) BP_ref__SIO_DIFF)
#endif /* (BP_ref__SIO_CFG) */

/* Interrupt Registers */
#if defined(BP_ref__INTSTAT)
    #define BP_ref_INTSTAT            (* (reg8 *) BP_ref__INTSTAT)
    #define BP_ref_SNAP               (* (reg8 *) BP_ref__SNAP)
    
	#define BP_ref_0_INTTYPE_REG 		(* (reg8 *) BP_ref__0__INTTYPE)
#endif /* (BP_ref__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_BP_ref_H */


/* [] END OF FILE */
