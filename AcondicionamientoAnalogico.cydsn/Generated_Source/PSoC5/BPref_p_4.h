/*******************************************************************************
* File Name: BPref_p_4.h  
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

#if !defined(CY_PINS_BPref_p_4_H) /* Pins BPref_p_4_H */
#define CY_PINS_BPref_p_4_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "BPref_p_4_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 BPref_p_4__PORT == 15 && ((BPref_p_4__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    BPref_p_4_Write(uint8 value);
void    BPref_p_4_SetDriveMode(uint8 mode);
uint8   BPref_p_4_ReadDataReg(void);
uint8   BPref_p_4_Read(void);
void    BPref_p_4_SetInterruptMode(uint16 position, uint16 mode);
uint8   BPref_p_4_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the BPref_p_4_SetDriveMode() function.
     *  @{
     */
        #define BPref_p_4_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define BPref_p_4_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define BPref_p_4_DM_RES_UP          PIN_DM_RES_UP
        #define BPref_p_4_DM_RES_DWN         PIN_DM_RES_DWN
        #define BPref_p_4_DM_OD_LO           PIN_DM_OD_LO
        #define BPref_p_4_DM_OD_HI           PIN_DM_OD_HI
        #define BPref_p_4_DM_STRONG          PIN_DM_STRONG
        #define BPref_p_4_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define BPref_p_4_MASK               BPref_p_4__MASK
#define BPref_p_4_SHIFT              BPref_p_4__SHIFT
#define BPref_p_4_WIDTH              1u

/* Interrupt constants */
#if defined(BPref_p_4__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in BPref_p_4_SetInterruptMode() function.
     *  @{
     */
        #define BPref_p_4_INTR_NONE      (uint16)(0x0000u)
        #define BPref_p_4_INTR_RISING    (uint16)(0x0001u)
        #define BPref_p_4_INTR_FALLING   (uint16)(0x0002u)
        #define BPref_p_4_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define BPref_p_4_INTR_MASK      (0x01u) 
#endif /* (BPref_p_4__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define BPref_p_4_PS                     (* (reg8 *) BPref_p_4__PS)
/* Data Register */
#define BPref_p_4_DR                     (* (reg8 *) BPref_p_4__DR)
/* Port Number */
#define BPref_p_4_PRT_NUM                (* (reg8 *) BPref_p_4__PRT) 
/* Connect to Analog Globals */                                                  
#define BPref_p_4_AG                     (* (reg8 *) BPref_p_4__AG)                       
/* Analog MUX bux enable */
#define BPref_p_4_AMUX                   (* (reg8 *) BPref_p_4__AMUX) 
/* Bidirectional Enable */                                                        
#define BPref_p_4_BIE                    (* (reg8 *) BPref_p_4__BIE)
/* Bit-mask for Aliased Register Access */
#define BPref_p_4_BIT_MASK               (* (reg8 *) BPref_p_4__BIT_MASK)
/* Bypass Enable */
#define BPref_p_4_BYP                    (* (reg8 *) BPref_p_4__BYP)
/* Port wide control signals */                                                   
#define BPref_p_4_CTL                    (* (reg8 *) BPref_p_4__CTL)
/* Drive Modes */
#define BPref_p_4_DM0                    (* (reg8 *) BPref_p_4__DM0) 
#define BPref_p_4_DM1                    (* (reg8 *) BPref_p_4__DM1)
#define BPref_p_4_DM2                    (* (reg8 *) BPref_p_4__DM2) 
/* Input Buffer Disable Override */
#define BPref_p_4_INP_DIS                (* (reg8 *) BPref_p_4__INP_DIS)
/* LCD Common or Segment Drive */
#define BPref_p_4_LCD_COM_SEG            (* (reg8 *) BPref_p_4__LCD_COM_SEG)
/* Enable Segment LCD */
#define BPref_p_4_LCD_EN                 (* (reg8 *) BPref_p_4__LCD_EN)
/* Slew Rate Control */
#define BPref_p_4_SLW                    (* (reg8 *) BPref_p_4__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define BPref_p_4_PRTDSI__CAPS_SEL       (* (reg8 *) BPref_p_4__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define BPref_p_4_PRTDSI__DBL_SYNC_IN    (* (reg8 *) BPref_p_4__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define BPref_p_4_PRTDSI__OE_SEL0        (* (reg8 *) BPref_p_4__PRTDSI__OE_SEL0) 
#define BPref_p_4_PRTDSI__OE_SEL1        (* (reg8 *) BPref_p_4__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define BPref_p_4_PRTDSI__OUT_SEL0       (* (reg8 *) BPref_p_4__PRTDSI__OUT_SEL0) 
#define BPref_p_4_PRTDSI__OUT_SEL1       (* (reg8 *) BPref_p_4__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define BPref_p_4_PRTDSI__SYNC_OUT       (* (reg8 *) BPref_p_4__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(BPref_p_4__SIO_CFG)
    #define BPref_p_4_SIO_HYST_EN        (* (reg8 *) BPref_p_4__SIO_HYST_EN)
    #define BPref_p_4_SIO_REG_HIFREQ     (* (reg8 *) BPref_p_4__SIO_REG_HIFREQ)
    #define BPref_p_4_SIO_CFG            (* (reg8 *) BPref_p_4__SIO_CFG)
    #define BPref_p_4_SIO_DIFF           (* (reg8 *) BPref_p_4__SIO_DIFF)
#endif /* (BPref_p_4__SIO_CFG) */

/* Interrupt Registers */
#if defined(BPref_p_4__INTSTAT)
    #define BPref_p_4_INTSTAT            (* (reg8 *) BPref_p_4__INTSTAT)
    #define BPref_p_4_SNAP               (* (reg8 *) BPref_p_4__SNAP)
    
	#define BPref_p_4_0_INTTYPE_REG 		(* (reg8 *) BPref_p_4__0__INTTYPE)
#endif /* (BPref_p_4__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_BPref_p_4_H */


/* [] END OF FILE */
