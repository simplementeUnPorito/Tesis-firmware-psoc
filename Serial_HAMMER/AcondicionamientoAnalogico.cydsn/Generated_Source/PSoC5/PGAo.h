/*******************************************************************************
* File Name: PGAo.h  
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

#if !defined(CY_PINS_PGAo_H) /* Pins PGAo_H */
#define CY_PINS_PGAo_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "PGAo_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 PGAo__PORT == 15 && ((PGAo__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    PGAo_Write(uint8 value);
void    PGAo_SetDriveMode(uint8 mode);
uint8   PGAo_ReadDataReg(void);
uint8   PGAo_Read(void);
void    PGAo_SetInterruptMode(uint16 position, uint16 mode);
uint8   PGAo_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the PGAo_SetDriveMode() function.
     *  @{
     */
        #define PGAo_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define PGAo_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define PGAo_DM_RES_UP          PIN_DM_RES_UP
        #define PGAo_DM_RES_DWN         PIN_DM_RES_DWN
        #define PGAo_DM_OD_LO           PIN_DM_OD_LO
        #define PGAo_DM_OD_HI           PIN_DM_OD_HI
        #define PGAo_DM_STRONG          PIN_DM_STRONG
        #define PGAo_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define PGAo_MASK               PGAo__MASK
#define PGAo_SHIFT              PGAo__SHIFT
#define PGAo_WIDTH              1u

/* Interrupt constants */
#if defined(PGAo__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in PGAo_SetInterruptMode() function.
     *  @{
     */
        #define PGAo_INTR_NONE      (uint16)(0x0000u)
        #define PGAo_INTR_RISING    (uint16)(0x0001u)
        #define PGAo_INTR_FALLING   (uint16)(0x0002u)
        #define PGAo_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define PGAo_INTR_MASK      (0x01u) 
#endif /* (PGAo__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define PGAo_PS                     (* (reg8 *) PGAo__PS)
/* Data Register */
#define PGAo_DR                     (* (reg8 *) PGAo__DR)
/* Port Number */
#define PGAo_PRT_NUM                (* (reg8 *) PGAo__PRT) 
/* Connect to Analog Globals */                                                  
#define PGAo_AG                     (* (reg8 *) PGAo__AG)                       
/* Analog MUX bux enable */
#define PGAo_AMUX                   (* (reg8 *) PGAo__AMUX) 
/* Bidirectional Enable */                                                        
#define PGAo_BIE                    (* (reg8 *) PGAo__BIE)
/* Bit-mask for Aliased Register Access */
#define PGAo_BIT_MASK               (* (reg8 *) PGAo__BIT_MASK)
/* Bypass Enable */
#define PGAo_BYP                    (* (reg8 *) PGAo__BYP)
/* Port wide control signals */                                                   
#define PGAo_CTL                    (* (reg8 *) PGAo__CTL)
/* Drive Modes */
#define PGAo_DM0                    (* (reg8 *) PGAo__DM0) 
#define PGAo_DM1                    (* (reg8 *) PGAo__DM1)
#define PGAo_DM2                    (* (reg8 *) PGAo__DM2) 
/* Input Buffer Disable Override */
#define PGAo_INP_DIS                (* (reg8 *) PGAo__INP_DIS)
/* LCD Common or Segment Drive */
#define PGAo_LCD_COM_SEG            (* (reg8 *) PGAo__LCD_COM_SEG)
/* Enable Segment LCD */
#define PGAo_LCD_EN                 (* (reg8 *) PGAo__LCD_EN)
/* Slew Rate Control */
#define PGAo_SLW                    (* (reg8 *) PGAo__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define PGAo_PRTDSI__CAPS_SEL       (* (reg8 *) PGAo__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define PGAo_PRTDSI__DBL_SYNC_IN    (* (reg8 *) PGAo__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define PGAo_PRTDSI__OE_SEL0        (* (reg8 *) PGAo__PRTDSI__OE_SEL0) 
#define PGAo_PRTDSI__OE_SEL1        (* (reg8 *) PGAo__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define PGAo_PRTDSI__OUT_SEL0       (* (reg8 *) PGAo__PRTDSI__OUT_SEL0) 
#define PGAo_PRTDSI__OUT_SEL1       (* (reg8 *) PGAo__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define PGAo_PRTDSI__SYNC_OUT       (* (reg8 *) PGAo__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(PGAo__SIO_CFG)
    #define PGAo_SIO_HYST_EN        (* (reg8 *) PGAo__SIO_HYST_EN)
    #define PGAo_SIO_REG_HIFREQ     (* (reg8 *) PGAo__SIO_REG_HIFREQ)
    #define PGAo_SIO_CFG            (* (reg8 *) PGAo__SIO_CFG)
    #define PGAo_SIO_DIFF           (* (reg8 *) PGAo__SIO_DIFF)
#endif /* (PGAo__SIO_CFG) */

/* Interrupt Registers */
#if defined(PGAo__INTSTAT)
    #define PGAo_INTSTAT            (* (reg8 *) PGAo__INTSTAT)
    #define PGAo_SNAP               (* (reg8 *) PGAo__SNAP)
    
	#define PGAo_0_INTTYPE_REG 		(* (reg8 *) PGAo__0__INTTYPE)
#endif /* (PGAo__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_PGAo_H */


/* [] END OF FILE */
