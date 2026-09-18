/*******************************************************************************
* File Name: LPin.h  
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

#if !defined(CY_PINS_LPin_H) /* Pins LPin_H */
#define CY_PINS_LPin_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "LPin_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 LPin__PORT == 15 && ((LPin__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    LPin_Write(uint8 value);
void    LPin_SetDriveMode(uint8 mode);
uint8   LPin_ReadDataReg(void);
uint8   LPin_Read(void);
void    LPin_SetInterruptMode(uint16 position, uint16 mode);
uint8   LPin_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the LPin_SetDriveMode() function.
     *  @{
     */
        #define LPin_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define LPin_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define LPin_DM_RES_UP          PIN_DM_RES_UP
        #define LPin_DM_RES_DWN         PIN_DM_RES_DWN
        #define LPin_DM_OD_LO           PIN_DM_OD_LO
        #define LPin_DM_OD_HI           PIN_DM_OD_HI
        #define LPin_DM_STRONG          PIN_DM_STRONG
        #define LPin_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define LPin_MASK               LPin__MASK
#define LPin_SHIFT              LPin__SHIFT
#define LPin_WIDTH              1u

/* Interrupt constants */
#if defined(LPin__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in LPin_SetInterruptMode() function.
     *  @{
     */
        #define LPin_INTR_NONE      (uint16)(0x0000u)
        #define LPin_INTR_RISING    (uint16)(0x0001u)
        #define LPin_INTR_FALLING   (uint16)(0x0002u)
        #define LPin_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define LPin_INTR_MASK      (0x01u) 
#endif /* (LPin__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define LPin_PS                     (* (reg8 *) LPin__PS)
/* Data Register */
#define LPin_DR                     (* (reg8 *) LPin__DR)
/* Port Number */
#define LPin_PRT_NUM                (* (reg8 *) LPin__PRT) 
/* Connect to Analog Globals */                                                  
#define LPin_AG                     (* (reg8 *) LPin__AG)                       
/* Analog MUX bux enable */
#define LPin_AMUX                   (* (reg8 *) LPin__AMUX) 
/* Bidirectional Enable */                                                        
#define LPin_BIE                    (* (reg8 *) LPin__BIE)
/* Bit-mask for Aliased Register Access */
#define LPin_BIT_MASK               (* (reg8 *) LPin__BIT_MASK)
/* Bypass Enable */
#define LPin_BYP                    (* (reg8 *) LPin__BYP)
/* Port wide control signals */                                                   
#define LPin_CTL                    (* (reg8 *) LPin__CTL)
/* Drive Modes */
#define LPin_DM0                    (* (reg8 *) LPin__DM0) 
#define LPin_DM1                    (* (reg8 *) LPin__DM1)
#define LPin_DM2                    (* (reg8 *) LPin__DM2) 
/* Input Buffer Disable Override */
#define LPin_INP_DIS                (* (reg8 *) LPin__INP_DIS)
/* LCD Common or Segment Drive */
#define LPin_LCD_COM_SEG            (* (reg8 *) LPin__LCD_COM_SEG)
/* Enable Segment LCD */
#define LPin_LCD_EN                 (* (reg8 *) LPin__LCD_EN)
/* Slew Rate Control */
#define LPin_SLW                    (* (reg8 *) LPin__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define LPin_PRTDSI__CAPS_SEL       (* (reg8 *) LPin__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define LPin_PRTDSI__DBL_SYNC_IN    (* (reg8 *) LPin__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define LPin_PRTDSI__OE_SEL0        (* (reg8 *) LPin__PRTDSI__OE_SEL0) 
#define LPin_PRTDSI__OE_SEL1        (* (reg8 *) LPin__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define LPin_PRTDSI__OUT_SEL0       (* (reg8 *) LPin__PRTDSI__OUT_SEL0) 
#define LPin_PRTDSI__OUT_SEL1       (* (reg8 *) LPin__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define LPin_PRTDSI__SYNC_OUT       (* (reg8 *) LPin__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(LPin__SIO_CFG)
    #define LPin_SIO_HYST_EN        (* (reg8 *) LPin__SIO_HYST_EN)
    #define LPin_SIO_REG_HIFREQ     (* (reg8 *) LPin__SIO_REG_HIFREQ)
    #define LPin_SIO_CFG            (* (reg8 *) LPin__SIO_CFG)
    #define LPin_SIO_DIFF           (* (reg8 *) LPin__SIO_DIFF)
#endif /* (LPin__SIO_CFG) */

/* Interrupt Registers */
#if defined(LPin__INTSTAT)
    #define LPin_INTSTAT            (* (reg8 *) LPin__INTSTAT)
    #define LPin_SNAP               (* (reg8 *) LPin__SNAP)
    
	#define LPin_0_INTTYPE_REG 		(* (reg8 *) LPin__0__INTTYPE)
#endif /* (LPin__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_LPin_H */


/* [] END OF FILE */
