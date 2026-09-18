/*******************************************************************************
* File Name: PGApin.h  
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

#if !defined(CY_PINS_PGApin_H) /* Pins PGApin_H */
#define CY_PINS_PGApin_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "PGApin_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 PGApin__PORT == 15 && ((PGApin__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    PGApin_Write(uint8 value);
void    PGApin_SetDriveMode(uint8 mode);
uint8   PGApin_ReadDataReg(void);
uint8   PGApin_Read(void);
void    PGApin_SetInterruptMode(uint16 position, uint16 mode);
uint8   PGApin_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the PGApin_SetDriveMode() function.
     *  @{
     */
        #define PGApin_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define PGApin_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define PGApin_DM_RES_UP          PIN_DM_RES_UP
        #define PGApin_DM_RES_DWN         PIN_DM_RES_DWN
        #define PGApin_DM_OD_LO           PIN_DM_OD_LO
        #define PGApin_DM_OD_HI           PIN_DM_OD_HI
        #define PGApin_DM_STRONG          PIN_DM_STRONG
        #define PGApin_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define PGApin_MASK               PGApin__MASK
#define PGApin_SHIFT              PGApin__SHIFT
#define PGApin_WIDTH              1u

/* Interrupt constants */
#if defined(PGApin__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in PGApin_SetInterruptMode() function.
     *  @{
     */
        #define PGApin_INTR_NONE      (uint16)(0x0000u)
        #define PGApin_INTR_RISING    (uint16)(0x0001u)
        #define PGApin_INTR_FALLING   (uint16)(0x0002u)
        #define PGApin_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define PGApin_INTR_MASK      (0x01u) 
#endif /* (PGApin__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define PGApin_PS                     (* (reg8 *) PGApin__PS)
/* Data Register */
#define PGApin_DR                     (* (reg8 *) PGApin__DR)
/* Port Number */
#define PGApin_PRT_NUM                (* (reg8 *) PGApin__PRT) 
/* Connect to Analog Globals */                                                  
#define PGApin_AG                     (* (reg8 *) PGApin__AG)                       
/* Analog MUX bux enable */
#define PGApin_AMUX                   (* (reg8 *) PGApin__AMUX) 
/* Bidirectional Enable */                                                        
#define PGApin_BIE                    (* (reg8 *) PGApin__BIE)
/* Bit-mask for Aliased Register Access */
#define PGApin_BIT_MASK               (* (reg8 *) PGApin__BIT_MASK)
/* Bypass Enable */
#define PGApin_BYP                    (* (reg8 *) PGApin__BYP)
/* Port wide control signals */                                                   
#define PGApin_CTL                    (* (reg8 *) PGApin__CTL)
/* Drive Modes */
#define PGApin_DM0                    (* (reg8 *) PGApin__DM0) 
#define PGApin_DM1                    (* (reg8 *) PGApin__DM1)
#define PGApin_DM2                    (* (reg8 *) PGApin__DM2) 
/* Input Buffer Disable Override */
#define PGApin_INP_DIS                (* (reg8 *) PGApin__INP_DIS)
/* LCD Common or Segment Drive */
#define PGApin_LCD_COM_SEG            (* (reg8 *) PGApin__LCD_COM_SEG)
/* Enable Segment LCD */
#define PGApin_LCD_EN                 (* (reg8 *) PGApin__LCD_EN)
/* Slew Rate Control */
#define PGApin_SLW                    (* (reg8 *) PGApin__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define PGApin_PRTDSI__CAPS_SEL       (* (reg8 *) PGApin__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define PGApin_PRTDSI__DBL_SYNC_IN    (* (reg8 *) PGApin__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define PGApin_PRTDSI__OE_SEL0        (* (reg8 *) PGApin__PRTDSI__OE_SEL0) 
#define PGApin_PRTDSI__OE_SEL1        (* (reg8 *) PGApin__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define PGApin_PRTDSI__OUT_SEL0       (* (reg8 *) PGApin__PRTDSI__OUT_SEL0) 
#define PGApin_PRTDSI__OUT_SEL1       (* (reg8 *) PGApin__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define PGApin_PRTDSI__SYNC_OUT       (* (reg8 *) PGApin__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(PGApin__SIO_CFG)
    #define PGApin_SIO_HYST_EN        (* (reg8 *) PGApin__SIO_HYST_EN)
    #define PGApin_SIO_REG_HIFREQ     (* (reg8 *) PGApin__SIO_REG_HIFREQ)
    #define PGApin_SIO_CFG            (* (reg8 *) PGApin__SIO_CFG)
    #define PGApin_SIO_DIFF           (* (reg8 *) PGApin__SIO_DIFF)
#endif /* (PGApin__SIO_CFG) */

/* Interrupt Registers */
#if defined(PGApin__INTSTAT)
    #define PGApin_INTSTAT            (* (reg8 *) PGApin__INTSTAT)
    #define PGApin_SNAP               (* (reg8 *) PGApin__SNAP)
    
	#define PGApin_0_INTTYPE_REG 		(* (reg8 *) PGApin__0__INTTYPE)
#endif /* (PGApin__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_PGApin_H */


/* [] END OF FILE */
