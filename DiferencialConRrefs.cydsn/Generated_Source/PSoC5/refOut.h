/*******************************************************************************
* File Name: refOut.h  
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

#if !defined(CY_PINS_refOut_H) /* Pins refOut_H */
#define CY_PINS_refOut_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "refOut_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 refOut__PORT == 15 && ((refOut__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    refOut_Write(uint8 value);
void    refOut_SetDriveMode(uint8 mode);
uint8   refOut_ReadDataReg(void);
uint8   refOut_Read(void);
void    refOut_SetInterruptMode(uint16 position, uint16 mode);
uint8   refOut_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the refOut_SetDriveMode() function.
     *  @{
     */
        #define refOut_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define refOut_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define refOut_DM_RES_UP          PIN_DM_RES_UP
        #define refOut_DM_RES_DWN         PIN_DM_RES_DWN
        #define refOut_DM_OD_LO           PIN_DM_OD_LO
        #define refOut_DM_OD_HI           PIN_DM_OD_HI
        #define refOut_DM_STRONG          PIN_DM_STRONG
        #define refOut_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define refOut_MASK               refOut__MASK
#define refOut_SHIFT              refOut__SHIFT
#define refOut_WIDTH              1u

/* Interrupt constants */
#if defined(refOut__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in refOut_SetInterruptMode() function.
     *  @{
     */
        #define refOut_INTR_NONE      (uint16)(0x0000u)
        #define refOut_INTR_RISING    (uint16)(0x0001u)
        #define refOut_INTR_FALLING   (uint16)(0x0002u)
        #define refOut_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define refOut_INTR_MASK      (0x01u) 
#endif /* (refOut__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define refOut_PS                     (* (reg8 *) refOut__PS)
/* Data Register */
#define refOut_DR                     (* (reg8 *) refOut__DR)
/* Port Number */
#define refOut_PRT_NUM                (* (reg8 *) refOut__PRT) 
/* Connect to Analog Globals */                                                  
#define refOut_AG                     (* (reg8 *) refOut__AG)                       
/* Analog MUX bux enable */
#define refOut_AMUX                   (* (reg8 *) refOut__AMUX) 
/* Bidirectional Enable */                                                        
#define refOut_BIE                    (* (reg8 *) refOut__BIE)
/* Bit-mask for Aliased Register Access */
#define refOut_BIT_MASK               (* (reg8 *) refOut__BIT_MASK)
/* Bypass Enable */
#define refOut_BYP                    (* (reg8 *) refOut__BYP)
/* Port wide control signals */                                                   
#define refOut_CTL                    (* (reg8 *) refOut__CTL)
/* Drive Modes */
#define refOut_DM0                    (* (reg8 *) refOut__DM0) 
#define refOut_DM1                    (* (reg8 *) refOut__DM1)
#define refOut_DM2                    (* (reg8 *) refOut__DM2) 
/* Input Buffer Disable Override */
#define refOut_INP_DIS                (* (reg8 *) refOut__INP_DIS)
/* LCD Common or Segment Drive */
#define refOut_LCD_COM_SEG            (* (reg8 *) refOut__LCD_COM_SEG)
/* Enable Segment LCD */
#define refOut_LCD_EN                 (* (reg8 *) refOut__LCD_EN)
/* Slew Rate Control */
#define refOut_SLW                    (* (reg8 *) refOut__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define refOut_PRTDSI__CAPS_SEL       (* (reg8 *) refOut__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define refOut_PRTDSI__DBL_SYNC_IN    (* (reg8 *) refOut__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define refOut_PRTDSI__OE_SEL0        (* (reg8 *) refOut__PRTDSI__OE_SEL0) 
#define refOut_PRTDSI__OE_SEL1        (* (reg8 *) refOut__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define refOut_PRTDSI__OUT_SEL0       (* (reg8 *) refOut__PRTDSI__OUT_SEL0) 
#define refOut_PRTDSI__OUT_SEL1       (* (reg8 *) refOut__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define refOut_PRTDSI__SYNC_OUT       (* (reg8 *) refOut__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(refOut__SIO_CFG)
    #define refOut_SIO_HYST_EN        (* (reg8 *) refOut__SIO_HYST_EN)
    #define refOut_SIO_REG_HIFREQ     (* (reg8 *) refOut__SIO_REG_HIFREQ)
    #define refOut_SIO_CFG            (* (reg8 *) refOut__SIO_CFG)
    #define refOut_SIO_DIFF           (* (reg8 *) refOut__SIO_DIFF)
#endif /* (refOut__SIO_CFG) */

/* Interrupt Registers */
#if defined(refOut__INTSTAT)
    #define refOut_INTSTAT            (* (reg8 *) refOut__INTSTAT)
    #define refOut_SNAP               (* (reg8 *) refOut__SNAP)
    
	#define refOut_0_INTTYPE_REG 		(* (reg8 *) refOut__0__INTTYPE)
#endif /* (refOut__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_refOut_H */


/* [] END OF FILE */
