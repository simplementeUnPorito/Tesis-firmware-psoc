/*******************************************************************************
* File Name: AMux.h  
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

#if !defined(CY_PINS_AMux_H) /* Pins AMux_H */
#define CY_PINS_AMux_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "AMux_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 AMux__PORT == 15 && ((AMux__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    AMux_Write(uint8 value);
void    AMux_SetDriveMode(uint8 mode);
uint8   AMux_ReadDataReg(void);
uint8   AMux_Read(void);
void    AMux_SetInterruptMode(uint16 position, uint16 mode);
uint8   AMux_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the AMux_SetDriveMode() function.
     *  @{
     */
        #define AMux_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define AMux_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define AMux_DM_RES_UP          PIN_DM_RES_UP
        #define AMux_DM_RES_DWN         PIN_DM_RES_DWN
        #define AMux_DM_OD_LO           PIN_DM_OD_LO
        #define AMux_DM_OD_HI           PIN_DM_OD_HI
        #define AMux_DM_STRONG          PIN_DM_STRONG
        #define AMux_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define AMux_MASK               AMux__MASK
#define AMux_SHIFT              AMux__SHIFT
#define AMux_WIDTH              1u

/* Interrupt constants */
#if defined(AMux__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in AMux_SetInterruptMode() function.
     *  @{
     */
        #define AMux_INTR_NONE      (uint16)(0x0000u)
        #define AMux_INTR_RISING    (uint16)(0x0001u)
        #define AMux_INTR_FALLING   (uint16)(0x0002u)
        #define AMux_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define AMux_INTR_MASK      (0x01u) 
#endif /* (AMux__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define AMux_PS                     (* (reg8 *) AMux__PS)
/* Data Register */
#define AMux_DR                     (* (reg8 *) AMux__DR)
/* Port Number */
#define AMux_PRT_NUM                (* (reg8 *) AMux__PRT) 
/* Connect to Analog Globals */                                                  
#define AMux_AG                     (* (reg8 *) AMux__AG)                       
/* Analog MUX bux enable */
#define AMux_AMUX                   (* (reg8 *) AMux__AMUX) 
/* Bidirectional Enable */                                                        
#define AMux_BIE                    (* (reg8 *) AMux__BIE)
/* Bit-mask for Aliased Register Access */
#define AMux_BIT_MASK               (* (reg8 *) AMux__BIT_MASK)
/* Bypass Enable */
#define AMux_BYP                    (* (reg8 *) AMux__BYP)
/* Port wide control signals */                                                   
#define AMux_CTL                    (* (reg8 *) AMux__CTL)
/* Drive Modes */
#define AMux_DM0                    (* (reg8 *) AMux__DM0) 
#define AMux_DM1                    (* (reg8 *) AMux__DM1)
#define AMux_DM2                    (* (reg8 *) AMux__DM2) 
/* Input Buffer Disable Override */
#define AMux_INP_DIS                (* (reg8 *) AMux__INP_DIS)
/* LCD Common or Segment Drive */
#define AMux_LCD_COM_SEG            (* (reg8 *) AMux__LCD_COM_SEG)
/* Enable Segment LCD */
#define AMux_LCD_EN                 (* (reg8 *) AMux__LCD_EN)
/* Slew Rate Control */
#define AMux_SLW                    (* (reg8 *) AMux__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define AMux_PRTDSI__CAPS_SEL       (* (reg8 *) AMux__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define AMux_PRTDSI__DBL_SYNC_IN    (* (reg8 *) AMux__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define AMux_PRTDSI__OE_SEL0        (* (reg8 *) AMux__PRTDSI__OE_SEL0) 
#define AMux_PRTDSI__OE_SEL1        (* (reg8 *) AMux__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define AMux_PRTDSI__OUT_SEL0       (* (reg8 *) AMux__PRTDSI__OUT_SEL0) 
#define AMux_PRTDSI__OUT_SEL1       (* (reg8 *) AMux__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define AMux_PRTDSI__SYNC_OUT       (* (reg8 *) AMux__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(AMux__SIO_CFG)
    #define AMux_SIO_HYST_EN        (* (reg8 *) AMux__SIO_HYST_EN)
    #define AMux_SIO_REG_HIFREQ     (* (reg8 *) AMux__SIO_REG_HIFREQ)
    #define AMux_SIO_CFG            (* (reg8 *) AMux__SIO_CFG)
    #define AMux_SIO_DIFF           (* (reg8 *) AMux__SIO_DIFF)
#endif /* (AMux__SIO_CFG) */

/* Interrupt Registers */
#if defined(AMux__INTSTAT)
    #define AMux_INTSTAT            (* (reg8 *) AMux__INTSTAT)
    #define AMux_SNAP               (* (reg8 *) AMux__SNAP)
    
	#define AMux_0_INTTYPE_REG 		(* (reg8 *) AMux__0__INTTYPE)
#endif /* (AMux__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_AMux_H */


/* [] END OF FILE */
