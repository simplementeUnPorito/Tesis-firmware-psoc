/*******************************************************************************
* File Name: Vshield.h  
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

#if !defined(CY_PINS_Vshield_H) /* Pins Vshield_H */
#define CY_PINS_Vshield_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "Vshield_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 Vshield__PORT == 15 && ((Vshield__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    Vshield_Write(uint8 value);
void    Vshield_SetDriveMode(uint8 mode);
uint8   Vshield_ReadDataReg(void);
uint8   Vshield_Read(void);
void    Vshield_SetInterruptMode(uint16 position, uint16 mode);
uint8   Vshield_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the Vshield_SetDriveMode() function.
     *  @{
     */
        #define Vshield_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define Vshield_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define Vshield_DM_RES_UP          PIN_DM_RES_UP
        #define Vshield_DM_RES_DWN         PIN_DM_RES_DWN
        #define Vshield_DM_OD_LO           PIN_DM_OD_LO
        #define Vshield_DM_OD_HI           PIN_DM_OD_HI
        #define Vshield_DM_STRONG          PIN_DM_STRONG
        #define Vshield_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define Vshield_MASK               Vshield__MASK
#define Vshield_SHIFT              Vshield__SHIFT
#define Vshield_WIDTH              1u

/* Interrupt constants */
#if defined(Vshield__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in Vshield_SetInterruptMode() function.
     *  @{
     */
        #define Vshield_INTR_NONE      (uint16)(0x0000u)
        #define Vshield_INTR_RISING    (uint16)(0x0001u)
        #define Vshield_INTR_FALLING   (uint16)(0x0002u)
        #define Vshield_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define Vshield_INTR_MASK      (0x01u) 
#endif /* (Vshield__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define Vshield_PS                     (* (reg8 *) Vshield__PS)
/* Data Register */
#define Vshield_DR                     (* (reg8 *) Vshield__DR)
/* Port Number */
#define Vshield_PRT_NUM                (* (reg8 *) Vshield__PRT) 
/* Connect to Analog Globals */                                                  
#define Vshield_AG                     (* (reg8 *) Vshield__AG)                       
/* Analog MUX bux enable */
#define Vshield_AMUX                   (* (reg8 *) Vshield__AMUX) 
/* Bidirectional Enable */                                                        
#define Vshield_BIE                    (* (reg8 *) Vshield__BIE)
/* Bit-mask for Aliased Register Access */
#define Vshield_BIT_MASK               (* (reg8 *) Vshield__BIT_MASK)
/* Bypass Enable */
#define Vshield_BYP                    (* (reg8 *) Vshield__BYP)
/* Port wide control signals */                                                   
#define Vshield_CTL                    (* (reg8 *) Vshield__CTL)
/* Drive Modes */
#define Vshield_DM0                    (* (reg8 *) Vshield__DM0) 
#define Vshield_DM1                    (* (reg8 *) Vshield__DM1)
#define Vshield_DM2                    (* (reg8 *) Vshield__DM2) 
/* Input Buffer Disable Override */
#define Vshield_INP_DIS                (* (reg8 *) Vshield__INP_DIS)
/* LCD Common or Segment Drive */
#define Vshield_LCD_COM_SEG            (* (reg8 *) Vshield__LCD_COM_SEG)
/* Enable Segment LCD */
#define Vshield_LCD_EN                 (* (reg8 *) Vshield__LCD_EN)
/* Slew Rate Control */
#define Vshield_SLW                    (* (reg8 *) Vshield__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define Vshield_PRTDSI__CAPS_SEL       (* (reg8 *) Vshield__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define Vshield_PRTDSI__DBL_SYNC_IN    (* (reg8 *) Vshield__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define Vshield_PRTDSI__OE_SEL0        (* (reg8 *) Vshield__PRTDSI__OE_SEL0) 
#define Vshield_PRTDSI__OE_SEL1        (* (reg8 *) Vshield__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define Vshield_PRTDSI__OUT_SEL0       (* (reg8 *) Vshield__PRTDSI__OUT_SEL0) 
#define Vshield_PRTDSI__OUT_SEL1       (* (reg8 *) Vshield__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define Vshield_PRTDSI__SYNC_OUT       (* (reg8 *) Vshield__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(Vshield__SIO_CFG)
    #define Vshield_SIO_HYST_EN        (* (reg8 *) Vshield__SIO_HYST_EN)
    #define Vshield_SIO_REG_HIFREQ     (* (reg8 *) Vshield__SIO_REG_HIFREQ)
    #define Vshield_SIO_CFG            (* (reg8 *) Vshield__SIO_CFG)
    #define Vshield_SIO_DIFF           (* (reg8 *) Vshield__SIO_DIFF)
#endif /* (Vshield__SIO_CFG) */

/* Interrupt Registers */
#if defined(Vshield__INTSTAT)
    #define Vshield_INTSTAT            (* (reg8 *) Vshield__INTSTAT)
    #define Vshield_SNAP               (* (reg8 *) Vshield__SNAP)
    
	#define Vshield_0_INTTYPE_REG 		(* (reg8 *) Vshield__0__INTTYPE)
#endif /* (Vshield__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_Vshield_H */


/* [] END OF FILE */
