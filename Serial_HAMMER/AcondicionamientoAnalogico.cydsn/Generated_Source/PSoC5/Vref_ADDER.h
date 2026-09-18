/*******************************************************************************
* File Name: Vref_ADDER.h  
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

#if !defined(CY_PINS_Vref_ADDER_H) /* Pins Vref_ADDER_H */
#define CY_PINS_Vref_ADDER_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "Vref_ADDER_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 Vref_ADDER__PORT == 15 && ((Vref_ADDER__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    Vref_ADDER_Write(uint8 value);
void    Vref_ADDER_SetDriveMode(uint8 mode);
uint8   Vref_ADDER_ReadDataReg(void);
uint8   Vref_ADDER_Read(void);
void    Vref_ADDER_SetInterruptMode(uint16 position, uint16 mode);
uint8   Vref_ADDER_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the Vref_ADDER_SetDriveMode() function.
     *  @{
     */
        #define Vref_ADDER_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define Vref_ADDER_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define Vref_ADDER_DM_RES_UP          PIN_DM_RES_UP
        #define Vref_ADDER_DM_RES_DWN         PIN_DM_RES_DWN
        #define Vref_ADDER_DM_OD_LO           PIN_DM_OD_LO
        #define Vref_ADDER_DM_OD_HI           PIN_DM_OD_HI
        #define Vref_ADDER_DM_STRONG          PIN_DM_STRONG
        #define Vref_ADDER_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define Vref_ADDER_MASK               Vref_ADDER__MASK
#define Vref_ADDER_SHIFT              Vref_ADDER__SHIFT
#define Vref_ADDER_WIDTH              1u

/* Interrupt constants */
#if defined(Vref_ADDER__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in Vref_ADDER_SetInterruptMode() function.
     *  @{
     */
        #define Vref_ADDER_INTR_NONE      (uint16)(0x0000u)
        #define Vref_ADDER_INTR_RISING    (uint16)(0x0001u)
        #define Vref_ADDER_INTR_FALLING   (uint16)(0x0002u)
        #define Vref_ADDER_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define Vref_ADDER_INTR_MASK      (0x01u) 
#endif /* (Vref_ADDER__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define Vref_ADDER_PS                     (* (reg8 *) Vref_ADDER__PS)
/* Data Register */
#define Vref_ADDER_DR                     (* (reg8 *) Vref_ADDER__DR)
/* Port Number */
#define Vref_ADDER_PRT_NUM                (* (reg8 *) Vref_ADDER__PRT) 
/* Connect to Analog Globals */                                                  
#define Vref_ADDER_AG                     (* (reg8 *) Vref_ADDER__AG)                       
/* Analog MUX bux enable */
#define Vref_ADDER_AMUX                   (* (reg8 *) Vref_ADDER__AMUX) 
/* Bidirectional Enable */                                                        
#define Vref_ADDER_BIE                    (* (reg8 *) Vref_ADDER__BIE)
/* Bit-mask for Aliased Register Access */
#define Vref_ADDER_BIT_MASK               (* (reg8 *) Vref_ADDER__BIT_MASK)
/* Bypass Enable */
#define Vref_ADDER_BYP                    (* (reg8 *) Vref_ADDER__BYP)
/* Port wide control signals */                                                   
#define Vref_ADDER_CTL                    (* (reg8 *) Vref_ADDER__CTL)
/* Drive Modes */
#define Vref_ADDER_DM0                    (* (reg8 *) Vref_ADDER__DM0) 
#define Vref_ADDER_DM1                    (* (reg8 *) Vref_ADDER__DM1)
#define Vref_ADDER_DM2                    (* (reg8 *) Vref_ADDER__DM2) 
/* Input Buffer Disable Override */
#define Vref_ADDER_INP_DIS                (* (reg8 *) Vref_ADDER__INP_DIS)
/* LCD Common or Segment Drive */
#define Vref_ADDER_LCD_COM_SEG            (* (reg8 *) Vref_ADDER__LCD_COM_SEG)
/* Enable Segment LCD */
#define Vref_ADDER_LCD_EN                 (* (reg8 *) Vref_ADDER__LCD_EN)
/* Slew Rate Control */
#define Vref_ADDER_SLW                    (* (reg8 *) Vref_ADDER__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define Vref_ADDER_PRTDSI__CAPS_SEL       (* (reg8 *) Vref_ADDER__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define Vref_ADDER_PRTDSI__DBL_SYNC_IN    (* (reg8 *) Vref_ADDER__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define Vref_ADDER_PRTDSI__OE_SEL0        (* (reg8 *) Vref_ADDER__PRTDSI__OE_SEL0) 
#define Vref_ADDER_PRTDSI__OE_SEL1        (* (reg8 *) Vref_ADDER__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define Vref_ADDER_PRTDSI__OUT_SEL0       (* (reg8 *) Vref_ADDER__PRTDSI__OUT_SEL0) 
#define Vref_ADDER_PRTDSI__OUT_SEL1       (* (reg8 *) Vref_ADDER__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define Vref_ADDER_PRTDSI__SYNC_OUT       (* (reg8 *) Vref_ADDER__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(Vref_ADDER__SIO_CFG)
    #define Vref_ADDER_SIO_HYST_EN        (* (reg8 *) Vref_ADDER__SIO_HYST_EN)
    #define Vref_ADDER_SIO_REG_HIFREQ     (* (reg8 *) Vref_ADDER__SIO_REG_HIFREQ)
    #define Vref_ADDER_SIO_CFG            (* (reg8 *) Vref_ADDER__SIO_CFG)
    #define Vref_ADDER_SIO_DIFF           (* (reg8 *) Vref_ADDER__SIO_DIFF)
#endif /* (Vref_ADDER__SIO_CFG) */

/* Interrupt Registers */
#if defined(Vref_ADDER__INTSTAT)
    #define Vref_ADDER_INTSTAT            (* (reg8 *) Vref_ADDER__INTSTAT)
    #define Vref_ADDER_SNAP               (* (reg8 *) Vref_ADDER__SNAP)
    
	#define Vref_ADDER_0_INTTYPE_REG 		(* (reg8 *) Vref_ADDER__0__INTTYPE)
#endif /* (Vref_ADDER__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_Vref_ADDER_H */


/* [] END OF FILE */
