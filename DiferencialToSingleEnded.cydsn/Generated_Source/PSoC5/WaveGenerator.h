/*******************************************************************************
* File Name: WaveGenerator.h  
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

#if !defined(CY_PINS_WaveGenerator_H) /* Pins WaveGenerator_H */
#define CY_PINS_WaveGenerator_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "WaveGenerator_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 WaveGenerator__PORT == 15 && ((WaveGenerator__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    WaveGenerator_Write(uint8 value);
void    WaveGenerator_SetDriveMode(uint8 mode);
uint8   WaveGenerator_ReadDataReg(void);
uint8   WaveGenerator_Read(void);
void    WaveGenerator_SetInterruptMode(uint16 position, uint16 mode);
uint8   WaveGenerator_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the WaveGenerator_SetDriveMode() function.
     *  @{
     */
        #define WaveGenerator_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define WaveGenerator_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define WaveGenerator_DM_RES_UP          PIN_DM_RES_UP
        #define WaveGenerator_DM_RES_DWN         PIN_DM_RES_DWN
        #define WaveGenerator_DM_OD_LO           PIN_DM_OD_LO
        #define WaveGenerator_DM_OD_HI           PIN_DM_OD_HI
        #define WaveGenerator_DM_STRONG          PIN_DM_STRONG
        #define WaveGenerator_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define WaveGenerator_MASK               WaveGenerator__MASK
#define WaveGenerator_SHIFT              WaveGenerator__SHIFT
#define WaveGenerator_WIDTH              1u

/* Interrupt constants */
#if defined(WaveGenerator__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in WaveGenerator_SetInterruptMode() function.
     *  @{
     */
        #define WaveGenerator_INTR_NONE      (uint16)(0x0000u)
        #define WaveGenerator_INTR_RISING    (uint16)(0x0001u)
        #define WaveGenerator_INTR_FALLING   (uint16)(0x0002u)
        #define WaveGenerator_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define WaveGenerator_INTR_MASK      (0x01u) 
#endif /* (WaveGenerator__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define WaveGenerator_PS                     (* (reg8 *) WaveGenerator__PS)
/* Data Register */
#define WaveGenerator_DR                     (* (reg8 *) WaveGenerator__DR)
/* Port Number */
#define WaveGenerator_PRT_NUM                (* (reg8 *) WaveGenerator__PRT) 
/* Connect to Analog Globals */                                                  
#define WaveGenerator_AG                     (* (reg8 *) WaveGenerator__AG)                       
/* Analog MUX bux enable */
#define WaveGenerator_AMUX                   (* (reg8 *) WaveGenerator__AMUX) 
/* Bidirectional Enable */                                                        
#define WaveGenerator_BIE                    (* (reg8 *) WaveGenerator__BIE)
/* Bit-mask for Aliased Register Access */
#define WaveGenerator_BIT_MASK               (* (reg8 *) WaveGenerator__BIT_MASK)
/* Bypass Enable */
#define WaveGenerator_BYP                    (* (reg8 *) WaveGenerator__BYP)
/* Port wide control signals */                                                   
#define WaveGenerator_CTL                    (* (reg8 *) WaveGenerator__CTL)
/* Drive Modes */
#define WaveGenerator_DM0                    (* (reg8 *) WaveGenerator__DM0) 
#define WaveGenerator_DM1                    (* (reg8 *) WaveGenerator__DM1)
#define WaveGenerator_DM2                    (* (reg8 *) WaveGenerator__DM2) 
/* Input Buffer Disable Override */
#define WaveGenerator_INP_DIS                (* (reg8 *) WaveGenerator__INP_DIS)
/* LCD Common or Segment Drive */
#define WaveGenerator_LCD_COM_SEG            (* (reg8 *) WaveGenerator__LCD_COM_SEG)
/* Enable Segment LCD */
#define WaveGenerator_LCD_EN                 (* (reg8 *) WaveGenerator__LCD_EN)
/* Slew Rate Control */
#define WaveGenerator_SLW                    (* (reg8 *) WaveGenerator__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define WaveGenerator_PRTDSI__CAPS_SEL       (* (reg8 *) WaveGenerator__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define WaveGenerator_PRTDSI__DBL_SYNC_IN    (* (reg8 *) WaveGenerator__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define WaveGenerator_PRTDSI__OE_SEL0        (* (reg8 *) WaveGenerator__PRTDSI__OE_SEL0) 
#define WaveGenerator_PRTDSI__OE_SEL1        (* (reg8 *) WaveGenerator__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define WaveGenerator_PRTDSI__OUT_SEL0       (* (reg8 *) WaveGenerator__PRTDSI__OUT_SEL0) 
#define WaveGenerator_PRTDSI__OUT_SEL1       (* (reg8 *) WaveGenerator__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define WaveGenerator_PRTDSI__SYNC_OUT       (* (reg8 *) WaveGenerator__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(WaveGenerator__SIO_CFG)
    #define WaveGenerator_SIO_HYST_EN        (* (reg8 *) WaveGenerator__SIO_HYST_EN)
    #define WaveGenerator_SIO_REG_HIFREQ     (* (reg8 *) WaveGenerator__SIO_REG_HIFREQ)
    #define WaveGenerator_SIO_CFG            (* (reg8 *) WaveGenerator__SIO_CFG)
    #define WaveGenerator_SIO_DIFF           (* (reg8 *) WaveGenerator__SIO_DIFF)
#endif /* (WaveGenerator__SIO_CFG) */

/* Interrupt Registers */
#if defined(WaveGenerator__INTSTAT)
    #define WaveGenerator_INTSTAT            (* (reg8 *) WaveGenerator__INTSTAT)
    #define WaveGenerator_SNAP               (* (reg8 *) WaveGenerator__SNAP)
    
	#define WaveGenerator_0_INTTYPE_REG 		(* (reg8 *) WaveGenerator__0__INTTYPE)
#endif /* (WaveGenerator__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_WaveGenerator_H */


/* [] END OF FILE */
