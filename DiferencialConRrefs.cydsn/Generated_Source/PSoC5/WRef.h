/*******************************************************************************
* File Name: WRef.h  
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

#if !defined(CY_PINS_WRef_H) /* Pins WRef_H */
#define CY_PINS_WRef_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "WRef_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 WRef__PORT == 15 && ((WRef__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    WRef_Write(uint8 value);
void    WRef_SetDriveMode(uint8 mode);
uint8   WRef_ReadDataReg(void);
uint8   WRef_Read(void);
void    WRef_SetInterruptMode(uint16 position, uint16 mode);
uint8   WRef_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the WRef_SetDriveMode() function.
     *  @{
     */
        #define WRef_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define WRef_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define WRef_DM_RES_UP          PIN_DM_RES_UP
        #define WRef_DM_RES_DWN         PIN_DM_RES_DWN
        #define WRef_DM_OD_LO           PIN_DM_OD_LO
        #define WRef_DM_OD_HI           PIN_DM_OD_HI
        #define WRef_DM_STRONG          PIN_DM_STRONG
        #define WRef_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define WRef_MASK               WRef__MASK
#define WRef_SHIFT              WRef__SHIFT
#define WRef_WIDTH              1u

/* Interrupt constants */
#if defined(WRef__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in WRef_SetInterruptMode() function.
     *  @{
     */
        #define WRef_INTR_NONE      (uint16)(0x0000u)
        #define WRef_INTR_RISING    (uint16)(0x0001u)
        #define WRef_INTR_FALLING   (uint16)(0x0002u)
        #define WRef_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define WRef_INTR_MASK      (0x01u) 
#endif /* (WRef__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define WRef_PS                     (* (reg8 *) WRef__PS)
/* Data Register */
#define WRef_DR                     (* (reg8 *) WRef__DR)
/* Port Number */
#define WRef_PRT_NUM                (* (reg8 *) WRef__PRT) 
/* Connect to Analog Globals */                                                  
#define WRef_AG                     (* (reg8 *) WRef__AG)                       
/* Analog MUX bux enable */
#define WRef_AMUX                   (* (reg8 *) WRef__AMUX) 
/* Bidirectional Enable */                                                        
#define WRef_BIE                    (* (reg8 *) WRef__BIE)
/* Bit-mask for Aliased Register Access */
#define WRef_BIT_MASK               (* (reg8 *) WRef__BIT_MASK)
/* Bypass Enable */
#define WRef_BYP                    (* (reg8 *) WRef__BYP)
/* Port wide control signals */                                                   
#define WRef_CTL                    (* (reg8 *) WRef__CTL)
/* Drive Modes */
#define WRef_DM0                    (* (reg8 *) WRef__DM0) 
#define WRef_DM1                    (* (reg8 *) WRef__DM1)
#define WRef_DM2                    (* (reg8 *) WRef__DM2) 
/* Input Buffer Disable Override */
#define WRef_INP_DIS                (* (reg8 *) WRef__INP_DIS)
/* LCD Common or Segment Drive */
#define WRef_LCD_COM_SEG            (* (reg8 *) WRef__LCD_COM_SEG)
/* Enable Segment LCD */
#define WRef_LCD_EN                 (* (reg8 *) WRef__LCD_EN)
/* Slew Rate Control */
#define WRef_SLW                    (* (reg8 *) WRef__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define WRef_PRTDSI__CAPS_SEL       (* (reg8 *) WRef__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define WRef_PRTDSI__DBL_SYNC_IN    (* (reg8 *) WRef__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define WRef_PRTDSI__OE_SEL0        (* (reg8 *) WRef__PRTDSI__OE_SEL0) 
#define WRef_PRTDSI__OE_SEL1        (* (reg8 *) WRef__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define WRef_PRTDSI__OUT_SEL0       (* (reg8 *) WRef__PRTDSI__OUT_SEL0) 
#define WRef_PRTDSI__OUT_SEL1       (* (reg8 *) WRef__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define WRef_PRTDSI__SYNC_OUT       (* (reg8 *) WRef__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(WRef__SIO_CFG)
    #define WRef_SIO_HYST_EN        (* (reg8 *) WRef__SIO_HYST_EN)
    #define WRef_SIO_REG_HIFREQ     (* (reg8 *) WRef__SIO_REG_HIFREQ)
    #define WRef_SIO_CFG            (* (reg8 *) WRef__SIO_CFG)
    #define WRef_SIO_DIFF           (* (reg8 *) WRef__SIO_DIFF)
#endif /* (WRef__SIO_CFG) */

/* Interrupt Registers */
#if defined(WRef__INTSTAT)
    #define WRef_INTSTAT            (* (reg8 *) WRef__INTSTAT)
    #define WRef_SNAP               (* (reg8 *) WRef__SNAP)
    
	#define WRef_0_INTTYPE_REG 		(* (reg8 *) WRef__0__INTTYPE)
#endif /* (WRef__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_WRef_H */


/* [] END OF FILE */
