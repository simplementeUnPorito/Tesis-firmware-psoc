/*******************************************************************************
* File Name: TIAout.h  
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

#if !defined(CY_PINS_TIAout_H) /* Pins TIAout_H */
#define CY_PINS_TIAout_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "TIAout_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 TIAout__PORT == 15 && ((TIAout__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    TIAout_Write(uint8 value);
void    TIAout_SetDriveMode(uint8 mode);
uint8   TIAout_ReadDataReg(void);
uint8   TIAout_Read(void);
void    TIAout_SetInterruptMode(uint16 position, uint16 mode);
uint8   TIAout_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the TIAout_SetDriveMode() function.
     *  @{
     */
        #define TIAout_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define TIAout_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define TIAout_DM_RES_UP          PIN_DM_RES_UP
        #define TIAout_DM_RES_DWN         PIN_DM_RES_DWN
        #define TIAout_DM_OD_LO           PIN_DM_OD_LO
        #define TIAout_DM_OD_HI           PIN_DM_OD_HI
        #define TIAout_DM_STRONG          PIN_DM_STRONG
        #define TIAout_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define TIAout_MASK               TIAout__MASK
#define TIAout_SHIFT              TIAout__SHIFT
#define TIAout_WIDTH              1u

/* Interrupt constants */
#if defined(TIAout__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in TIAout_SetInterruptMode() function.
     *  @{
     */
        #define TIAout_INTR_NONE      (uint16)(0x0000u)
        #define TIAout_INTR_RISING    (uint16)(0x0001u)
        #define TIAout_INTR_FALLING   (uint16)(0x0002u)
        #define TIAout_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define TIAout_INTR_MASK      (0x01u) 
#endif /* (TIAout__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define TIAout_PS                     (* (reg8 *) TIAout__PS)
/* Data Register */
#define TIAout_DR                     (* (reg8 *) TIAout__DR)
/* Port Number */
#define TIAout_PRT_NUM                (* (reg8 *) TIAout__PRT) 
/* Connect to Analog Globals */                                                  
#define TIAout_AG                     (* (reg8 *) TIAout__AG)                       
/* Analog MUX bux enable */
#define TIAout_AMUX                   (* (reg8 *) TIAout__AMUX) 
/* Bidirectional Enable */                                                        
#define TIAout_BIE                    (* (reg8 *) TIAout__BIE)
/* Bit-mask for Aliased Register Access */
#define TIAout_BIT_MASK               (* (reg8 *) TIAout__BIT_MASK)
/* Bypass Enable */
#define TIAout_BYP                    (* (reg8 *) TIAout__BYP)
/* Port wide control signals */                                                   
#define TIAout_CTL                    (* (reg8 *) TIAout__CTL)
/* Drive Modes */
#define TIAout_DM0                    (* (reg8 *) TIAout__DM0) 
#define TIAout_DM1                    (* (reg8 *) TIAout__DM1)
#define TIAout_DM2                    (* (reg8 *) TIAout__DM2) 
/* Input Buffer Disable Override */
#define TIAout_INP_DIS                (* (reg8 *) TIAout__INP_DIS)
/* LCD Common or Segment Drive */
#define TIAout_LCD_COM_SEG            (* (reg8 *) TIAout__LCD_COM_SEG)
/* Enable Segment LCD */
#define TIAout_LCD_EN                 (* (reg8 *) TIAout__LCD_EN)
/* Slew Rate Control */
#define TIAout_SLW                    (* (reg8 *) TIAout__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define TIAout_PRTDSI__CAPS_SEL       (* (reg8 *) TIAout__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define TIAout_PRTDSI__DBL_SYNC_IN    (* (reg8 *) TIAout__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define TIAout_PRTDSI__OE_SEL0        (* (reg8 *) TIAout__PRTDSI__OE_SEL0) 
#define TIAout_PRTDSI__OE_SEL1        (* (reg8 *) TIAout__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define TIAout_PRTDSI__OUT_SEL0       (* (reg8 *) TIAout__PRTDSI__OUT_SEL0) 
#define TIAout_PRTDSI__OUT_SEL1       (* (reg8 *) TIAout__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define TIAout_PRTDSI__SYNC_OUT       (* (reg8 *) TIAout__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(TIAout__SIO_CFG)
    #define TIAout_SIO_HYST_EN        (* (reg8 *) TIAout__SIO_HYST_EN)
    #define TIAout_SIO_REG_HIFREQ     (* (reg8 *) TIAout__SIO_REG_HIFREQ)
    #define TIAout_SIO_CFG            (* (reg8 *) TIAout__SIO_CFG)
    #define TIAout_SIO_DIFF           (* (reg8 *) TIAout__SIO_DIFF)
#endif /* (TIAout__SIO_CFG) */

/* Interrupt Registers */
#if defined(TIAout__INTSTAT)
    #define TIAout_INTSTAT            (* (reg8 *) TIAout__INTSTAT)
    #define TIAout_SNAP               (* (reg8 *) TIAout__SNAP)
    
	#define TIAout_0_INTTYPE_REG 		(* (reg8 *) TIAout__0__INTTYPE)
#endif /* (TIAout__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_TIAout_H */


/* [] END OF FILE */
