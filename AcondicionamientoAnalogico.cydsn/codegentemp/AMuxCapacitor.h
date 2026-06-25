/*******************************************************************************
* File Name: AMuxCapacitor.h  
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

#if !defined(CY_PINS_AMuxCapacitor_H) /* Pins AMuxCapacitor_H */
#define CY_PINS_AMuxCapacitor_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "AMuxCapacitor_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 AMuxCapacitor__PORT == 15 && ((AMuxCapacitor__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    AMuxCapacitor_Write(uint8 value);
void    AMuxCapacitor_SetDriveMode(uint8 mode);
uint8   AMuxCapacitor_ReadDataReg(void);
uint8   AMuxCapacitor_Read(void);
void    AMuxCapacitor_SetInterruptMode(uint16 position, uint16 mode);
uint8   AMuxCapacitor_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the AMuxCapacitor_SetDriveMode() function.
     *  @{
     */
        #define AMuxCapacitor_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define AMuxCapacitor_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define AMuxCapacitor_DM_RES_UP          PIN_DM_RES_UP
        #define AMuxCapacitor_DM_RES_DWN         PIN_DM_RES_DWN
        #define AMuxCapacitor_DM_OD_LO           PIN_DM_OD_LO
        #define AMuxCapacitor_DM_OD_HI           PIN_DM_OD_HI
        #define AMuxCapacitor_DM_STRONG          PIN_DM_STRONG
        #define AMuxCapacitor_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define AMuxCapacitor_MASK               AMuxCapacitor__MASK
#define AMuxCapacitor_SHIFT              AMuxCapacitor__SHIFT
#define AMuxCapacitor_WIDTH              1u

/* Interrupt constants */
#if defined(AMuxCapacitor__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in AMuxCapacitor_SetInterruptMode() function.
     *  @{
     */
        #define AMuxCapacitor_INTR_NONE      (uint16)(0x0000u)
        #define AMuxCapacitor_INTR_RISING    (uint16)(0x0001u)
        #define AMuxCapacitor_INTR_FALLING   (uint16)(0x0002u)
        #define AMuxCapacitor_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define AMuxCapacitor_INTR_MASK      (0x01u) 
#endif /* (AMuxCapacitor__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define AMuxCapacitor_PS                     (* (reg8 *) AMuxCapacitor__PS)
/* Data Register */
#define AMuxCapacitor_DR                     (* (reg8 *) AMuxCapacitor__DR)
/* Port Number */
#define AMuxCapacitor_PRT_NUM                (* (reg8 *) AMuxCapacitor__PRT) 
/* Connect to Analog Globals */                                                  
#define AMuxCapacitor_AG                     (* (reg8 *) AMuxCapacitor__AG)                       
/* Analog MUX bux enable */
#define AMuxCapacitor_AMUX                   (* (reg8 *) AMuxCapacitor__AMUX) 
/* Bidirectional Enable */                                                        
#define AMuxCapacitor_BIE                    (* (reg8 *) AMuxCapacitor__BIE)
/* Bit-mask for Aliased Register Access */
#define AMuxCapacitor_BIT_MASK               (* (reg8 *) AMuxCapacitor__BIT_MASK)
/* Bypass Enable */
#define AMuxCapacitor_BYP                    (* (reg8 *) AMuxCapacitor__BYP)
/* Port wide control signals */                                                   
#define AMuxCapacitor_CTL                    (* (reg8 *) AMuxCapacitor__CTL)
/* Drive Modes */
#define AMuxCapacitor_DM0                    (* (reg8 *) AMuxCapacitor__DM0) 
#define AMuxCapacitor_DM1                    (* (reg8 *) AMuxCapacitor__DM1)
#define AMuxCapacitor_DM2                    (* (reg8 *) AMuxCapacitor__DM2) 
/* Input Buffer Disable Override */
#define AMuxCapacitor_INP_DIS                (* (reg8 *) AMuxCapacitor__INP_DIS)
/* LCD Common or Segment Drive */
#define AMuxCapacitor_LCD_COM_SEG            (* (reg8 *) AMuxCapacitor__LCD_COM_SEG)
/* Enable Segment LCD */
#define AMuxCapacitor_LCD_EN                 (* (reg8 *) AMuxCapacitor__LCD_EN)
/* Slew Rate Control */
#define AMuxCapacitor_SLW                    (* (reg8 *) AMuxCapacitor__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define AMuxCapacitor_PRTDSI__CAPS_SEL       (* (reg8 *) AMuxCapacitor__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define AMuxCapacitor_PRTDSI__DBL_SYNC_IN    (* (reg8 *) AMuxCapacitor__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define AMuxCapacitor_PRTDSI__OE_SEL0        (* (reg8 *) AMuxCapacitor__PRTDSI__OE_SEL0) 
#define AMuxCapacitor_PRTDSI__OE_SEL1        (* (reg8 *) AMuxCapacitor__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define AMuxCapacitor_PRTDSI__OUT_SEL0       (* (reg8 *) AMuxCapacitor__PRTDSI__OUT_SEL0) 
#define AMuxCapacitor_PRTDSI__OUT_SEL1       (* (reg8 *) AMuxCapacitor__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define AMuxCapacitor_PRTDSI__SYNC_OUT       (* (reg8 *) AMuxCapacitor__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(AMuxCapacitor__SIO_CFG)
    #define AMuxCapacitor_SIO_HYST_EN        (* (reg8 *) AMuxCapacitor__SIO_HYST_EN)
    #define AMuxCapacitor_SIO_REG_HIFREQ     (* (reg8 *) AMuxCapacitor__SIO_REG_HIFREQ)
    #define AMuxCapacitor_SIO_CFG            (* (reg8 *) AMuxCapacitor__SIO_CFG)
    #define AMuxCapacitor_SIO_DIFF           (* (reg8 *) AMuxCapacitor__SIO_DIFF)
#endif /* (AMuxCapacitor__SIO_CFG) */

/* Interrupt Registers */
#if defined(AMuxCapacitor__INTSTAT)
    #define AMuxCapacitor_INTSTAT            (* (reg8 *) AMuxCapacitor__INTSTAT)
    #define AMuxCapacitor_SNAP               (* (reg8 *) AMuxCapacitor__SNAP)
    
	#define AMuxCapacitor_0_INTTYPE_REG 		(* (reg8 *) AMuxCapacitor__0__INTTYPE)
#endif /* (AMuxCapacitor__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_AMuxCapacitor_H */


/* [] END OF FILE */
