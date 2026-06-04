/*******************************************************************************
* File Name: SEo.h  
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

#if !defined(CY_PINS_SEo_H) /* Pins SEo_H */
#define CY_PINS_SEo_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "SEo_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 SEo__PORT == 15 && ((SEo__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    SEo_Write(uint8 value);
void    SEo_SetDriveMode(uint8 mode);
uint8   SEo_ReadDataReg(void);
uint8   SEo_Read(void);
void    SEo_SetInterruptMode(uint16 position, uint16 mode);
uint8   SEo_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the SEo_SetDriveMode() function.
     *  @{
     */
        #define SEo_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define SEo_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define SEo_DM_RES_UP          PIN_DM_RES_UP
        #define SEo_DM_RES_DWN         PIN_DM_RES_DWN
        #define SEo_DM_OD_LO           PIN_DM_OD_LO
        #define SEo_DM_OD_HI           PIN_DM_OD_HI
        #define SEo_DM_STRONG          PIN_DM_STRONG
        #define SEo_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define SEo_MASK               SEo__MASK
#define SEo_SHIFT              SEo__SHIFT
#define SEo_WIDTH              1u

/* Interrupt constants */
#if defined(SEo__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in SEo_SetInterruptMode() function.
     *  @{
     */
        #define SEo_INTR_NONE      (uint16)(0x0000u)
        #define SEo_INTR_RISING    (uint16)(0x0001u)
        #define SEo_INTR_FALLING   (uint16)(0x0002u)
        #define SEo_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define SEo_INTR_MASK      (0x01u) 
#endif /* (SEo__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define SEo_PS                     (* (reg8 *) SEo__PS)
/* Data Register */
#define SEo_DR                     (* (reg8 *) SEo__DR)
/* Port Number */
#define SEo_PRT_NUM                (* (reg8 *) SEo__PRT) 
/* Connect to Analog Globals */                                                  
#define SEo_AG                     (* (reg8 *) SEo__AG)                       
/* Analog MUX bux enable */
#define SEo_AMUX                   (* (reg8 *) SEo__AMUX) 
/* Bidirectional Enable */                                                        
#define SEo_BIE                    (* (reg8 *) SEo__BIE)
/* Bit-mask for Aliased Register Access */
#define SEo_BIT_MASK               (* (reg8 *) SEo__BIT_MASK)
/* Bypass Enable */
#define SEo_BYP                    (* (reg8 *) SEo__BYP)
/* Port wide control signals */                                                   
#define SEo_CTL                    (* (reg8 *) SEo__CTL)
/* Drive Modes */
#define SEo_DM0                    (* (reg8 *) SEo__DM0) 
#define SEo_DM1                    (* (reg8 *) SEo__DM1)
#define SEo_DM2                    (* (reg8 *) SEo__DM2) 
/* Input Buffer Disable Override */
#define SEo_INP_DIS                (* (reg8 *) SEo__INP_DIS)
/* LCD Common or Segment Drive */
#define SEo_LCD_COM_SEG            (* (reg8 *) SEo__LCD_COM_SEG)
/* Enable Segment LCD */
#define SEo_LCD_EN                 (* (reg8 *) SEo__LCD_EN)
/* Slew Rate Control */
#define SEo_SLW                    (* (reg8 *) SEo__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define SEo_PRTDSI__CAPS_SEL       (* (reg8 *) SEo__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define SEo_PRTDSI__DBL_SYNC_IN    (* (reg8 *) SEo__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define SEo_PRTDSI__OE_SEL0        (* (reg8 *) SEo__PRTDSI__OE_SEL0) 
#define SEo_PRTDSI__OE_SEL1        (* (reg8 *) SEo__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define SEo_PRTDSI__OUT_SEL0       (* (reg8 *) SEo__PRTDSI__OUT_SEL0) 
#define SEo_PRTDSI__OUT_SEL1       (* (reg8 *) SEo__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define SEo_PRTDSI__SYNC_OUT       (* (reg8 *) SEo__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(SEo__SIO_CFG)
    #define SEo_SIO_HYST_EN        (* (reg8 *) SEo__SIO_HYST_EN)
    #define SEo_SIO_REG_HIFREQ     (* (reg8 *) SEo__SIO_REG_HIFREQ)
    #define SEo_SIO_CFG            (* (reg8 *) SEo__SIO_CFG)
    #define SEo_SIO_DIFF           (* (reg8 *) SEo__SIO_DIFF)
#endif /* (SEo__SIO_CFG) */

/* Interrupt Registers */
#if defined(SEo__INTSTAT)
    #define SEo_INTSTAT            (* (reg8 *) SEo__INTSTAT)
    #define SEo_SNAP               (* (reg8 *) SEo__SNAP)
    
	#define SEo_0_INTTYPE_REG 		(* (reg8 *) SEo__0__INTTYPE)
#endif /* (SEo__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_SEo_H */


/* [] END OF FILE */
