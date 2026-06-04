/*******************************************************************************
* File Name: LPm.h  
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

#if !defined(CY_PINS_LPm_H) /* Pins LPm_H */
#define CY_PINS_LPm_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "LPm_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 LPm__PORT == 15 && ((LPm__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    LPm_Write(uint8 value);
void    LPm_SetDriveMode(uint8 mode);
uint8   LPm_ReadDataReg(void);
uint8   LPm_Read(void);
void    LPm_SetInterruptMode(uint16 position, uint16 mode);
uint8   LPm_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the LPm_SetDriveMode() function.
     *  @{
     */
        #define LPm_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define LPm_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define LPm_DM_RES_UP          PIN_DM_RES_UP
        #define LPm_DM_RES_DWN         PIN_DM_RES_DWN
        #define LPm_DM_OD_LO           PIN_DM_OD_LO
        #define LPm_DM_OD_HI           PIN_DM_OD_HI
        #define LPm_DM_STRONG          PIN_DM_STRONG
        #define LPm_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define LPm_MASK               LPm__MASK
#define LPm_SHIFT              LPm__SHIFT
#define LPm_WIDTH              1u

/* Interrupt constants */
#if defined(LPm__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in LPm_SetInterruptMode() function.
     *  @{
     */
        #define LPm_INTR_NONE      (uint16)(0x0000u)
        #define LPm_INTR_RISING    (uint16)(0x0001u)
        #define LPm_INTR_FALLING   (uint16)(0x0002u)
        #define LPm_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define LPm_INTR_MASK      (0x01u) 
#endif /* (LPm__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define LPm_PS                     (* (reg8 *) LPm__PS)
/* Data Register */
#define LPm_DR                     (* (reg8 *) LPm__DR)
/* Port Number */
#define LPm_PRT_NUM                (* (reg8 *) LPm__PRT) 
/* Connect to Analog Globals */                                                  
#define LPm_AG                     (* (reg8 *) LPm__AG)                       
/* Analog MUX bux enable */
#define LPm_AMUX                   (* (reg8 *) LPm__AMUX) 
/* Bidirectional Enable */                                                        
#define LPm_BIE                    (* (reg8 *) LPm__BIE)
/* Bit-mask for Aliased Register Access */
#define LPm_BIT_MASK               (* (reg8 *) LPm__BIT_MASK)
/* Bypass Enable */
#define LPm_BYP                    (* (reg8 *) LPm__BYP)
/* Port wide control signals */                                                   
#define LPm_CTL                    (* (reg8 *) LPm__CTL)
/* Drive Modes */
#define LPm_DM0                    (* (reg8 *) LPm__DM0) 
#define LPm_DM1                    (* (reg8 *) LPm__DM1)
#define LPm_DM2                    (* (reg8 *) LPm__DM2) 
/* Input Buffer Disable Override */
#define LPm_INP_DIS                (* (reg8 *) LPm__INP_DIS)
/* LCD Common or Segment Drive */
#define LPm_LCD_COM_SEG            (* (reg8 *) LPm__LCD_COM_SEG)
/* Enable Segment LCD */
#define LPm_LCD_EN                 (* (reg8 *) LPm__LCD_EN)
/* Slew Rate Control */
#define LPm_SLW                    (* (reg8 *) LPm__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define LPm_PRTDSI__CAPS_SEL       (* (reg8 *) LPm__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define LPm_PRTDSI__DBL_SYNC_IN    (* (reg8 *) LPm__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define LPm_PRTDSI__OE_SEL0        (* (reg8 *) LPm__PRTDSI__OE_SEL0) 
#define LPm_PRTDSI__OE_SEL1        (* (reg8 *) LPm__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define LPm_PRTDSI__OUT_SEL0       (* (reg8 *) LPm__PRTDSI__OUT_SEL0) 
#define LPm_PRTDSI__OUT_SEL1       (* (reg8 *) LPm__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define LPm_PRTDSI__SYNC_OUT       (* (reg8 *) LPm__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(LPm__SIO_CFG)
    #define LPm_SIO_HYST_EN        (* (reg8 *) LPm__SIO_HYST_EN)
    #define LPm_SIO_REG_HIFREQ     (* (reg8 *) LPm__SIO_REG_HIFREQ)
    #define LPm_SIO_CFG            (* (reg8 *) LPm__SIO_CFG)
    #define LPm_SIO_DIFF           (* (reg8 *) LPm__SIO_DIFF)
#endif /* (LPm__SIO_CFG) */

/* Interrupt Registers */
#if defined(LPm__INTSTAT)
    #define LPm_INTSTAT            (* (reg8 *) LPm__INTSTAT)
    #define LPm_SNAP               (* (reg8 *) LPm__SNAP)
    
	#define LPm_0_INTTYPE_REG 		(* (reg8 *) LPm__0__INTTYPE)
#endif /* (LPm__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_LPm_H */


/* [] END OF FILE */
