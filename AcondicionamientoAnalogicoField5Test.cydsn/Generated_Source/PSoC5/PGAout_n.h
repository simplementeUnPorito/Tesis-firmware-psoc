/*******************************************************************************
* File Name: PGAout_n.h  
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

#if !defined(CY_PINS_PGAout_n_H) /* Pins PGAout_n_H */
#define CY_PINS_PGAout_n_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "PGAout_n_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 PGAout_n__PORT == 15 && ((PGAout_n__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    PGAout_n_Write(uint8 value);
void    PGAout_n_SetDriveMode(uint8 mode);
uint8   PGAout_n_ReadDataReg(void);
uint8   PGAout_n_Read(void);
void    PGAout_n_SetInterruptMode(uint16 position, uint16 mode);
uint8   PGAout_n_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the PGAout_n_SetDriveMode() function.
     *  @{
     */
        #define PGAout_n_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define PGAout_n_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define PGAout_n_DM_RES_UP          PIN_DM_RES_UP
        #define PGAout_n_DM_RES_DWN         PIN_DM_RES_DWN
        #define PGAout_n_DM_OD_LO           PIN_DM_OD_LO
        #define PGAout_n_DM_OD_HI           PIN_DM_OD_HI
        #define PGAout_n_DM_STRONG          PIN_DM_STRONG
        #define PGAout_n_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define PGAout_n_MASK               PGAout_n__MASK
#define PGAout_n_SHIFT              PGAout_n__SHIFT
#define PGAout_n_WIDTH              1u

/* Interrupt constants */
#if defined(PGAout_n__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in PGAout_n_SetInterruptMode() function.
     *  @{
     */
        #define PGAout_n_INTR_NONE      (uint16)(0x0000u)
        #define PGAout_n_INTR_RISING    (uint16)(0x0001u)
        #define PGAout_n_INTR_FALLING   (uint16)(0x0002u)
        #define PGAout_n_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define PGAout_n_INTR_MASK      (0x01u) 
#endif /* (PGAout_n__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define PGAout_n_PS                     (* (reg8 *) PGAout_n__PS)
/* Data Register */
#define PGAout_n_DR                     (* (reg8 *) PGAout_n__DR)
/* Port Number */
#define PGAout_n_PRT_NUM                (* (reg8 *) PGAout_n__PRT) 
/* Connect to Analog Globals */                                                  
#define PGAout_n_AG                     (* (reg8 *) PGAout_n__AG)                       
/* Analog MUX bux enable */
#define PGAout_n_AMUX                   (* (reg8 *) PGAout_n__AMUX) 
/* Bidirectional Enable */                                                        
#define PGAout_n_BIE                    (* (reg8 *) PGAout_n__BIE)
/* Bit-mask for Aliased Register Access */
#define PGAout_n_BIT_MASK               (* (reg8 *) PGAout_n__BIT_MASK)
/* Bypass Enable */
#define PGAout_n_BYP                    (* (reg8 *) PGAout_n__BYP)
/* Port wide control signals */                                                   
#define PGAout_n_CTL                    (* (reg8 *) PGAout_n__CTL)
/* Drive Modes */
#define PGAout_n_DM0                    (* (reg8 *) PGAout_n__DM0) 
#define PGAout_n_DM1                    (* (reg8 *) PGAout_n__DM1)
#define PGAout_n_DM2                    (* (reg8 *) PGAout_n__DM2) 
/* Input Buffer Disable Override */
#define PGAout_n_INP_DIS                (* (reg8 *) PGAout_n__INP_DIS)
/* LCD Common or Segment Drive */
#define PGAout_n_LCD_COM_SEG            (* (reg8 *) PGAout_n__LCD_COM_SEG)
/* Enable Segment LCD */
#define PGAout_n_LCD_EN                 (* (reg8 *) PGAout_n__LCD_EN)
/* Slew Rate Control */
#define PGAout_n_SLW                    (* (reg8 *) PGAout_n__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define PGAout_n_PRTDSI__CAPS_SEL       (* (reg8 *) PGAout_n__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define PGAout_n_PRTDSI__DBL_SYNC_IN    (* (reg8 *) PGAout_n__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define PGAout_n_PRTDSI__OE_SEL0        (* (reg8 *) PGAout_n__PRTDSI__OE_SEL0) 
#define PGAout_n_PRTDSI__OE_SEL1        (* (reg8 *) PGAout_n__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define PGAout_n_PRTDSI__OUT_SEL0       (* (reg8 *) PGAout_n__PRTDSI__OUT_SEL0) 
#define PGAout_n_PRTDSI__OUT_SEL1       (* (reg8 *) PGAout_n__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define PGAout_n_PRTDSI__SYNC_OUT       (* (reg8 *) PGAout_n__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(PGAout_n__SIO_CFG)
    #define PGAout_n_SIO_HYST_EN        (* (reg8 *) PGAout_n__SIO_HYST_EN)
    #define PGAout_n_SIO_REG_HIFREQ     (* (reg8 *) PGAout_n__SIO_REG_HIFREQ)
    #define PGAout_n_SIO_CFG            (* (reg8 *) PGAout_n__SIO_CFG)
    #define PGAout_n_SIO_DIFF           (* (reg8 *) PGAout_n__SIO_DIFF)
#endif /* (PGAout_n__SIO_CFG) */

/* Interrupt Registers */
#if defined(PGAout_n__INTSTAT)
    #define PGAout_n_INTSTAT            (* (reg8 *) PGAout_n__INTSTAT)
    #define PGAout_n_SNAP               (* (reg8 *) PGAout_n__SNAP)
    
	#define PGAout_n_0_INTTYPE_REG 		(* (reg8 *) PGAout_n__0__INTTYPE)
#endif /* (PGAout_n__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_PGAout_n_H */


/* [] END OF FILE */
