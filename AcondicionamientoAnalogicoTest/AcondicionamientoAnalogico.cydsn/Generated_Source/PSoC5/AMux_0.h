/*******************************************************************************
* File Name: AMux_0.h  
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

#if !defined(CY_PINS_AMux_0_H) /* Pins AMux_0_H */
#define CY_PINS_AMux_0_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "AMux_0_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 AMux_0__PORT == 15 && ((AMux_0__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    AMux_0_Write(uint8 value);
void    AMux_0_SetDriveMode(uint8 mode);
uint8   AMux_0_ReadDataReg(void);
uint8   AMux_0_Read(void);
void    AMux_0_SetInterruptMode(uint16 position, uint16 mode);
uint8   AMux_0_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the AMux_0_SetDriveMode() function.
     *  @{
     */
        #define AMux_0_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define AMux_0_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define AMux_0_DM_RES_UP          PIN_DM_RES_UP
        #define AMux_0_DM_RES_DWN         PIN_DM_RES_DWN
        #define AMux_0_DM_OD_LO           PIN_DM_OD_LO
        #define AMux_0_DM_OD_HI           PIN_DM_OD_HI
        #define AMux_0_DM_STRONG          PIN_DM_STRONG
        #define AMux_0_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define AMux_0_MASK               AMux_0__MASK
#define AMux_0_SHIFT              AMux_0__SHIFT
#define AMux_0_WIDTH              1u

/* Interrupt constants */
#if defined(AMux_0__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in AMux_0_SetInterruptMode() function.
     *  @{
     */
        #define AMux_0_INTR_NONE      (uint16)(0x0000u)
        #define AMux_0_INTR_RISING    (uint16)(0x0001u)
        #define AMux_0_INTR_FALLING   (uint16)(0x0002u)
        #define AMux_0_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define AMux_0_INTR_MASK      (0x01u) 
#endif /* (AMux_0__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define AMux_0_PS                     (* (reg8 *) AMux_0__PS)
/* Data Register */
#define AMux_0_DR                     (* (reg8 *) AMux_0__DR)
/* Port Number */
#define AMux_0_PRT_NUM                (* (reg8 *) AMux_0__PRT) 
/* Connect to Analog Globals */                                                  
#define AMux_0_AG                     (* (reg8 *) AMux_0__AG)                       
/* Analog MUX bux enable */
#define AMux_0_AMUX                   (* (reg8 *) AMux_0__AMUX) 
/* Bidirectional Enable */                                                        
#define AMux_0_BIE                    (* (reg8 *) AMux_0__BIE)
/* Bit-mask for Aliased Register Access */
#define AMux_0_BIT_MASK               (* (reg8 *) AMux_0__BIT_MASK)
/* Bypass Enable */
#define AMux_0_BYP                    (* (reg8 *) AMux_0__BYP)
/* Port wide control signals */                                                   
#define AMux_0_CTL                    (* (reg8 *) AMux_0__CTL)
/* Drive Modes */
#define AMux_0_DM0                    (* (reg8 *) AMux_0__DM0) 
#define AMux_0_DM1                    (* (reg8 *) AMux_0__DM1)
#define AMux_0_DM2                    (* (reg8 *) AMux_0__DM2) 
/* Input Buffer Disable Override */
#define AMux_0_INP_DIS                (* (reg8 *) AMux_0__INP_DIS)
/* LCD Common or Segment Drive */
#define AMux_0_LCD_COM_SEG            (* (reg8 *) AMux_0__LCD_COM_SEG)
/* Enable Segment LCD */
#define AMux_0_LCD_EN                 (* (reg8 *) AMux_0__LCD_EN)
/* Slew Rate Control */
#define AMux_0_SLW                    (* (reg8 *) AMux_0__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define AMux_0_PRTDSI__CAPS_SEL       (* (reg8 *) AMux_0__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define AMux_0_PRTDSI__DBL_SYNC_IN    (* (reg8 *) AMux_0__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define AMux_0_PRTDSI__OE_SEL0        (* (reg8 *) AMux_0__PRTDSI__OE_SEL0) 
#define AMux_0_PRTDSI__OE_SEL1        (* (reg8 *) AMux_0__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define AMux_0_PRTDSI__OUT_SEL0       (* (reg8 *) AMux_0__PRTDSI__OUT_SEL0) 
#define AMux_0_PRTDSI__OUT_SEL1       (* (reg8 *) AMux_0__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define AMux_0_PRTDSI__SYNC_OUT       (* (reg8 *) AMux_0__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(AMux_0__SIO_CFG)
    #define AMux_0_SIO_HYST_EN        (* (reg8 *) AMux_0__SIO_HYST_EN)
    #define AMux_0_SIO_REG_HIFREQ     (* (reg8 *) AMux_0__SIO_REG_HIFREQ)
    #define AMux_0_SIO_CFG            (* (reg8 *) AMux_0__SIO_CFG)
    #define AMux_0_SIO_DIFF           (* (reg8 *) AMux_0__SIO_DIFF)
#endif /* (AMux_0__SIO_CFG) */

/* Interrupt Registers */
#if defined(AMux_0__INTSTAT)
    #define AMux_0_INTSTAT            (* (reg8 *) AMux_0__INTSTAT)
    #define AMux_0_SNAP               (* (reg8 *) AMux_0__SNAP)
    
	#define AMux_0_0_INTTYPE_REG 		(* (reg8 *) AMux_0__0__INTTYPE)
#endif /* (AMux_0__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_AMux_0_H */


/* [] END OF FILE */
