/*******************************************************************************
* File Name: BP_vp.h  
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

#if !defined(CY_PINS_BP_vp_H) /* Pins BP_vp_H */
#define CY_PINS_BP_vp_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "BP_vp_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 BP_vp__PORT == 15 && ((BP_vp__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    BP_vp_Write(uint8 value);
void    BP_vp_SetDriveMode(uint8 mode);
uint8   BP_vp_ReadDataReg(void);
uint8   BP_vp_Read(void);
void    BP_vp_SetInterruptMode(uint16 position, uint16 mode);
uint8   BP_vp_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the BP_vp_SetDriveMode() function.
     *  @{
     */
        #define BP_vp_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define BP_vp_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define BP_vp_DM_RES_UP          PIN_DM_RES_UP
        #define BP_vp_DM_RES_DWN         PIN_DM_RES_DWN
        #define BP_vp_DM_OD_LO           PIN_DM_OD_LO
        #define BP_vp_DM_OD_HI           PIN_DM_OD_HI
        #define BP_vp_DM_STRONG          PIN_DM_STRONG
        #define BP_vp_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define BP_vp_MASK               BP_vp__MASK
#define BP_vp_SHIFT              BP_vp__SHIFT
#define BP_vp_WIDTH              1u

/* Interrupt constants */
#if defined(BP_vp__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in BP_vp_SetInterruptMode() function.
     *  @{
     */
        #define BP_vp_INTR_NONE      (uint16)(0x0000u)
        #define BP_vp_INTR_RISING    (uint16)(0x0001u)
        #define BP_vp_INTR_FALLING   (uint16)(0x0002u)
        #define BP_vp_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define BP_vp_INTR_MASK      (0x01u) 
#endif /* (BP_vp__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define BP_vp_PS                     (* (reg8 *) BP_vp__PS)
/* Data Register */
#define BP_vp_DR                     (* (reg8 *) BP_vp__DR)
/* Port Number */
#define BP_vp_PRT_NUM                (* (reg8 *) BP_vp__PRT) 
/* Connect to Analog Globals */                                                  
#define BP_vp_AG                     (* (reg8 *) BP_vp__AG)                       
/* Analog MUX bux enable */
#define BP_vp_AMUX                   (* (reg8 *) BP_vp__AMUX) 
/* Bidirectional Enable */                                                        
#define BP_vp_BIE                    (* (reg8 *) BP_vp__BIE)
/* Bit-mask for Aliased Register Access */
#define BP_vp_BIT_MASK               (* (reg8 *) BP_vp__BIT_MASK)
/* Bypass Enable */
#define BP_vp_BYP                    (* (reg8 *) BP_vp__BYP)
/* Port wide control signals */                                                   
#define BP_vp_CTL                    (* (reg8 *) BP_vp__CTL)
/* Drive Modes */
#define BP_vp_DM0                    (* (reg8 *) BP_vp__DM0) 
#define BP_vp_DM1                    (* (reg8 *) BP_vp__DM1)
#define BP_vp_DM2                    (* (reg8 *) BP_vp__DM2) 
/* Input Buffer Disable Override */
#define BP_vp_INP_DIS                (* (reg8 *) BP_vp__INP_DIS)
/* LCD Common or Segment Drive */
#define BP_vp_LCD_COM_SEG            (* (reg8 *) BP_vp__LCD_COM_SEG)
/* Enable Segment LCD */
#define BP_vp_LCD_EN                 (* (reg8 *) BP_vp__LCD_EN)
/* Slew Rate Control */
#define BP_vp_SLW                    (* (reg8 *) BP_vp__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define BP_vp_PRTDSI__CAPS_SEL       (* (reg8 *) BP_vp__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define BP_vp_PRTDSI__DBL_SYNC_IN    (* (reg8 *) BP_vp__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define BP_vp_PRTDSI__OE_SEL0        (* (reg8 *) BP_vp__PRTDSI__OE_SEL0) 
#define BP_vp_PRTDSI__OE_SEL1        (* (reg8 *) BP_vp__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define BP_vp_PRTDSI__OUT_SEL0       (* (reg8 *) BP_vp__PRTDSI__OUT_SEL0) 
#define BP_vp_PRTDSI__OUT_SEL1       (* (reg8 *) BP_vp__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define BP_vp_PRTDSI__SYNC_OUT       (* (reg8 *) BP_vp__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(BP_vp__SIO_CFG)
    #define BP_vp_SIO_HYST_EN        (* (reg8 *) BP_vp__SIO_HYST_EN)
    #define BP_vp_SIO_REG_HIFREQ     (* (reg8 *) BP_vp__SIO_REG_HIFREQ)
    #define BP_vp_SIO_CFG            (* (reg8 *) BP_vp__SIO_CFG)
    #define BP_vp_SIO_DIFF           (* (reg8 *) BP_vp__SIO_DIFF)
#endif /* (BP_vp__SIO_CFG) */

/* Interrupt Registers */
#if defined(BP_vp__INTSTAT)
    #define BP_vp_INTSTAT            (* (reg8 *) BP_vp__INTSTAT)
    #define BP_vp_SNAP               (* (reg8 *) BP_vp__SNAP)
    
	#define BP_vp_0_INTTYPE_REG 		(* (reg8 *) BP_vp__0__INTTYPE)
#endif /* (BP_vp__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_BP_vp_H */


/* [] END OF FILE */
