/*******************************************************************************
* File Name: LPo.h  
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

#if !defined(CY_PINS_LPo_H) /* Pins LPo_H */
#define CY_PINS_LPo_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "LPo_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 LPo__PORT == 15 && ((LPo__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    LPo_Write(uint8 value);
void    LPo_SetDriveMode(uint8 mode);
uint8   LPo_ReadDataReg(void);
uint8   LPo_Read(void);
void    LPo_SetInterruptMode(uint16 position, uint16 mode);
uint8   LPo_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the LPo_SetDriveMode() function.
     *  @{
     */
        #define LPo_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define LPo_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define LPo_DM_RES_UP          PIN_DM_RES_UP
        #define LPo_DM_RES_DWN         PIN_DM_RES_DWN
        #define LPo_DM_OD_LO           PIN_DM_OD_LO
        #define LPo_DM_OD_HI           PIN_DM_OD_HI
        #define LPo_DM_STRONG          PIN_DM_STRONG
        #define LPo_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define LPo_MASK               LPo__MASK
#define LPo_SHIFT              LPo__SHIFT
#define LPo_WIDTH              1u

/* Interrupt constants */
#if defined(LPo__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in LPo_SetInterruptMode() function.
     *  @{
     */
        #define LPo_INTR_NONE      (uint16)(0x0000u)
        #define LPo_INTR_RISING    (uint16)(0x0001u)
        #define LPo_INTR_FALLING   (uint16)(0x0002u)
        #define LPo_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define LPo_INTR_MASK      (0x01u) 
#endif /* (LPo__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define LPo_PS                     (* (reg8 *) LPo__PS)
/* Data Register */
#define LPo_DR                     (* (reg8 *) LPo__DR)
/* Port Number */
#define LPo_PRT_NUM                (* (reg8 *) LPo__PRT) 
/* Connect to Analog Globals */                                                  
#define LPo_AG                     (* (reg8 *) LPo__AG)                       
/* Analog MUX bux enable */
#define LPo_AMUX                   (* (reg8 *) LPo__AMUX) 
/* Bidirectional Enable */                                                        
#define LPo_BIE                    (* (reg8 *) LPo__BIE)
/* Bit-mask for Aliased Register Access */
#define LPo_BIT_MASK               (* (reg8 *) LPo__BIT_MASK)
/* Bypass Enable */
#define LPo_BYP                    (* (reg8 *) LPo__BYP)
/* Port wide control signals */                                                   
#define LPo_CTL                    (* (reg8 *) LPo__CTL)
/* Drive Modes */
#define LPo_DM0                    (* (reg8 *) LPo__DM0) 
#define LPo_DM1                    (* (reg8 *) LPo__DM1)
#define LPo_DM2                    (* (reg8 *) LPo__DM2) 
/* Input Buffer Disable Override */
#define LPo_INP_DIS                (* (reg8 *) LPo__INP_DIS)
/* LCD Common or Segment Drive */
#define LPo_LCD_COM_SEG            (* (reg8 *) LPo__LCD_COM_SEG)
/* Enable Segment LCD */
#define LPo_LCD_EN                 (* (reg8 *) LPo__LCD_EN)
/* Slew Rate Control */
#define LPo_SLW                    (* (reg8 *) LPo__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define LPo_PRTDSI__CAPS_SEL       (* (reg8 *) LPo__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define LPo_PRTDSI__DBL_SYNC_IN    (* (reg8 *) LPo__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define LPo_PRTDSI__OE_SEL0        (* (reg8 *) LPo__PRTDSI__OE_SEL0) 
#define LPo_PRTDSI__OE_SEL1        (* (reg8 *) LPo__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define LPo_PRTDSI__OUT_SEL0       (* (reg8 *) LPo__PRTDSI__OUT_SEL0) 
#define LPo_PRTDSI__OUT_SEL1       (* (reg8 *) LPo__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define LPo_PRTDSI__SYNC_OUT       (* (reg8 *) LPo__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(LPo__SIO_CFG)
    #define LPo_SIO_HYST_EN        (* (reg8 *) LPo__SIO_HYST_EN)
    #define LPo_SIO_REG_HIFREQ     (* (reg8 *) LPo__SIO_REG_HIFREQ)
    #define LPo_SIO_CFG            (* (reg8 *) LPo__SIO_CFG)
    #define LPo_SIO_DIFF           (* (reg8 *) LPo__SIO_DIFF)
#endif /* (LPo__SIO_CFG) */

/* Interrupt Registers */
#if defined(LPo__INTSTAT)
    #define LPo_INTSTAT            (* (reg8 *) LPo__INTSTAT)
    #define LPo_SNAP               (* (reg8 *) LPo__SNAP)
    
	#define LPo_0_INTTYPE_REG 		(* (reg8 *) LPo__0__INTTYPE)
#endif /* (LPo__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_LPo_H */


/* [] END OF FILE */
