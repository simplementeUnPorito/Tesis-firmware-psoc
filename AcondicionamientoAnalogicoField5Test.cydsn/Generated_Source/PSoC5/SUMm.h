/*******************************************************************************
* File Name: SUMm.h  
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

#if !defined(CY_PINS_SUMm_H) /* Pins SUMm_H */
#define CY_PINS_SUMm_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "SUMm_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 SUMm__PORT == 15 && ((SUMm__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    SUMm_Write(uint8 value);
void    SUMm_SetDriveMode(uint8 mode);
uint8   SUMm_ReadDataReg(void);
uint8   SUMm_Read(void);
void    SUMm_SetInterruptMode(uint16 position, uint16 mode);
uint8   SUMm_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the SUMm_SetDriveMode() function.
     *  @{
     */
        #define SUMm_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define SUMm_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define SUMm_DM_RES_UP          PIN_DM_RES_UP
        #define SUMm_DM_RES_DWN         PIN_DM_RES_DWN
        #define SUMm_DM_OD_LO           PIN_DM_OD_LO
        #define SUMm_DM_OD_HI           PIN_DM_OD_HI
        #define SUMm_DM_STRONG          PIN_DM_STRONG
        #define SUMm_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define SUMm_MASK               SUMm__MASK
#define SUMm_SHIFT              SUMm__SHIFT
#define SUMm_WIDTH              1u

/* Interrupt constants */
#if defined(SUMm__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in SUMm_SetInterruptMode() function.
     *  @{
     */
        #define SUMm_INTR_NONE      (uint16)(0x0000u)
        #define SUMm_INTR_RISING    (uint16)(0x0001u)
        #define SUMm_INTR_FALLING   (uint16)(0x0002u)
        #define SUMm_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define SUMm_INTR_MASK      (0x01u) 
#endif /* (SUMm__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define SUMm_PS                     (* (reg8 *) SUMm__PS)
/* Data Register */
#define SUMm_DR                     (* (reg8 *) SUMm__DR)
/* Port Number */
#define SUMm_PRT_NUM                (* (reg8 *) SUMm__PRT) 
/* Connect to Analog Globals */                                                  
#define SUMm_AG                     (* (reg8 *) SUMm__AG)                       
/* Analog MUX bux enable */
#define SUMm_AMUX                   (* (reg8 *) SUMm__AMUX) 
/* Bidirectional Enable */                                                        
#define SUMm_BIE                    (* (reg8 *) SUMm__BIE)
/* Bit-mask for Aliased Register Access */
#define SUMm_BIT_MASK               (* (reg8 *) SUMm__BIT_MASK)
/* Bypass Enable */
#define SUMm_BYP                    (* (reg8 *) SUMm__BYP)
/* Port wide control signals */                                                   
#define SUMm_CTL                    (* (reg8 *) SUMm__CTL)
/* Drive Modes */
#define SUMm_DM0                    (* (reg8 *) SUMm__DM0) 
#define SUMm_DM1                    (* (reg8 *) SUMm__DM1)
#define SUMm_DM2                    (* (reg8 *) SUMm__DM2) 
/* Input Buffer Disable Override */
#define SUMm_INP_DIS                (* (reg8 *) SUMm__INP_DIS)
/* LCD Common or Segment Drive */
#define SUMm_LCD_COM_SEG            (* (reg8 *) SUMm__LCD_COM_SEG)
/* Enable Segment LCD */
#define SUMm_LCD_EN                 (* (reg8 *) SUMm__LCD_EN)
/* Slew Rate Control */
#define SUMm_SLW                    (* (reg8 *) SUMm__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define SUMm_PRTDSI__CAPS_SEL       (* (reg8 *) SUMm__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define SUMm_PRTDSI__DBL_SYNC_IN    (* (reg8 *) SUMm__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define SUMm_PRTDSI__OE_SEL0        (* (reg8 *) SUMm__PRTDSI__OE_SEL0) 
#define SUMm_PRTDSI__OE_SEL1        (* (reg8 *) SUMm__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define SUMm_PRTDSI__OUT_SEL0       (* (reg8 *) SUMm__PRTDSI__OUT_SEL0) 
#define SUMm_PRTDSI__OUT_SEL1       (* (reg8 *) SUMm__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define SUMm_PRTDSI__SYNC_OUT       (* (reg8 *) SUMm__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(SUMm__SIO_CFG)
    #define SUMm_SIO_HYST_EN        (* (reg8 *) SUMm__SIO_HYST_EN)
    #define SUMm_SIO_REG_HIFREQ     (* (reg8 *) SUMm__SIO_REG_HIFREQ)
    #define SUMm_SIO_CFG            (* (reg8 *) SUMm__SIO_CFG)
    #define SUMm_SIO_DIFF           (* (reg8 *) SUMm__SIO_DIFF)
#endif /* (SUMm__SIO_CFG) */

/* Interrupt Registers */
#if defined(SUMm__INTSTAT)
    #define SUMm_INTSTAT            (* (reg8 *) SUMm__INTSTAT)
    #define SUMm_SNAP               (* (reg8 *) SUMm__SNAP)
    
	#define SUMm_0_INTTYPE_REG 		(* (reg8 *) SUMm__0__INTTYPE)
#endif /* (SUMm__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_SUMm_H */


/* [] END OF FILE */
