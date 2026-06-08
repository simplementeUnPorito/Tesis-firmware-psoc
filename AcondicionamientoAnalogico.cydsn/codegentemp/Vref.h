/*******************************************************************************
* File Name: Vref.h  
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

#if !defined(CY_PINS_Vref_H) /* Pins Vref_H */
#define CY_PINS_Vref_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "Vref_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 Vref__PORT == 15 && ((Vref__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    Vref_Write(uint8 value);
void    Vref_SetDriveMode(uint8 mode);
uint8   Vref_ReadDataReg(void);
uint8   Vref_Read(void);
void    Vref_SetInterruptMode(uint16 position, uint16 mode);
uint8   Vref_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the Vref_SetDriveMode() function.
     *  @{
     */
        #define Vref_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define Vref_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define Vref_DM_RES_UP          PIN_DM_RES_UP
        #define Vref_DM_RES_DWN         PIN_DM_RES_DWN
        #define Vref_DM_OD_LO           PIN_DM_OD_LO
        #define Vref_DM_OD_HI           PIN_DM_OD_HI
        #define Vref_DM_STRONG          PIN_DM_STRONG
        #define Vref_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define Vref_MASK               Vref__MASK
#define Vref_SHIFT              Vref__SHIFT
#define Vref_WIDTH              1u

/* Interrupt constants */
#if defined(Vref__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in Vref_SetInterruptMode() function.
     *  @{
     */
        #define Vref_INTR_NONE      (uint16)(0x0000u)
        #define Vref_INTR_RISING    (uint16)(0x0001u)
        #define Vref_INTR_FALLING   (uint16)(0x0002u)
        #define Vref_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define Vref_INTR_MASK      (0x01u) 
#endif /* (Vref__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define Vref_PS                     (* (reg8 *) Vref__PS)
/* Data Register */
#define Vref_DR                     (* (reg8 *) Vref__DR)
/* Port Number */
#define Vref_PRT_NUM                (* (reg8 *) Vref__PRT) 
/* Connect to Analog Globals */                                                  
#define Vref_AG                     (* (reg8 *) Vref__AG)                       
/* Analog MUX bux enable */
#define Vref_AMUX                   (* (reg8 *) Vref__AMUX) 
/* Bidirectional Enable */                                                        
#define Vref_BIE                    (* (reg8 *) Vref__BIE)
/* Bit-mask for Aliased Register Access */
#define Vref_BIT_MASK               (* (reg8 *) Vref__BIT_MASK)
/* Bypass Enable */
#define Vref_BYP                    (* (reg8 *) Vref__BYP)
/* Port wide control signals */                                                   
#define Vref_CTL                    (* (reg8 *) Vref__CTL)
/* Drive Modes */
#define Vref_DM0                    (* (reg8 *) Vref__DM0) 
#define Vref_DM1                    (* (reg8 *) Vref__DM1)
#define Vref_DM2                    (* (reg8 *) Vref__DM2) 
/* Input Buffer Disable Override */
#define Vref_INP_DIS                (* (reg8 *) Vref__INP_DIS)
/* LCD Common or Segment Drive */
#define Vref_LCD_COM_SEG            (* (reg8 *) Vref__LCD_COM_SEG)
/* Enable Segment LCD */
#define Vref_LCD_EN                 (* (reg8 *) Vref__LCD_EN)
/* Slew Rate Control */
#define Vref_SLW                    (* (reg8 *) Vref__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define Vref_PRTDSI__CAPS_SEL       (* (reg8 *) Vref__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define Vref_PRTDSI__DBL_SYNC_IN    (* (reg8 *) Vref__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define Vref_PRTDSI__OE_SEL0        (* (reg8 *) Vref__PRTDSI__OE_SEL0) 
#define Vref_PRTDSI__OE_SEL1        (* (reg8 *) Vref__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define Vref_PRTDSI__OUT_SEL0       (* (reg8 *) Vref__PRTDSI__OUT_SEL0) 
#define Vref_PRTDSI__OUT_SEL1       (* (reg8 *) Vref__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define Vref_PRTDSI__SYNC_OUT       (* (reg8 *) Vref__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(Vref__SIO_CFG)
    #define Vref_SIO_HYST_EN        (* (reg8 *) Vref__SIO_HYST_EN)
    #define Vref_SIO_REG_HIFREQ     (* (reg8 *) Vref__SIO_REG_HIFREQ)
    #define Vref_SIO_CFG            (* (reg8 *) Vref__SIO_CFG)
    #define Vref_SIO_DIFF           (* (reg8 *) Vref__SIO_DIFF)
#endif /* (Vref__SIO_CFG) */

/* Interrupt Registers */
#if defined(Vref__INTSTAT)
    #define Vref_INTSTAT            (* (reg8 *) Vref__INTSTAT)
    #define Vref_SNAP               (* (reg8 *) Vref__SNAP)
    
	#define Vref_0_INTTYPE_REG 		(* (reg8 *) Vref__0__INTTYPE)
#endif /* (Vref__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_Vref_H */


/* [] END OF FILE */
