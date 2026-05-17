/*******************************************************************************
* File Name: Vcomun.h  
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

#if !defined(CY_PINS_Vcomun_H) /* Pins Vcomun_H */
#define CY_PINS_Vcomun_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "Vcomun_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 Vcomun__PORT == 15 && ((Vcomun__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    Vcomun_Write(uint8 value);
void    Vcomun_SetDriveMode(uint8 mode);
uint8   Vcomun_ReadDataReg(void);
uint8   Vcomun_Read(void);
void    Vcomun_SetInterruptMode(uint16 position, uint16 mode);
uint8   Vcomun_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the Vcomun_SetDriveMode() function.
     *  @{
     */
        #define Vcomun_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define Vcomun_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define Vcomun_DM_RES_UP          PIN_DM_RES_UP
        #define Vcomun_DM_RES_DWN         PIN_DM_RES_DWN
        #define Vcomun_DM_OD_LO           PIN_DM_OD_LO
        #define Vcomun_DM_OD_HI           PIN_DM_OD_HI
        #define Vcomun_DM_STRONG          PIN_DM_STRONG
        #define Vcomun_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define Vcomun_MASK               Vcomun__MASK
#define Vcomun_SHIFT              Vcomun__SHIFT
#define Vcomun_WIDTH              1u

/* Interrupt constants */
#if defined(Vcomun__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in Vcomun_SetInterruptMode() function.
     *  @{
     */
        #define Vcomun_INTR_NONE      (uint16)(0x0000u)
        #define Vcomun_INTR_RISING    (uint16)(0x0001u)
        #define Vcomun_INTR_FALLING   (uint16)(0x0002u)
        #define Vcomun_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define Vcomun_INTR_MASK      (0x01u) 
#endif /* (Vcomun__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define Vcomun_PS                     (* (reg8 *) Vcomun__PS)
/* Data Register */
#define Vcomun_DR                     (* (reg8 *) Vcomun__DR)
/* Port Number */
#define Vcomun_PRT_NUM                (* (reg8 *) Vcomun__PRT) 
/* Connect to Analog Globals */                                                  
#define Vcomun_AG                     (* (reg8 *) Vcomun__AG)                       
/* Analog MUX bux enable */
#define Vcomun_AMUX                   (* (reg8 *) Vcomun__AMUX) 
/* Bidirectional Enable */                                                        
#define Vcomun_BIE                    (* (reg8 *) Vcomun__BIE)
/* Bit-mask for Aliased Register Access */
#define Vcomun_BIT_MASK               (* (reg8 *) Vcomun__BIT_MASK)
/* Bypass Enable */
#define Vcomun_BYP                    (* (reg8 *) Vcomun__BYP)
/* Port wide control signals */                                                   
#define Vcomun_CTL                    (* (reg8 *) Vcomun__CTL)
/* Drive Modes */
#define Vcomun_DM0                    (* (reg8 *) Vcomun__DM0) 
#define Vcomun_DM1                    (* (reg8 *) Vcomun__DM1)
#define Vcomun_DM2                    (* (reg8 *) Vcomun__DM2) 
/* Input Buffer Disable Override */
#define Vcomun_INP_DIS                (* (reg8 *) Vcomun__INP_DIS)
/* LCD Common or Segment Drive */
#define Vcomun_LCD_COM_SEG            (* (reg8 *) Vcomun__LCD_COM_SEG)
/* Enable Segment LCD */
#define Vcomun_LCD_EN                 (* (reg8 *) Vcomun__LCD_EN)
/* Slew Rate Control */
#define Vcomun_SLW                    (* (reg8 *) Vcomun__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define Vcomun_PRTDSI__CAPS_SEL       (* (reg8 *) Vcomun__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define Vcomun_PRTDSI__DBL_SYNC_IN    (* (reg8 *) Vcomun__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define Vcomun_PRTDSI__OE_SEL0        (* (reg8 *) Vcomun__PRTDSI__OE_SEL0) 
#define Vcomun_PRTDSI__OE_SEL1        (* (reg8 *) Vcomun__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define Vcomun_PRTDSI__OUT_SEL0       (* (reg8 *) Vcomun__PRTDSI__OUT_SEL0) 
#define Vcomun_PRTDSI__OUT_SEL1       (* (reg8 *) Vcomun__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define Vcomun_PRTDSI__SYNC_OUT       (* (reg8 *) Vcomun__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(Vcomun__SIO_CFG)
    #define Vcomun_SIO_HYST_EN        (* (reg8 *) Vcomun__SIO_HYST_EN)
    #define Vcomun_SIO_REG_HIFREQ     (* (reg8 *) Vcomun__SIO_REG_HIFREQ)
    #define Vcomun_SIO_CFG            (* (reg8 *) Vcomun__SIO_CFG)
    #define Vcomun_SIO_DIFF           (* (reg8 *) Vcomun__SIO_DIFF)
#endif /* (Vcomun__SIO_CFG) */

/* Interrupt Registers */
#if defined(Vcomun__INTSTAT)
    #define Vcomun_INTSTAT            (* (reg8 *) Vcomun__INTSTAT)
    #define Vcomun_SNAP               (* (reg8 *) Vcomun__SNAP)
    
	#define Vcomun_0_INTTYPE_REG 		(* (reg8 *) Vcomun__0__INTTYPE)
#endif /* (Vcomun__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_Vcomun_H */


/* [] END OF FILE */
