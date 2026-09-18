/*******************************************************************************
* File Name: Vref_PGA.h  
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

#if !defined(CY_PINS_Vref_PGA_H) /* Pins Vref_PGA_H */
#define CY_PINS_Vref_PGA_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "Vref_PGA_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 Vref_PGA__PORT == 15 && ((Vref_PGA__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    Vref_PGA_Write(uint8 value);
void    Vref_PGA_SetDriveMode(uint8 mode);
uint8   Vref_PGA_ReadDataReg(void);
uint8   Vref_PGA_Read(void);
void    Vref_PGA_SetInterruptMode(uint16 position, uint16 mode);
uint8   Vref_PGA_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the Vref_PGA_SetDriveMode() function.
     *  @{
     */
        #define Vref_PGA_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define Vref_PGA_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define Vref_PGA_DM_RES_UP          PIN_DM_RES_UP
        #define Vref_PGA_DM_RES_DWN         PIN_DM_RES_DWN
        #define Vref_PGA_DM_OD_LO           PIN_DM_OD_LO
        #define Vref_PGA_DM_OD_HI           PIN_DM_OD_HI
        #define Vref_PGA_DM_STRONG          PIN_DM_STRONG
        #define Vref_PGA_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define Vref_PGA_MASK               Vref_PGA__MASK
#define Vref_PGA_SHIFT              Vref_PGA__SHIFT
#define Vref_PGA_WIDTH              1u

/* Interrupt constants */
#if defined(Vref_PGA__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in Vref_PGA_SetInterruptMode() function.
     *  @{
     */
        #define Vref_PGA_INTR_NONE      (uint16)(0x0000u)
        #define Vref_PGA_INTR_RISING    (uint16)(0x0001u)
        #define Vref_PGA_INTR_FALLING   (uint16)(0x0002u)
        #define Vref_PGA_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define Vref_PGA_INTR_MASK      (0x01u) 
#endif /* (Vref_PGA__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define Vref_PGA_PS                     (* (reg8 *) Vref_PGA__PS)
/* Data Register */
#define Vref_PGA_DR                     (* (reg8 *) Vref_PGA__DR)
/* Port Number */
#define Vref_PGA_PRT_NUM                (* (reg8 *) Vref_PGA__PRT) 
/* Connect to Analog Globals */                                                  
#define Vref_PGA_AG                     (* (reg8 *) Vref_PGA__AG)                       
/* Analog MUX bux enable */
#define Vref_PGA_AMUX                   (* (reg8 *) Vref_PGA__AMUX) 
/* Bidirectional Enable */                                                        
#define Vref_PGA_BIE                    (* (reg8 *) Vref_PGA__BIE)
/* Bit-mask for Aliased Register Access */
#define Vref_PGA_BIT_MASK               (* (reg8 *) Vref_PGA__BIT_MASK)
/* Bypass Enable */
#define Vref_PGA_BYP                    (* (reg8 *) Vref_PGA__BYP)
/* Port wide control signals */                                                   
#define Vref_PGA_CTL                    (* (reg8 *) Vref_PGA__CTL)
/* Drive Modes */
#define Vref_PGA_DM0                    (* (reg8 *) Vref_PGA__DM0) 
#define Vref_PGA_DM1                    (* (reg8 *) Vref_PGA__DM1)
#define Vref_PGA_DM2                    (* (reg8 *) Vref_PGA__DM2) 
/* Input Buffer Disable Override */
#define Vref_PGA_INP_DIS                (* (reg8 *) Vref_PGA__INP_DIS)
/* LCD Common or Segment Drive */
#define Vref_PGA_LCD_COM_SEG            (* (reg8 *) Vref_PGA__LCD_COM_SEG)
/* Enable Segment LCD */
#define Vref_PGA_LCD_EN                 (* (reg8 *) Vref_PGA__LCD_EN)
/* Slew Rate Control */
#define Vref_PGA_SLW                    (* (reg8 *) Vref_PGA__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define Vref_PGA_PRTDSI__CAPS_SEL       (* (reg8 *) Vref_PGA__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define Vref_PGA_PRTDSI__DBL_SYNC_IN    (* (reg8 *) Vref_PGA__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define Vref_PGA_PRTDSI__OE_SEL0        (* (reg8 *) Vref_PGA__PRTDSI__OE_SEL0) 
#define Vref_PGA_PRTDSI__OE_SEL1        (* (reg8 *) Vref_PGA__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define Vref_PGA_PRTDSI__OUT_SEL0       (* (reg8 *) Vref_PGA__PRTDSI__OUT_SEL0) 
#define Vref_PGA_PRTDSI__OUT_SEL1       (* (reg8 *) Vref_PGA__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define Vref_PGA_PRTDSI__SYNC_OUT       (* (reg8 *) Vref_PGA__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(Vref_PGA__SIO_CFG)
    #define Vref_PGA_SIO_HYST_EN        (* (reg8 *) Vref_PGA__SIO_HYST_EN)
    #define Vref_PGA_SIO_REG_HIFREQ     (* (reg8 *) Vref_PGA__SIO_REG_HIFREQ)
    #define Vref_PGA_SIO_CFG            (* (reg8 *) Vref_PGA__SIO_CFG)
    #define Vref_PGA_SIO_DIFF           (* (reg8 *) Vref_PGA__SIO_DIFF)
#endif /* (Vref_PGA__SIO_CFG) */

/* Interrupt Registers */
#if defined(Vref_PGA__INTSTAT)
    #define Vref_PGA_INTSTAT            (* (reg8 *) Vref_PGA__INTSTAT)
    #define Vref_PGA_SNAP               (* (reg8 *) Vref_PGA__SNAP)
    
	#define Vref_PGA_0_INTTYPE_REG 		(* (reg8 *) Vref_PGA__0__INTTYPE)
#endif /* (Vref_PGA__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_Vref_PGA_H */


/* [] END OF FILE */
