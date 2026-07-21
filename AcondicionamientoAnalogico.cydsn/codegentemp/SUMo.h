/*******************************************************************************
* File Name: SUMo.h  
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

#if !defined(CY_PINS_SUMo_H) /* Pins SUMo_H */
#define CY_PINS_SUMo_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "SUMo_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 SUMo__PORT == 15 && ((SUMo__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    SUMo_Write(uint8 value);
void    SUMo_SetDriveMode(uint8 mode);
uint8   SUMo_ReadDataReg(void);
uint8   SUMo_Read(void);
void    SUMo_SetInterruptMode(uint16 position, uint16 mode);
uint8   SUMo_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the SUMo_SetDriveMode() function.
     *  @{
     */
        #define SUMo_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define SUMo_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define SUMo_DM_RES_UP          PIN_DM_RES_UP
        #define SUMo_DM_RES_DWN         PIN_DM_RES_DWN
        #define SUMo_DM_OD_LO           PIN_DM_OD_LO
        #define SUMo_DM_OD_HI           PIN_DM_OD_HI
        #define SUMo_DM_STRONG          PIN_DM_STRONG
        #define SUMo_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define SUMo_MASK               SUMo__MASK
#define SUMo_SHIFT              SUMo__SHIFT
#define SUMo_WIDTH              1u

/* Interrupt constants */
#if defined(SUMo__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in SUMo_SetInterruptMode() function.
     *  @{
     */
        #define SUMo_INTR_NONE      (uint16)(0x0000u)
        #define SUMo_INTR_RISING    (uint16)(0x0001u)
        #define SUMo_INTR_FALLING   (uint16)(0x0002u)
        #define SUMo_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define SUMo_INTR_MASK      (0x01u) 
#endif /* (SUMo__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define SUMo_PS                     (* (reg8 *) SUMo__PS)
/* Data Register */
#define SUMo_DR                     (* (reg8 *) SUMo__DR)
/* Port Number */
#define SUMo_PRT_NUM                (* (reg8 *) SUMo__PRT) 
/* Connect to Analog Globals */                                                  
#define SUMo_AG                     (* (reg8 *) SUMo__AG)                       
/* Analog MUX bux enable */
#define SUMo_AMUX                   (* (reg8 *) SUMo__AMUX) 
/* Bidirectional Enable */                                                        
#define SUMo_BIE                    (* (reg8 *) SUMo__BIE)
/* Bit-mask for Aliased Register Access */
#define SUMo_BIT_MASK               (* (reg8 *) SUMo__BIT_MASK)
/* Bypass Enable */
#define SUMo_BYP                    (* (reg8 *) SUMo__BYP)
/* Port wide control signals */                                                   
#define SUMo_CTL                    (* (reg8 *) SUMo__CTL)
/* Drive Modes */
#define SUMo_DM0                    (* (reg8 *) SUMo__DM0) 
#define SUMo_DM1                    (* (reg8 *) SUMo__DM1)
#define SUMo_DM2                    (* (reg8 *) SUMo__DM2) 
/* Input Buffer Disable Override */
#define SUMo_INP_DIS                (* (reg8 *) SUMo__INP_DIS)
/* LCD Common or Segment Drive */
#define SUMo_LCD_COM_SEG            (* (reg8 *) SUMo__LCD_COM_SEG)
/* Enable Segment LCD */
#define SUMo_LCD_EN                 (* (reg8 *) SUMo__LCD_EN)
/* Slew Rate Control */
#define SUMo_SLW                    (* (reg8 *) SUMo__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define SUMo_PRTDSI__CAPS_SEL       (* (reg8 *) SUMo__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define SUMo_PRTDSI__DBL_SYNC_IN    (* (reg8 *) SUMo__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define SUMo_PRTDSI__OE_SEL0        (* (reg8 *) SUMo__PRTDSI__OE_SEL0) 
#define SUMo_PRTDSI__OE_SEL1        (* (reg8 *) SUMo__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define SUMo_PRTDSI__OUT_SEL0       (* (reg8 *) SUMo__PRTDSI__OUT_SEL0) 
#define SUMo_PRTDSI__OUT_SEL1       (* (reg8 *) SUMo__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define SUMo_PRTDSI__SYNC_OUT       (* (reg8 *) SUMo__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(SUMo__SIO_CFG)
    #define SUMo_SIO_HYST_EN        (* (reg8 *) SUMo__SIO_HYST_EN)
    #define SUMo_SIO_REG_HIFREQ     (* (reg8 *) SUMo__SIO_REG_HIFREQ)
    #define SUMo_SIO_CFG            (* (reg8 *) SUMo__SIO_CFG)
    #define SUMo_SIO_DIFF           (* (reg8 *) SUMo__SIO_DIFF)
#endif /* (SUMo__SIO_CFG) */

/* Interrupt Registers */
#if defined(SUMo__INTSTAT)
    #define SUMo_INTSTAT            (* (reg8 *) SUMo__INTSTAT)
    #define SUMo_SNAP               (* (reg8 *) SUMo__SNAP)
    
	#define SUMo_0_INTTYPE_REG 		(* (reg8 *) SUMo__0__INTTYPE)
#endif /* (SUMo__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_SUMo_H */


/* [] END OF FILE */
