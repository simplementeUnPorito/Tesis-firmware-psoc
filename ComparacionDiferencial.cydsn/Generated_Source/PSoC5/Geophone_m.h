/*******************************************************************************
* File Name: Geophone_m.h  
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

#if !defined(CY_PINS_Geophone_m_H) /* Pins Geophone_m_H */
#define CY_PINS_Geophone_m_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "Geophone_m_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 Geophone_m__PORT == 15 && ((Geophone_m__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    Geophone_m_Write(uint8 value);
void    Geophone_m_SetDriveMode(uint8 mode);
uint8   Geophone_m_ReadDataReg(void);
uint8   Geophone_m_Read(void);
void    Geophone_m_SetInterruptMode(uint16 position, uint16 mode);
uint8   Geophone_m_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the Geophone_m_SetDriveMode() function.
     *  @{
     */
        #define Geophone_m_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define Geophone_m_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define Geophone_m_DM_RES_UP          PIN_DM_RES_UP
        #define Geophone_m_DM_RES_DWN         PIN_DM_RES_DWN
        #define Geophone_m_DM_OD_LO           PIN_DM_OD_LO
        #define Geophone_m_DM_OD_HI           PIN_DM_OD_HI
        #define Geophone_m_DM_STRONG          PIN_DM_STRONG
        #define Geophone_m_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define Geophone_m_MASK               Geophone_m__MASK
#define Geophone_m_SHIFT              Geophone_m__SHIFT
#define Geophone_m_WIDTH              1u

/* Interrupt constants */
#if defined(Geophone_m__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in Geophone_m_SetInterruptMode() function.
     *  @{
     */
        #define Geophone_m_INTR_NONE      (uint16)(0x0000u)
        #define Geophone_m_INTR_RISING    (uint16)(0x0001u)
        #define Geophone_m_INTR_FALLING   (uint16)(0x0002u)
        #define Geophone_m_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define Geophone_m_INTR_MASK      (0x01u) 
#endif /* (Geophone_m__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define Geophone_m_PS                     (* (reg8 *) Geophone_m__PS)
/* Data Register */
#define Geophone_m_DR                     (* (reg8 *) Geophone_m__DR)
/* Port Number */
#define Geophone_m_PRT_NUM                (* (reg8 *) Geophone_m__PRT) 
/* Connect to Analog Globals */                                                  
#define Geophone_m_AG                     (* (reg8 *) Geophone_m__AG)                       
/* Analog MUX bux enable */
#define Geophone_m_AMUX                   (* (reg8 *) Geophone_m__AMUX) 
/* Bidirectional Enable */                                                        
#define Geophone_m_BIE                    (* (reg8 *) Geophone_m__BIE)
/* Bit-mask for Aliased Register Access */
#define Geophone_m_BIT_MASK               (* (reg8 *) Geophone_m__BIT_MASK)
/* Bypass Enable */
#define Geophone_m_BYP                    (* (reg8 *) Geophone_m__BYP)
/* Port wide control signals */                                                   
#define Geophone_m_CTL                    (* (reg8 *) Geophone_m__CTL)
/* Drive Modes */
#define Geophone_m_DM0                    (* (reg8 *) Geophone_m__DM0) 
#define Geophone_m_DM1                    (* (reg8 *) Geophone_m__DM1)
#define Geophone_m_DM2                    (* (reg8 *) Geophone_m__DM2) 
/* Input Buffer Disable Override */
#define Geophone_m_INP_DIS                (* (reg8 *) Geophone_m__INP_DIS)
/* LCD Common or Segment Drive */
#define Geophone_m_LCD_COM_SEG            (* (reg8 *) Geophone_m__LCD_COM_SEG)
/* Enable Segment LCD */
#define Geophone_m_LCD_EN                 (* (reg8 *) Geophone_m__LCD_EN)
/* Slew Rate Control */
#define Geophone_m_SLW                    (* (reg8 *) Geophone_m__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define Geophone_m_PRTDSI__CAPS_SEL       (* (reg8 *) Geophone_m__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define Geophone_m_PRTDSI__DBL_SYNC_IN    (* (reg8 *) Geophone_m__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define Geophone_m_PRTDSI__OE_SEL0        (* (reg8 *) Geophone_m__PRTDSI__OE_SEL0) 
#define Geophone_m_PRTDSI__OE_SEL1        (* (reg8 *) Geophone_m__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define Geophone_m_PRTDSI__OUT_SEL0       (* (reg8 *) Geophone_m__PRTDSI__OUT_SEL0) 
#define Geophone_m_PRTDSI__OUT_SEL1       (* (reg8 *) Geophone_m__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define Geophone_m_PRTDSI__SYNC_OUT       (* (reg8 *) Geophone_m__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(Geophone_m__SIO_CFG)
    #define Geophone_m_SIO_HYST_EN        (* (reg8 *) Geophone_m__SIO_HYST_EN)
    #define Geophone_m_SIO_REG_HIFREQ     (* (reg8 *) Geophone_m__SIO_REG_HIFREQ)
    #define Geophone_m_SIO_CFG            (* (reg8 *) Geophone_m__SIO_CFG)
    #define Geophone_m_SIO_DIFF           (* (reg8 *) Geophone_m__SIO_DIFF)
#endif /* (Geophone_m__SIO_CFG) */

/* Interrupt Registers */
#if defined(Geophone_m__INTSTAT)
    #define Geophone_m_INTSTAT            (* (reg8 *) Geophone_m__INTSTAT)
    #define Geophone_m_SNAP               (* (reg8 *) Geophone_m__SNAP)
    
	#define Geophone_m_0_INTTYPE_REG 		(* (reg8 *) Geophone_m__0__INTTYPE)
#endif /* (Geophone_m__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_Geophone_m_H */


/* [] END OF FILE */
