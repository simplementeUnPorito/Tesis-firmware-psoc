/*******************************************************************************
* File Name: OpaVDAC_I.h  
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

#if !defined(CY_PINS_OpaVDAC_I_H) /* Pins OpaVDAC_I_H */
#define CY_PINS_OpaVDAC_I_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "OpaVDAC_I_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 OpaVDAC_I__PORT == 15 && ((OpaVDAC_I__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    OpaVDAC_I_Write(uint8 value);
void    OpaVDAC_I_SetDriveMode(uint8 mode);
uint8   OpaVDAC_I_ReadDataReg(void);
uint8   OpaVDAC_I_Read(void);
void    OpaVDAC_I_SetInterruptMode(uint16 position, uint16 mode);
uint8   OpaVDAC_I_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the OpaVDAC_I_SetDriveMode() function.
     *  @{
     */
        #define OpaVDAC_I_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define OpaVDAC_I_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define OpaVDAC_I_DM_RES_UP          PIN_DM_RES_UP
        #define OpaVDAC_I_DM_RES_DWN         PIN_DM_RES_DWN
        #define OpaVDAC_I_DM_OD_LO           PIN_DM_OD_LO
        #define OpaVDAC_I_DM_OD_HI           PIN_DM_OD_HI
        #define OpaVDAC_I_DM_STRONG          PIN_DM_STRONG
        #define OpaVDAC_I_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define OpaVDAC_I_MASK               OpaVDAC_I__MASK
#define OpaVDAC_I_SHIFT              OpaVDAC_I__SHIFT
#define OpaVDAC_I_WIDTH              1u

/* Interrupt constants */
#if defined(OpaVDAC_I__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in OpaVDAC_I_SetInterruptMode() function.
     *  @{
     */
        #define OpaVDAC_I_INTR_NONE      (uint16)(0x0000u)
        #define OpaVDAC_I_INTR_RISING    (uint16)(0x0001u)
        #define OpaVDAC_I_INTR_FALLING   (uint16)(0x0002u)
        #define OpaVDAC_I_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define OpaVDAC_I_INTR_MASK      (0x01u) 
#endif /* (OpaVDAC_I__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define OpaVDAC_I_PS                     (* (reg8 *) OpaVDAC_I__PS)
/* Data Register */
#define OpaVDAC_I_DR                     (* (reg8 *) OpaVDAC_I__DR)
/* Port Number */
#define OpaVDAC_I_PRT_NUM                (* (reg8 *) OpaVDAC_I__PRT) 
/* Connect to Analog Globals */                                                  
#define OpaVDAC_I_AG                     (* (reg8 *) OpaVDAC_I__AG)                       
/* Analog MUX bux enable */
#define OpaVDAC_I_AMUX                   (* (reg8 *) OpaVDAC_I__AMUX) 
/* Bidirectional Enable */                                                        
#define OpaVDAC_I_BIE                    (* (reg8 *) OpaVDAC_I__BIE)
/* Bit-mask for Aliased Register Access */
#define OpaVDAC_I_BIT_MASK               (* (reg8 *) OpaVDAC_I__BIT_MASK)
/* Bypass Enable */
#define OpaVDAC_I_BYP                    (* (reg8 *) OpaVDAC_I__BYP)
/* Port wide control signals */                                                   
#define OpaVDAC_I_CTL                    (* (reg8 *) OpaVDAC_I__CTL)
/* Drive Modes */
#define OpaVDAC_I_DM0                    (* (reg8 *) OpaVDAC_I__DM0) 
#define OpaVDAC_I_DM1                    (* (reg8 *) OpaVDAC_I__DM1)
#define OpaVDAC_I_DM2                    (* (reg8 *) OpaVDAC_I__DM2) 
/* Input Buffer Disable Override */
#define OpaVDAC_I_INP_DIS                (* (reg8 *) OpaVDAC_I__INP_DIS)
/* LCD Common or Segment Drive */
#define OpaVDAC_I_LCD_COM_SEG            (* (reg8 *) OpaVDAC_I__LCD_COM_SEG)
/* Enable Segment LCD */
#define OpaVDAC_I_LCD_EN                 (* (reg8 *) OpaVDAC_I__LCD_EN)
/* Slew Rate Control */
#define OpaVDAC_I_SLW                    (* (reg8 *) OpaVDAC_I__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define OpaVDAC_I_PRTDSI__CAPS_SEL       (* (reg8 *) OpaVDAC_I__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define OpaVDAC_I_PRTDSI__DBL_SYNC_IN    (* (reg8 *) OpaVDAC_I__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define OpaVDAC_I_PRTDSI__OE_SEL0        (* (reg8 *) OpaVDAC_I__PRTDSI__OE_SEL0) 
#define OpaVDAC_I_PRTDSI__OE_SEL1        (* (reg8 *) OpaVDAC_I__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define OpaVDAC_I_PRTDSI__OUT_SEL0       (* (reg8 *) OpaVDAC_I__PRTDSI__OUT_SEL0) 
#define OpaVDAC_I_PRTDSI__OUT_SEL1       (* (reg8 *) OpaVDAC_I__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define OpaVDAC_I_PRTDSI__SYNC_OUT       (* (reg8 *) OpaVDAC_I__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(OpaVDAC_I__SIO_CFG)
    #define OpaVDAC_I_SIO_HYST_EN        (* (reg8 *) OpaVDAC_I__SIO_HYST_EN)
    #define OpaVDAC_I_SIO_REG_HIFREQ     (* (reg8 *) OpaVDAC_I__SIO_REG_HIFREQ)
    #define OpaVDAC_I_SIO_CFG            (* (reg8 *) OpaVDAC_I__SIO_CFG)
    #define OpaVDAC_I_SIO_DIFF           (* (reg8 *) OpaVDAC_I__SIO_DIFF)
#endif /* (OpaVDAC_I__SIO_CFG) */

/* Interrupt Registers */
#if defined(OpaVDAC_I__INTSTAT)
    #define OpaVDAC_I_INTSTAT            (* (reg8 *) OpaVDAC_I__INTSTAT)
    #define OpaVDAC_I_SNAP               (* (reg8 *) OpaVDAC_I__SNAP)
    
	#define OpaVDAC_I_0_INTTYPE_REG 		(* (reg8 *) OpaVDAC_I__0__INTTYPE)
#endif /* (OpaVDAC_I__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_OpaVDAC_I_H */


/* [] END OF FILE */
