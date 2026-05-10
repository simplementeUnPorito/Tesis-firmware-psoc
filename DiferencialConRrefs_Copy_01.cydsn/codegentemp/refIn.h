/*******************************************************************************
* File Name: refIn.h  
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

#if !defined(CY_PINS_refIn_H) /* Pins refIn_H */
#define CY_PINS_refIn_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "refIn_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 refIn__PORT == 15 && ((refIn__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    refIn_Write(uint8 value);
void    refIn_SetDriveMode(uint8 mode);
uint8   refIn_ReadDataReg(void);
uint8   refIn_Read(void);
void    refIn_SetInterruptMode(uint16 position, uint16 mode);
uint8   refIn_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the refIn_SetDriveMode() function.
     *  @{
     */
        #define refIn_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define refIn_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define refIn_DM_RES_UP          PIN_DM_RES_UP
        #define refIn_DM_RES_DWN         PIN_DM_RES_DWN
        #define refIn_DM_OD_LO           PIN_DM_OD_LO
        #define refIn_DM_OD_HI           PIN_DM_OD_HI
        #define refIn_DM_STRONG          PIN_DM_STRONG
        #define refIn_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define refIn_MASK               refIn__MASK
#define refIn_SHIFT              refIn__SHIFT
#define refIn_WIDTH              1u

/* Interrupt constants */
#if defined(refIn__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in refIn_SetInterruptMode() function.
     *  @{
     */
        #define refIn_INTR_NONE      (uint16)(0x0000u)
        #define refIn_INTR_RISING    (uint16)(0x0001u)
        #define refIn_INTR_FALLING   (uint16)(0x0002u)
        #define refIn_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define refIn_INTR_MASK      (0x01u) 
#endif /* (refIn__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define refIn_PS                     (* (reg8 *) refIn__PS)
/* Data Register */
#define refIn_DR                     (* (reg8 *) refIn__DR)
/* Port Number */
#define refIn_PRT_NUM                (* (reg8 *) refIn__PRT) 
/* Connect to Analog Globals */                                                  
#define refIn_AG                     (* (reg8 *) refIn__AG)                       
/* Analog MUX bux enable */
#define refIn_AMUX                   (* (reg8 *) refIn__AMUX) 
/* Bidirectional Enable */                                                        
#define refIn_BIE                    (* (reg8 *) refIn__BIE)
/* Bit-mask for Aliased Register Access */
#define refIn_BIT_MASK               (* (reg8 *) refIn__BIT_MASK)
/* Bypass Enable */
#define refIn_BYP                    (* (reg8 *) refIn__BYP)
/* Port wide control signals */                                                   
#define refIn_CTL                    (* (reg8 *) refIn__CTL)
/* Drive Modes */
#define refIn_DM0                    (* (reg8 *) refIn__DM0) 
#define refIn_DM1                    (* (reg8 *) refIn__DM1)
#define refIn_DM2                    (* (reg8 *) refIn__DM2) 
/* Input Buffer Disable Override */
#define refIn_INP_DIS                (* (reg8 *) refIn__INP_DIS)
/* LCD Common or Segment Drive */
#define refIn_LCD_COM_SEG            (* (reg8 *) refIn__LCD_COM_SEG)
/* Enable Segment LCD */
#define refIn_LCD_EN                 (* (reg8 *) refIn__LCD_EN)
/* Slew Rate Control */
#define refIn_SLW                    (* (reg8 *) refIn__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define refIn_PRTDSI__CAPS_SEL       (* (reg8 *) refIn__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define refIn_PRTDSI__DBL_SYNC_IN    (* (reg8 *) refIn__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define refIn_PRTDSI__OE_SEL0        (* (reg8 *) refIn__PRTDSI__OE_SEL0) 
#define refIn_PRTDSI__OE_SEL1        (* (reg8 *) refIn__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define refIn_PRTDSI__OUT_SEL0       (* (reg8 *) refIn__PRTDSI__OUT_SEL0) 
#define refIn_PRTDSI__OUT_SEL1       (* (reg8 *) refIn__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define refIn_PRTDSI__SYNC_OUT       (* (reg8 *) refIn__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(refIn__SIO_CFG)
    #define refIn_SIO_HYST_EN        (* (reg8 *) refIn__SIO_HYST_EN)
    #define refIn_SIO_REG_HIFREQ     (* (reg8 *) refIn__SIO_REG_HIFREQ)
    #define refIn_SIO_CFG            (* (reg8 *) refIn__SIO_CFG)
    #define refIn_SIO_DIFF           (* (reg8 *) refIn__SIO_DIFF)
#endif /* (refIn__SIO_CFG) */

/* Interrupt Registers */
#if defined(refIn__INTSTAT)
    #define refIn_INTSTAT            (* (reg8 *) refIn__INTSTAT)
    #define refIn_SNAP               (* (reg8 *) refIn__SNAP)
    
	#define refIn_0_INTTYPE_REG 		(* (reg8 *) refIn__0__INTTYPE)
#endif /* (refIn__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_refIn_H */


/* [] END OF FILE */
