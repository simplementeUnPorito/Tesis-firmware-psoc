/*******************************************************************************
* File Name: boton.h  
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

#if !defined(CY_PINS_boton_H) /* Pins boton_H */
#define CY_PINS_boton_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "boton_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 boton__PORT == 15 && ((boton__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    boton_Write(uint8 value);
void    boton_SetDriveMode(uint8 mode);
uint8   boton_ReadDataReg(void);
uint8   boton_Read(void);
void    boton_SetInterruptMode(uint16 position, uint16 mode);
uint8   boton_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the boton_SetDriveMode() function.
     *  @{
     */
        #define boton_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define boton_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define boton_DM_RES_UP          PIN_DM_RES_UP
        #define boton_DM_RES_DWN         PIN_DM_RES_DWN
        #define boton_DM_OD_LO           PIN_DM_OD_LO
        #define boton_DM_OD_HI           PIN_DM_OD_HI
        #define boton_DM_STRONG          PIN_DM_STRONG
        #define boton_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define boton_MASK               boton__MASK
#define boton_SHIFT              boton__SHIFT
#define boton_WIDTH              1u

/* Interrupt constants */
#if defined(boton__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in boton_SetInterruptMode() function.
     *  @{
     */
        #define boton_INTR_NONE      (uint16)(0x0000u)
        #define boton_INTR_RISING    (uint16)(0x0001u)
        #define boton_INTR_FALLING   (uint16)(0x0002u)
        #define boton_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define boton_INTR_MASK      (0x01u) 
#endif /* (boton__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define boton_PS                     (* (reg8 *) boton__PS)
/* Data Register */
#define boton_DR                     (* (reg8 *) boton__DR)
/* Port Number */
#define boton_PRT_NUM                (* (reg8 *) boton__PRT) 
/* Connect to Analog Globals */                                                  
#define boton_AG                     (* (reg8 *) boton__AG)                       
/* Analog MUX bux enable */
#define boton_AMUX                   (* (reg8 *) boton__AMUX) 
/* Bidirectional Enable */                                                        
#define boton_BIE                    (* (reg8 *) boton__BIE)
/* Bit-mask for Aliased Register Access */
#define boton_BIT_MASK               (* (reg8 *) boton__BIT_MASK)
/* Bypass Enable */
#define boton_BYP                    (* (reg8 *) boton__BYP)
/* Port wide control signals */                                                   
#define boton_CTL                    (* (reg8 *) boton__CTL)
/* Drive Modes */
#define boton_DM0                    (* (reg8 *) boton__DM0) 
#define boton_DM1                    (* (reg8 *) boton__DM1)
#define boton_DM2                    (* (reg8 *) boton__DM2) 
/* Input Buffer Disable Override */
#define boton_INP_DIS                (* (reg8 *) boton__INP_DIS)
/* LCD Common or Segment Drive */
#define boton_LCD_COM_SEG            (* (reg8 *) boton__LCD_COM_SEG)
/* Enable Segment LCD */
#define boton_LCD_EN                 (* (reg8 *) boton__LCD_EN)
/* Slew Rate Control */
#define boton_SLW                    (* (reg8 *) boton__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define boton_PRTDSI__CAPS_SEL       (* (reg8 *) boton__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define boton_PRTDSI__DBL_SYNC_IN    (* (reg8 *) boton__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define boton_PRTDSI__OE_SEL0        (* (reg8 *) boton__PRTDSI__OE_SEL0) 
#define boton_PRTDSI__OE_SEL1        (* (reg8 *) boton__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define boton_PRTDSI__OUT_SEL0       (* (reg8 *) boton__PRTDSI__OUT_SEL0) 
#define boton_PRTDSI__OUT_SEL1       (* (reg8 *) boton__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define boton_PRTDSI__SYNC_OUT       (* (reg8 *) boton__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(boton__SIO_CFG)
    #define boton_SIO_HYST_EN        (* (reg8 *) boton__SIO_HYST_EN)
    #define boton_SIO_REG_HIFREQ     (* (reg8 *) boton__SIO_REG_HIFREQ)
    #define boton_SIO_CFG            (* (reg8 *) boton__SIO_CFG)
    #define boton_SIO_DIFF           (* (reg8 *) boton__SIO_DIFF)
#endif /* (boton__SIO_CFG) */

/* Interrupt Registers */
#if defined(boton__INTSTAT)
    #define boton_INTSTAT            (* (reg8 *) boton__INTSTAT)
    #define boton_SNAP               (* (reg8 *) boton__SNAP)
    
	#define boton_0_INTTYPE_REG 		(* (reg8 *) boton__0__INTTYPE)
#endif /* (boton__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_boton_H */


/* [] END OF FILE */
