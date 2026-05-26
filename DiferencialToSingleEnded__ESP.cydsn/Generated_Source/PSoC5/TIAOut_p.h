/*******************************************************************************
* File Name: TIAOut_p.h  
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

#if !defined(CY_PINS_TIAOut_p_H) /* Pins TIAOut_p_H */
#define CY_PINS_TIAOut_p_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "TIAOut_p_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 TIAOut_p__PORT == 15 && ((TIAOut_p__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    TIAOut_p_Write(uint8 value);
void    TIAOut_p_SetDriveMode(uint8 mode);
uint8   TIAOut_p_ReadDataReg(void);
uint8   TIAOut_p_Read(void);
void    TIAOut_p_SetInterruptMode(uint16 position, uint16 mode);
uint8   TIAOut_p_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the TIAOut_p_SetDriveMode() function.
     *  @{
     */
        #define TIAOut_p_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define TIAOut_p_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define TIAOut_p_DM_RES_UP          PIN_DM_RES_UP
        #define TIAOut_p_DM_RES_DWN         PIN_DM_RES_DWN
        #define TIAOut_p_DM_OD_LO           PIN_DM_OD_LO
        #define TIAOut_p_DM_OD_HI           PIN_DM_OD_HI
        #define TIAOut_p_DM_STRONG          PIN_DM_STRONG
        #define TIAOut_p_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define TIAOut_p_MASK               TIAOut_p__MASK
#define TIAOut_p_SHIFT              TIAOut_p__SHIFT
#define TIAOut_p_WIDTH              1u

/* Interrupt constants */
#if defined(TIAOut_p__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in TIAOut_p_SetInterruptMode() function.
     *  @{
     */
        #define TIAOut_p_INTR_NONE      (uint16)(0x0000u)
        #define TIAOut_p_INTR_RISING    (uint16)(0x0001u)
        #define TIAOut_p_INTR_FALLING   (uint16)(0x0002u)
        #define TIAOut_p_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define TIAOut_p_INTR_MASK      (0x01u) 
#endif /* (TIAOut_p__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define TIAOut_p_PS                     (* (reg8 *) TIAOut_p__PS)
/* Data Register */
#define TIAOut_p_DR                     (* (reg8 *) TIAOut_p__DR)
/* Port Number */
#define TIAOut_p_PRT_NUM                (* (reg8 *) TIAOut_p__PRT) 
/* Connect to Analog Globals */                                                  
#define TIAOut_p_AG                     (* (reg8 *) TIAOut_p__AG)                       
/* Analog MUX bux enable */
#define TIAOut_p_AMUX                   (* (reg8 *) TIAOut_p__AMUX) 
/* Bidirectional Enable */                                                        
#define TIAOut_p_BIE                    (* (reg8 *) TIAOut_p__BIE)
/* Bit-mask for Aliased Register Access */
#define TIAOut_p_BIT_MASK               (* (reg8 *) TIAOut_p__BIT_MASK)
/* Bypass Enable */
#define TIAOut_p_BYP                    (* (reg8 *) TIAOut_p__BYP)
/* Port wide control signals */                                                   
#define TIAOut_p_CTL                    (* (reg8 *) TIAOut_p__CTL)
/* Drive Modes */
#define TIAOut_p_DM0                    (* (reg8 *) TIAOut_p__DM0) 
#define TIAOut_p_DM1                    (* (reg8 *) TIAOut_p__DM1)
#define TIAOut_p_DM2                    (* (reg8 *) TIAOut_p__DM2) 
/* Input Buffer Disable Override */
#define TIAOut_p_INP_DIS                (* (reg8 *) TIAOut_p__INP_DIS)
/* LCD Common or Segment Drive */
#define TIAOut_p_LCD_COM_SEG            (* (reg8 *) TIAOut_p__LCD_COM_SEG)
/* Enable Segment LCD */
#define TIAOut_p_LCD_EN                 (* (reg8 *) TIAOut_p__LCD_EN)
/* Slew Rate Control */
#define TIAOut_p_SLW                    (* (reg8 *) TIAOut_p__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define TIAOut_p_PRTDSI__CAPS_SEL       (* (reg8 *) TIAOut_p__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define TIAOut_p_PRTDSI__DBL_SYNC_IN    (* (reg8 *) TIAOut_p__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define TIAOut_p_PRTDSI__OE_SEL0        (* (reg8 *) TIAOut_p__PRTDSI__OE_SEL0) 
#define TIAOut_p_PRTDSI__OE_SEL1        (* (reg8 *) TIAOut_p__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define TIAOut_p_PRTDSI__OUT_SEL0       (* (reg8 *) TIAOut_p__PRTDSI__OUT_SEL0) 
#define TIAOut_p_PRTDSI__OUT_SEL1       (* (reg8 *) TIAOut_p__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define TIAOut_p_PRTDSI__SYNC_OUT       (* (reg8 *) TIAOut_p__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(TIAOut_p__SIO_CFG)
    #define TIAOut_p_SIO_HYST_EN        (* (reg8 *) TIAOut_p__SIO_HYST_EN)
    #define TIAOut_p_SIO_REG_HIFREQ     (* (reg8 *) TIAOut_p__SIO_REG_HIFREQ)
    #define TIAOut_p_SIO_CFG            (* (reg8 *) TIAOut_p__SIO_CFG)
    #define TIAOut_p_SIO_DIFF           (* (reg8 *) TIAOut_p__SIO_DIFF)
#endif /* (TIAOut_p__SIO_CFG) */

/* Interrupt Registers */
#if defined(TIAOut_p__INTSTAT)
    #define TIAOut_p_INTSTAT            (* (reg8 *) TIAOut_p__INTSTAT)
    #define TIAOut_p_SNAP               (* (reg8 *) TIAOut_p__SNAP)
    
	#define TIAOut_p_0_INTTYPE_REG 		(* (reg8 *) TIAOut_p__0__INTTYPE)
#endif /* (TIAOut_p__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_TIAOut_p_H */


/* [] END OF FILE */
