/*******************************************************************************
* File Name: Vref_LP.h  
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

#if !defined(CY_PINS_Vref_LP_H) /* Pins Vref_LP_H */
#define CY_PINS_Vref_LP_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "Vref_LP_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 Vref_LP__PORT == 15 && ((Vref_LP__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    Vref_LP_Write(uint8 value);
void    Vref_LP_SetDriveMode(uint8 mode);
uint8   Vref_LP_ReadDataReg(void);
uint8   Vref_LP_Read(void);
void    Vref_LP_SetInterruptMode(uint16 position, uint16 mode);
uint8   Vref_LP_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the Vref_LP_SetDriveMode() function.
     *  @{
     */
        #define Vref_LP_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define Vref_LP_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define Vref_LP_DM_RES_UP          PIN_DM_RES_UP
        #define Vref_LP_DM_RES_DWN         PIN_DM_RES_DWN
        #define Vref_LP_DM_OD_LO           PIN_DM_OD_LO
        #define Vref_LP_DM_OD_HI           PIN_DM_OD_HI
        #define Vref_LP_DM_STRONG          PIN_DM_STRONG
        #define Vref_LP_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define Vref_LP_MASK               Vref_LP__MASK
#define Vref_LP_SHIFT              Vref_LP__SHIFT
#define Vref_LP_WIDTH              1u

/* Interrupt constants */
#if defined(Vref_LP__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in Vref_LP_SetInterruptMode() function.
     *  @{
     */
        #define Vref_LP_INTR_NONE      (uint16)(0x0000u)
        #define Vref_LP_INTR_RISING    (uint16)(0x0001u)
        #define Vref_LP_INTR_FALLING   (uint16)(0x0002u)
        #define Vref_LP_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define Vref_LP_INTR_MASK      (0x01u) 
#endif /* (Vref_LP__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define Vref_LP_PS                     (* (reg8 *) Vref_LP__PS)
/* Data Register */
#define Vref_LP_DR                     (* (reg8 *) Vref_LP__DR)
/* Port Number */
#define Vref_LP_PRT_NUM                (* (reg8 *) Vref_LP__PRT) 
/* Connect to Analog Globals */                                                  
#define Vref_LP_AG                     (* (reg8 *) Vref_LP__AG)                       
/* Analog MUX bux enable */
#define Vref_LP_AMUX                   (* (reg8 *) Vref_LP__AMUX) 
/* Bidirectional Enable */                                                        
#define Vref_LP_BIE                    (* (reg8 *) Vref_LP__BIE)
/* Bit-mask for Aliased Register Access */
#define Vref_LP_BIT_MASK               (* (reg8 *) Vref_LP__BIT_MASK)
/* Bypass Enable */
#define Vref_LP_BYP                    (* (reg8 *) Vref_LP__BYP)
/* Port wide control signals */                                                   
#define Vref_LP_CTL                    (* (reg8 *) Vref_LP__CTL)
/* Drive Modes */
#define Vref_LP_DM0                    (* (reg8 *) Vref_LP__DM0) 
#define Vref_LP_DM1                    (* (reg8 *) Vref_LP__DM1)
#define Vref_LP_DM2                    (* (reg8 *) Vref_LP__DM2) 
/* Input Buffer Disable Override */
#define Vref_LP_INP_DIS                (* (reg8 *) Vref_LP__INP_DIS)
/* LCD Common or Segment Drive */
#define Vref_LP_LCD_COM_SEG            (* (reg8 *) Vref_LP__LCD_COM_SEG)
/* Enable Segment LCD */
#define Vref_LP_LCD_EN                 (* (reg8 *) Vref_LP__LCD_EN)
/* Slew Rate Control */
#define Vref_LP_SLW                    (* (reg8 *) Vref_LP__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define Vref_LP_PRTDSI__CAPS_SEL       (* (reg8 *) Vref_LP__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define Vref_LP_PRTDSI__DBL_SYNC_IN    (* (reg8 *) Vref_LP__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define Vref_LP_PRTDSI__OE_SEL0        (* (reg8 *) Vref_LP__PRTDSI__OE_SEL0) 
#define Vref_LP_PRTDSI__OE_SEL1        (* (reg8 *) Vref_LP__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define Vref_LP_PRTDSI__OUT_SEL0       (* (reg8 *) Vref_LP__PRTDSI__OUT_SEL0) 
#define Vref_LP_PRTDSI__OUT_SEL1       (* (reg8 *) Vref_LP__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define Vref_LP_PRTDSI__SYNC_OUT       (* (reg8 *) Vref_LP__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(Vref_LP__SIO_CFG)
    #define Vref_LP_SIO_HYST_EN        (* (reg8 *) Vref_LP__SIO_HYST_EN)
    #define Vref_LP_SIO_REG_HIFREQ     (* (reg8 *) Vref_LP__SIO_REG_HIFREQ)
    #define Vref_LP_SIO_CFG            (* (reg8 *) Vref_LP__SIO_CFG)
    #define Vref_LP_SIO_DIFF           (* (reg8 *) Vref_LP__SIO_DIFF)
#endif /* (Vref_LP__SIO_CFG) */

/* Interrupt Registers */
#if defined(Vref_LP__INTSTAT)
    #define Vref_LP_INTSTAT            (* (reg8 *) Vref_LP__INTSTAT)
    #define Vref_LP_SNAP               (* (reg8 *) Vref_LP__SNAP)
    
	#define Vref_LP_0_INTTYPE_REG 		(* (reg8 *) Vref_LP__0__INTTYPE)
#endif /* (Vref_LP__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_Vref_LP_H */


/* [] END OF FILE */
