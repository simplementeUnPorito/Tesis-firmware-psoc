/*******************************************************************************
* File Name: Geophone_p.h  
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

#if !defined(CY_PINS_Geophone_p_H) /* Pins Geophone_p_H */
#define CY_PINS_Geophone_p_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "Geophone_p_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 Geophone_p__PORT == 15 && ((Geophone_p__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    Geophone_p_Write(uint8 value);
void    Geophone_p_SetDriveMode(uint8 mode);
uint8   Geophone_p_ReadDataReg(void);
uint8   Geophone_p_Read(void);
void    Geophone_p_SetInterruptMode(uint16 position, uint16 mode);
uint8   Geophone_p_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the Geophone_p_SetDriveMode() function.
     *  @{
     */
        #define Geophone_p_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define Geophone_p_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define Geophone_p_DM_RES_UP          PIN_DM_RES_UP
        #define Geophone_p_DM_RES_DWN         PIN_DM_RES_DWN
        #define Geophone_p_DM_OD_LO           PIN_DM_OD_LO
        #define Geophone_p_DM_OD_HI           PIN_DM_OD_HI
        #define Geophone_p_DM_STRONG          PIN_DM_STRONG
        #define Geophone_p_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define Geophone_p_MASK               Geophone_p__MASK
#define Geophone_p_SHIFT              Geophone_p__SHIFT
#define Geophone_p_WIDTH              1u

/* Interrupt constants */
#if defined(Geophone_p__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in Geophone_p_SetInterruptMode() function.
     *  @{
     */
        #define Geophone_p_INTR_NONE      (uint16)(0x0000u)
        #define Geophone_p_INTR_RISING    (uint16)(0x0001u)
        #define Geophone_p_INTR_FALLING   (uint16)(0x0002u)
        #define Geophone_p_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define Geophone_p_INTR_MASK      (0x01u) 
#endif /* (Geophone_p__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define Geophone_p_PS                     (* (reg8 *) Geophone_p__PS)
/* Data Register */
#define Geophone_p_DR                     (* (reg8 *) Geophone_p__DR)
/* Port Number */
#define Geophone_p_PRT_NUM                (* (reg8 *) Geophone_p__PRT) 
/* Connect to Analog Globals */                                                  
#define Geophone_p_AG                     (* (reg8 *) Geophone_p__AG)                       
/* Analog MUX bux enable */
#define Geophone_p_AMUX                   (* (reg8 *) Geophone_p__AMUX) 
/* Bidirectional Enable */                                                        
#define Geophone_p_BIE                    (* (reg8 *) Geophone_p__BIE)
/* Bit-mask for Aliased Register Access */
#define Geophone_p_BIT_MASK               (* (reg8 *) Geophone_p__BIT_MASK)
/* Bypass Enable */
#define Geophone_p_BYP                    (* (reg8 *) Geophone_p__BYP)
/* Port wide control signals */                                                   
#define Geophone_p_CTL                    (* (reg8 *) Geophone_p__CTL)
/* Drive Modes */
#define Geophone_p_DM0                    (* (reg8 *) Geophone_p__DM0) 
#define Geophone_p_DM1                    (* (reg8 *) Geophone_p__DM1)
#define Geophone_p_DM2                    (* (reg8 *) Geophone_p__DM2) 
/* Input Buffer Disable Override */
#define Geophone_p_INP_DIS                (* (reg8 *) Geophone_p__INP_DIS)
/* LCD Common or Segment Drive */
#define Geophone_p_LCD_COM_SEG            (* (reg8 *) Geophone_p__LCD_COM_SEG)
/* Enable Segment LCD */
#define Geophone_p_LCD_EN                 (* (reg8 *) Geophone_p__LCD_EN)
/* Slew Rate Control */
#define Geophone_p_SLW                    (* (reg8 *) Geophone_p__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define Geophone_p_PRTDSI__CAPS_SEL       (* (reg8 *) Geophone_p__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define Geophone_p_PRTDSI__DBL_SYNC_IN    (* (reg8 *) Geophone_p__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define Geophone_p_PRTDSI__OE_SEL0        (* (reg8 *) Geophone_p__PRTDSI__OE_SEL0) 
#define Geophone_p_PRTDSI__OE_SEL1        (* (reg8 *) Geophone_p__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define Geophone_p_PRTDSI__OUT_SEL0       (* (reg8 *) Geophone_p__PRTDSI__OUT_SEL0) 
#define Geophone_p_PRTDSI__OUT_SEL1       (* (reg8 *) Geophone_p__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define Geophone_p_PRTDSI__SYNC_OUT       (* (reg8 *) Geophone_p__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(Geophone_p__SIO_CFG)
    #define Geophone_p_SIO_HYST_EN        (* (reg8 *) Geophone_p__SIO_HYST_EN)
    #define Geophone_p_SIO_REG_HIFREQ     (* (reg8 *) Geophone_p__SIO_REG_HIFREQ)
    #define Geophone_p_SIO_CFG            (* (reg8 *) Geophone_p__SIO_CFG)
    #define Geophone_p_SIO_DIFF           (* (reg8 *) Geophone_p__SIO_DIFF)
#endif /* (Geophone_p__SIO_CFG) */

/* Interrupt Registers */
#if defined(Geophone_p__INTSTAT)
    #define Geophone_p_INTSTAT            (* (reg8 *) Geophone_p__INTSTAT)
    #define Geophone_p_SNAP               (* (reg8 *) Geophone_p__SNAP)
    
	#define Geophone_p_0_INTTYPE_REG 		(* (reg8 *) Geophone_p__0__INTTYPE)
#endif /* (Geophone_p__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_Geophone_p_H */


/* [] END OF FILE */
