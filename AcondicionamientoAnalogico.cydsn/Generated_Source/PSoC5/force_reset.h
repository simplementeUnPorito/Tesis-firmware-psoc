/*******************************************************************************
* File Name: force_reset.h  
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

#if !defined(CY_PINS_force_reset_H) /* Pins force_reset_H */
#define CY_PINS_force_reset_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "force_reset_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 force_reset__PORT == 15 && ((force_reset__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    force_reset_Write(uint8 value);
void    force_reset_SetDriveMode(uint8 mode);
uint8   force_reset_ReadDataReg(void);
uint8   force_reset_Read(void);
void    force_reset_SetInterruptMode(uint16 position, uint16 mode);
uint8   force_reset_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the force_reset_SetDriveMode() function.
     *  @{
     */
        #define force_reset_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define force_reset_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define force_reset_DM_RES_UP          PIN_DM_RES_UP
        #define force_reset_DM_RES_DWN         PIN_DM_RES_DWN
        #define force_reset_DM_OD_LO           PIN_DM_OD_LO
        #define force_reset_DM_OD_HI           PIN_DM_OD_HI
        #define force_reset_DM_STRONG          PIN_DM_STRONG
        #define force_reset_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define force_reset_MASK               force_reset__MASK
#define force_reset_SHIFT              force_reset__SHIFT
#define force_reset_WIDTH              1u

/* Interrupt constants */
#if defined(force_reset__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in force_reset_SetInterruptMode() function.
     *  @{
     */
        #define force_reset_INTR_NONE      (uint16)(0x0000u)
        #define force_reset_INTR_RISING    (uint16)(0x0001u)
        #define force_reset_INTR_FALLING   (uint16)(0x0002u)
        #define force_reset_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define force_reset_INTR_MASK      (0x01u) 
#endif /* (force_reset__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define force_reset_PS                     (* (reg8 *) force_reset__PS)
/* Data Register */
#define force_reset_DR                     (* (reg8 *) force_reset__DR)
/* Port Number */
#define force_reset_PRT_NUM                (* (reg8 *) force_reset__PRT) 
/* Connect to Analog Globals */                                                  
#define force_reset_AG                     (* (reg8 *) force_reset__AG)                       
/* Analog MUX bux enable */
#define force_reset_AMUX                   (* (reg8 *) force_reset__AMUX) 
/* Bidirectional Enable */                                                        
#define force_reset_BIE                    (* (reg8 *) force_reset__BIE)
/* Bit-mask for Aliased Register Access */
#define force_reset_BIT_MASK               (* (reg8 *) force_reset__BIT_MASK)
/* Bypass Enable */
#define force_reset_BYP                    (* (reg8 *) force_reset__BYP)
/* Port wide control signals */                                                   
#define force_reset_CTL                    (* (reg8 *) force_reset__CTL)
/* Drive Modes */
#define force_reset_DM0                    (* (reg8 *) force_reset__DM0) 
#define force_reset_DM1                    (* (reg8 *) force_reset__DM1)
#define force_reset_DM2                    (* (reg8 *) force_reset__DM2) 
/* Input Buffer Disable Override */
#define force_reset_INP_DIS                (* (reg8 *) force_reset__INP_DIS)
/* LCD Common or Segment Drive */
#define force_reset_LCD_COM_SEG            (* (reg8 *) force_reset__LCD_COM_SEG)
/* Enable Segment LCD */
#define force_reset_LCD_EN                 (* (reg8 *) force_reset__LCD_EN)
/* Slew Rate Control */
#define force_reset_SLW                    (* (reg8 *) force_reset__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define force_reset_PRTDSI__CAPS_SEL       (* (reg8 *) force_reset__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define force_reset_PRTDSI__DBL_SYNC_IN    (* (reg8 *) force_reset__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define force_reset_PRTDSI__OE_SEL0        (* (reg8 *) force_reset__PRTDSI__OE_SEL0) 
#define force_reset_PRTDSI__OE_SEL1        (* (reg8 *) force_reset__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define force_reset_PRTDSI__OUT_SEL0       (* (reg8 *) force_reset__PRTDSI__OUT_SEL0) 
#define force_reset_PRTDSI__OUT_SEL1       (* (reg8 *) force_reset__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define force_reset_PRTDSI__SYNC_OUT       (* (reg8 *) force_reset__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(force_reset__SIO_CFG)
    #define force_reset_SIO_HYST_EN        (* (reg8 *) force_reset__SIO_HYST_EN)
    #define force_reset_SIO_REG_HIFREQ     (* (reg8 *) force_reset__SIO_REG_HIFREQ)
    #define force_reset_SIO_CFG            (* (reg8 *) force_reset__SIO_CFG)
    #define force_reset_SIO_DIFF           (* (reg8 *) force_reset__SIO_DIFF)
#endif /* (force_reset__SIO_CFG) */

/* Interrupt Registers */
#if defined(force_reset__INTSTAT)
    #define force_reset_INTSTAT            (* (reg8 *) force_reset__INTSTAT)
    #define force_reset_SNAP               (* (reg8 *) force_reset__SNAP)
    
	#define force_reset_0_INTTYPE_REG 		(* (reg8 *) force_reset__0__INTTYPE)
#endif /* (force_reset__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_force_reset_H */


/* [] END OF FILE */
