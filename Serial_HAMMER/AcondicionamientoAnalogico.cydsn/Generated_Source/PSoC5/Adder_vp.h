/*******************************************************************************
* File Name: Adder_vp.h  
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

#if !defined(CY_PINS_Adder_vp_H) /* Pins Adder_vp_H */
#define CY_PINS_Adder_vp_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "Adder_vp_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 Adder_vp__PORT == 15 && ((Adder_vp__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    Adder_vp_Write(uint8 value);
void    Adder_vp_SetDriveMode(uint8 mode);
uint8   Adder_vp_ReadDataReg(void);
uint8   Adder_vp_Read(void);
void    Adder_vp_SetInterruptMode(uint16 position, uint16 mode);
uint8   Adder_vp_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the Adder_vp_SetDriveMode() function.
     *  @{
     */
        #define Adder_vp_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define Adder_vp_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define Adder_vp_DM_RES_UP          PIN_DM_RES_UP
        #define Adder_vp_DM_RES_DWN         PIN_DM_RES_DWN
        #define Adder_vp_DM_OD_LO           PIN_DM_OD_LO
        #define Adder_vp_DM_OD_HI           PIN_DM_OD_HI
        #define Adder_vp_DM_STRONG          PIN_DM_STRONG
        #define Adder_vp_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define Adder_vp_MASK               Adder_vp__MASK
#define Adder_vp_SHIFT              Adder_vp__SHIFT
#define Adder_vp_WIDTH              1u

/* Interrupt constants */
#if defined(Adder_vp__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in Adder_vp_SetInterruptMode() function.
     *  @{
     */
        #define Adder_vp_INTR_NONE      (uint16)(0x0000u)
        #define Adder_vp_INTR_RISING    (uint16)(0x0001u)
        #define Adder_vp_INTR_FALLING   (uint16)(0x0002u)
        #define Adder_vp_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define Adder_vp_INTR_MASK      (0x01u) 
#endif /* (Adder_vp__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define Adder_vp_PS                     (* (reg8 *) Adder_vp__PS)
/* Data Register */
#define Adder_vp_DR                     (* (reg8 *) Adder_vp__DR)
/* Port Number */
#define Adder_vp_PRT_NUM                (* (reg8 *) Adder_vp__PRT) 
/* Connect to Analog Globals */                                                  
#define Adder_vp_AG                     (* (reg8 *) Adder_vp__AG)                       
/* Analog MUX bux enable */
#define Adder_vp_AMUX                   (* (reg8 *) Adder_vp__AMUX) 
/* Bidirectional Enable */                                                        
#define Adder_vp_BIE                    (* (reg8 *) Adder_vp__BIE)
/* Bit-mask for Aliased Register Access */
#define Adder_vp_BIT_MASK               (* (reg8 *) Adder_vp__BIT_MASK)
/* Bypass Enable */
#define Adder_vp_BYP                    (* (reg8 *) Adder_vp__BYP)
/* Port wide control signals */                                                   
#define Adder_vp_CTL                    (* (reg8 *) Adder_vp__CTL)
/* Drive Modes */
#define Adder_vp_DM0                    (* (reg8 *) Adder_vp__DM0) 
#define Adder_vp_DM1                    (* (reg8 *) Adder_vp__DM1)
#define Adder_vp_DM2                    (* (reg8 *) Adder_vp__DM2) 
/* Input Buffer Disable Override */
#define Adder_vp_INP_DIS                (* (reg8 *) Adder_vp__INP_DIS)
/* LCD Common or Segment Drive */
#define Adder_vp_LCD_COM_SEG            (* (reg8 *) Adder_vp__LCD_COM_SEG)
/* Enable Segment LCD */
#define Adder_vp_LCD_EN                 (* (reg8 *) Adder_vp__LCD_EN)
/* Slew Rate Control */
#define Adder_vp_SLW                    (* (reg8 *) Adder_vp__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define Adder_vp_PRTDSI__CAPS_SEL       (* (reg8 *) Adder_vp__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define Adder_vp_PRTDSI__DBL_SYNC_IN    (* (reg8 *) Adder_vp__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define Adder_vp_PRTDSI__OE_SEL0        (* (reg8 *) Adder_vp__PRTDSI__OE_SEL0) 
#define Adder_vp_PRTDSI__OE_SEL1        (* (reg8 *) Adder_vp__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define Adder_vp_PRTDSI__OUT_SEL0       (* (reg8 *) Adder_vp__PRTDSI__OUT_SEL0) 
#define Adder_vp_PRTDSI__OUT_SEL1       (* (reg8 *) Adder_vp__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define Adder_vp_PRTDSI__SYNC_OUT       (* (reg8 *) Adder_vp__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(Adder_vp__SIO_CFG)
    #define Adder_vp_SIO_HYST_EN        (* (reg8 *) Adder_vp__SIO_HYST_EN)
    #define Adder_vp_SIO_REG_HIFREQ     (* (reg8 *) Adder_vp__SIO_REG_HIFREQ)
    #define Adder_vp_SIO_CFG            (* (reg8 *) Adder_vp__SIO_CFG)
    #define Adder_vp_SIO_DIFF           (* (reg8 *) Adder_vp__SIO_DIFF)
#endif /* (Adder_vp__SIO_CFG) */

/* Interrupt Registers */
#if defined(Adder_vp__INTSTAT)
    #define Adder_vp_INTSTAT            (* (reg8 *) Adder_vp__INTSTAT)
    #define Adder_vp_SNAP               (* (reg8 *) Adder_vp__SNAP)
    
	#define Adder_vp_0_INTTYPE_REG 		(* (reg8 *) Adder_vp__0__INTTYPE)
#endif /* (Adder_vp__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_Adder_vp_H */


/* [] END OF FILE */
