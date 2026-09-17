/*******************************************************************************
* File Name: OPA_minus.h  
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

#if !defined(CY_PINS_OPA_minus_H) /* Pins OPA_minus_H */
#define CY_PINS_OPA_minus_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "OPA_minus_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 OPA_minus__PORT == 15 && ((OPA_minus__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    OPA_minus_Write(uint8 value);
void    OPA_minus_SetDriveMode(uint8 mode);
uint8   OPA_minus_ReadDataReg(void);
uint8   OPA_minus_Read(void);
void    OPA_minus_SetInterruptMode(uint16 position, uint16 mode);
uint8   OPA_minus_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the OPA_minus_SetDriveMode() function.
     *  @{
     */
        #define OPA_minus_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define OPA_minus_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define OPA_minus_DM_RES_UP          PIN_DM_RES_UP
        #define OPA_minus_DM_RES_DWN         PIN_DM_RES_DWN
        #define OPA_minus_DM_OD_LO           PIN_DM_OD_LO
        #define OPA_minus_DM_OD_HI           PIN_DM_OD_HI
        #define OPA_minus_DM_STRONG          PIN_DM_STRONG
        #define OPA_minus_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define OPA_minus_MASK               OPA_minus__MASK
#define OPA_minus_SHIFT              OPA_minus__SHIFT
#define OPA_minus_WIDTH              1u

/* Interrupt constants */
#if defined(OPA_minus__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in OPA_minus_SetInterruptMode() function.
     *  @{
     */
        #define OPA_minus_INTR_NONE      (uint16)(0x0000u)
        #define OPA_minus_INTR_RISING    (uint16)(0x0001u)
        #define OPA_minus_INTR_FALLING   (uint16)(0x0002u)
        #define OPA_minus_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define OPA_minus_INTR_MASK      (0x01u) 
#endif /* (OPA_minus__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define OPA_minus_PS                     (* (reg8 *) OPA_minus__PS)
/* Data Register */
#define OPA_minus_DR                     (* (reg8 *) OPA_minus__DR)
/* Port Number */
#define OPA_minus_PRT_NUM                (* (reg8 *) OPA_minus__PRT) 
/* Connect to Analog Globals */                                                  
#define OPA_minus_AG                     (* (reg8 *) OPA_minus__AG)                       
/* Analog MUX bux enable */
#define OPA_minus_AMUX                   (* (reg8 *) OPA_minus__AMUX) 
/* Bidirectional Enable */                                                        
#define OPA_minus_BIE                    (* (reg8 *) OPA_minus__BIE)
/* Bit-mask for Aliased Register Access */
#define OPA_minus_BIT_MASK               (* (reg8 *) OPA_minus__BIT_MASK)
/* Bypass Enable */
#define OPA_minus_BYP                    (* (reg8 *) OPA_minus__BYP)
/* Port wide control signals */                                                   
#define OPA_minus_CTL                    (* (reg8 *) OPA_minus__CTL)
/* Drive Modes */
#define OPA_minus_DM0                    (* (reg8 *) OPA_minus__DM0) 
#define OPA_minus_DM1                    (* (reg8 *) OPA_minus__DM1)
#define OPA_minus_DM2                    (* (reg8 *) OPA_minus__DM2) 
/* Input Buffer Disable Override */
#define OPA_minus_INP_DIS                (* (reg8 *) OPA_minus__INP_DIS)
/* LCD Common or Segment Drive */
#define OPA_minus_LCD_COM_SEG            (* (reg8 *) OPA_minus__LCD_COM_SEG)
/* Enable Segment LCD */
#define OPA_minus_LCD_EN                 (* (reg8 *) OPA_minus__LCD_EN)
/* Slew Rate Control */
#define OPA_minus_SLW                    (* (reg8 *) OPA_minus__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define OPA_minus_PRTDSI__CAPS_SEL       (* (reg8 *) OPA_minus__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define OPA_minus_PRTDSI__DBL_SYNC_IN    (* (reg8 *) OPA_minus__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define OPA_minus_PRTDSI__OE_SEL0        (* (reg8 *) OPA_minus__PRTDSI__OE_SEL0) 
#define OPA_minus_PRTDSI__OE_SEL1        (* (reg8 *) OPA_minus__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define OPA_minus_PRTDSI__OUT_SEL0       (* (reg8 *) OPA_minus__PRTDSI__OUT_SEL0) 
#define OPA_minus_PRTDSI__OUT_SEL1       (* (reg8 *) OPA_minus__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define OPA_minus_PRTDSI__SYNC_OUT       (* (reg8 *) OPA_minus__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(OPA_minus__SIO_CFG)
    #define OPA_minus_SIO_HYST_EN        (* (reg8 *) OPA_minus__SIO_HYST_EN)
    #define OPA_minus_SIO_REG_HIFREQ     (* (reg8 *) OPA_minus__SIO_REG_HIFREQ)
    #define OPA_minus_SIO_CFG            (* (reg8 *) OPA_minus__SIO_CFG)
    #define OPA_minus_SIO_DIFF           (* (reg8 *) OPA_minus__SIO_DIFF)
#endif /* (OPA_minus__SIO_CFG) */

/* Interrupt Registers */
#if defined(OPA_minus__INTSTAT)
    #define OPA_minus_INTSTAT            (* (reg8 *) OPA_minus__INTSTAT)
    #define OPA_minus_SNAP               (* (reg8 *) OPA_minus__SNAP)
    
	#define OPA_minus_0_INTTYPE_REG 		(* (reg8 *) OPA_minus__0__INTTYPE)
#endif /* (OPA_minus__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_OPA_minus_H */


/* [] END OF FILE */
