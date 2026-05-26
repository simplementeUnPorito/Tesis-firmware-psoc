/*******************************************************************************
* File Name: BPp.h  
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

#if !defined(CY_PINS_BPp_H) /* Pins BPp_H */
#define CY_PINS_BPp_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "BPp_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 BPp__PORT == 15 && ((BPp__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    BPp_Write(uint8 value);
void    BPp_SetDriveMode(uint8 mode);
uint8   BPp_ReadDataReg(void);
uint8   BPp_Read(void);
void    BPp_SetInterruptMode(uint16 position, uint16 mode);
uint8   BPp_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the BPp_SetDriveMode() function.
     *  @{
     */
        #define BPp_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define BPp_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define BPp_DM_RES_UP          PIN_DM_RES_UP
        #define BPp_DM_RES_DWN         PIN_DM_RES_DWN
        #define BPp_DM_OD_LO           PIN_DM_OD_LO
        #define BPp_DM_OD_HI           PIN_DM_OD_HI
        #define BPp_DM_STRONG          PIN_DM_STRONG
        #define BPp_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define BPp_MASK               BPp__MASK
#define BPp_SHIFT              BPp__SHIFT
#define BPp_WIDTH              1u

/* Interrupt constants */
#if defined(BPp__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in BPp_SetInterruptMode() function.
     *  @{
     */
        #define BPp_INTR_NONE      (uint16)(0x0000u)
        #define BPp_INTR_RISING    (uint16)(0x0001u)
        #define BPp_INTR_FALLING   (uint16)(0x0002u)
        #define BPp_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define BPp_INTR_MASK      (0x01u) 
#endif /* (BPp__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define BPp_PS                     (* (reg8 *) BPp__PS)
/* Data Register */
#define BPp_DR                     (* (reg8 *) BPp__DR)
/* Port Number */
#define BPp_PRT_NUM                (* (reg8 *) BPp__PRT) 
/* Connect to Analog Globals */                                                  
#define BPp_AG                     (* (reg8 *) BPp__AG)                       
/* Analog MUX bux enable */
#define BPp_AMUX                   (* (reg8 *) BPp__AMUX) 
/* Bidirectional Enable */                                                        
#define BPp_BIE                    (* (reg8 *) BPp__BIE)
/* Bit-mask for Aliased Register Access */
#define BPp_BIT_MASK               (* (reg8 *) BPp__BIT_MASK)
/* Bypass Enable */
#define BPp_BYP                    (* (reg8 *) BPp__BYP)
/* Port wide control signals */                                                   
#define BPp_CTL                    (* (reg8 *) BPp__CTL)
/* Drive Modes */
#define BPp_DM0                    (* (reg8 *) BPp__DM0) 
#define BPp_DM1                    (* (reg8 *) BPp__DM1)
#define BPp_DM2                    (* (reg8 *) BPp__DM2) 
/* Input Buffer Disable Override */
#define BPp_INP_DIS                (* (reg8 *) BPp__INP_DIS)
/* LCD Common or Segment Drive */
#define BPp_LCD_COM_SEG            (* (reg8 *) BPp__LCD_COM_SEG)
/* Enable Segment LCD */
#define BPp_LCD_EN                 (* (reg8 *) BPp__LCD_EN)
/* Slew Rate Control */
#define BPp_SLW                    (* (reg8 *) BPp__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define BPp_PRTDSI__CAPS_SEL       (* (reg8 *) BPp__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define BPp_PRTDSI__DBL_SYNC_IN    (* (reg8 *) BPp__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define BPp_PRTDSI__OE_SEL0        (* (reg8 *) BPp__PRTDSI__OE_SEL0) 
#define BPp_PRTDSI__OE_SEL1        (* (reg8 *) BPp__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define BPp_PRTDSI__OUT_SEL0       (* (reg8 *) BPp__PRTDSI__OUT_SEL0) 
#define BPp_PRTDSI__OUT_SEL1       (* (reg8 *) BPp__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define BPp_PRTDSI__SYNC_OUT       (* (reg8 *) BPp__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(BPp__SIO_CFG)
    #define BPp_SIO_HYST_EN        (* (reg8 *) BPp__SIO_HYST_EN)
    #define BPp_SIO_REG_HIFREQ     (* (reg8 *) BPp__SIO_REG_HIFREQ)
    #define BPp_SIO_CFG            (* (reg8 *) BPp__SIO_CFG)
    #define BPp_SIO_DIFF           (* (reg8 *) BPp__SIO_DIFF)
#endif /* (BPp__SIO_CFG) */

/* Interrupt Registers */
#if defined(BPp__INTSTAT)
    #define BPp_INTSTAT            (* (reg8 *) BPp__INTSTAT)
    #define BPp_SNAP               (* (reg8 *) BPp__SNAP)
    
	#define BPp_0_INTTYPE_REG 		(* (reg8 *) BPp__0__INTTYPE)
#endif /* (BPp__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_BPp_H */


/* [] END OF FILE */
