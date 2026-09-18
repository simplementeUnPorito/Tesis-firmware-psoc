/*******************************************************************************
* File Name: LPp.h  
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

#if !defined(CY_PINS_LPp_H) /* Pins LPp_H */
#define CY_PINS_LPp_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "LPp_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 LPp__PORT == 15 && ((LPp__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    LPp_Write(uint8 value);
void    LPp_SetDriveMode(uint8 mode);
uint8   LPp_ReadDataReg(void);
uint8   LPp_Read(void);
void    LPp_SetInterruptMode(uint16 position, uint16 mode);
uint8   LPp_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the LPp_SetDriveMode() function.
     *  @{
     */
        #define LPp_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define LPp_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define LPp_DM_RES_UP          PIN_DM_RES_UP
        #define LPp_DM_RES_DWN         PIN_DM_RES_DWN
        #define LPp_DM_OD_LO           PIN_DM_OD_LO
        #define LPp_DM_OD_HI           PIN_DM_OD_HI
        #define LPp_DM_STRONG          PIN_DM_STRONG
        #define LPp_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define LPp_MASK               LPp__MASK
#define LPp_SHIFT              LPp__SHIFT
#define LPp_WIDTH              1u

/* Interrupt constants */
#if defined(LPp__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in LPp_SetInterruptMode() function.
     *  @{
     */
        #define LPp_INTR_NONE      (uint16)(0x0000u)
        #define LPp_INTR_RISING    (uint16)(0x0001u)
        #define LPp_INTR_FALLING   (uint16)(0x0002u)
        #define LPp_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define LPp_INTR_MASK      (0x01u) 
#endif /* (LPp__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define LPp_PS                     (* (reg8 *) LPp__PS)
/* Data Register */
#define LPp_DR                     (* (reg8 *) LPp__DR)
/* Port Number */
#define LPp_PRT_NUM                (* (reg8 *) LPp__PRT) 
/* Connect to Analog Globals */                                                  
#define LPp_AG                     (* (reg8 *) LPp__AG)                       
/* Analog MUX bux enable */
#define LPp_AMUX                   (* (reg8 *) LPp__AMUX) 
/* Bidirectional Enable */                                                        
#define LPp_BIE                    (* (reg8 *) LPp__BIE)
/* Bit-mask for Aliased Register Access */
#define LPp_BIT_MASK               (* (reg8 *) LPp__BIT_MASK)
/* Bypass Enable */
#define LPp_BYP                    (* (reg8 *) LPp__BYP)
/* Port wide control signals */                                                   
#define LPp_CTL                    (* (reg8 *) LPp__CTL)
/* Drive Modes */
#define LPp_DM0                    (* (reg8 *) LPp__DM0) 
#define LPp_DM1                    (* (reg8 *) LPp__DM1)
#define LPp_DM2                    (* (reg8 *) LPp__DM2) 
/* Input Buffer Disable Override */
#define LPp_INP_DIS                (* (reg8 *) LPp__INP_DIS)
/* LCD Common or Segment Drive */
#define LPp_LCD_COM_SEG            (* (reg8 *) LPp__LCD_COM_SEG)
/* Enable Segment LCD */
#define LPp_LCD_EN                 (* (reg8 *) LPp__LCD_EN)
/* Slew Rate Control */
#define LPp_SLW                    (* (reg8 *) LPp__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define LPp_PRTDSI__CAPS_SEL       (* (reg8 *) LPp__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define LPp_PRTDSI__DBL_SYNC_IN    (* (reg8 *) LPp__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define LPp_PRTDSI__OE_SEL0        (* (reg8 *) LPp__PRTDSI__OE_SEL0) 
#define LPp_PRTDSI__OE_SEL1        (* (reg8 *) LPp__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define LPp_PRTDSI__OUT_SEL0       (* (reg8 *) LPp__PRTDSI__OUT_SEL0) 
#define LPp_PRTDSI__OUT_SEL1       (* (reg8 *) LPp__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define LPp_PRTDSI__SYNC_OUT       (* (reg8 *) LPp__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(LPp__SIO_CFG)
    #define LPp_SIO_HYST_EN        (* (reg8 *) LPp__SIO_HYST_EN)
    #define LPp_SIO_REG_HIFREQ     (* (reg8 *) LPp__SIO_REG_HIFREQ)
    #define LPp_SIO_CFG            (* (reg8 *) LPp__SIO_CFG)
    #define LPp_SIO_DIFF           (* (reg8 *) LPp__SIO_DIFF)
#endif /* (LPp__SIO_CFG) */

/* Interrupt Registers */
#if defined(LPp__INTSTAT)
    #define LPp_INTSTAT            (* (reg8 *) LPp__INTSTAT)
    #define LPp_SNAP               (* (reg8 *) LPp__SNAP)
    
	#define LPp_0_INTTYPE_REG 		(* (reg8 *) LPp__0__INTTYPE)
#endif /* (LPp__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_LPp_H */


/* [] END OF FILE */
