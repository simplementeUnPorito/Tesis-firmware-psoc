/*******************************************************************************
* File Name: ADDERp.h  
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

#if !defined(CY_PINS_ADDERp_H) /* Pins ADDERp_H */
#define CY_PINS_ADDERp_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "ADDERp_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 ADDERp__PORT == 15 && ((ADDERp__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    ADDERp_Write(uint8 value);
void    ADDERp_SetDriveMode(uint8 mode);
uint8   ADDERp_ReadDataReg(void);
uint8   ADDERp_Read(void);
void    ADDERp_SetInterruptMode(uint16 position, uint16 mode);
uint8   ADDERp_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the ADDERp_SetDriveMode() function.
     *  @{
     */
        #define ADDERp_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define ADDERp_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define ADDERp_DM_RES_UP          PIN_DM_RES_UP
        #define ADDERp_DM_RES_DWN         PIN_DM_RES_DWN
        #define ADDERp_DM_OD_LO           PIN_DM_OD_LO
        #define ADDERp_DM_OD_HI           PIN_DM_OD_HI
        #define ADDERp_DM_STRONG          PIN_DM_STRONG
        #define ADDERp_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define ADDERp_MASK               ADDERp__MASK
#define ADDERp_SHIFT              ADDERp__SHIFT
#define ADDERp_WIDTH              1u

/* Interrupt constants */
#if defined(ADDERp__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in ADDERp_SetInterruptMode() function.
     *  @{
     */
        #define ADDERp_INTR_NONE      (uint16)(0x0000u)
        #define ADDERp_INTR_RISING    (uint16)(0x0001u)
        #define ADDERp_INTR_FALLING   (uint16)(0x0002u)
        #define ADDERp_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define ADDERp_INTR_MASK      (0x01u) 
#endif /* (ADDERp__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define ADDERp_PS                     (* (reg8 *) ADDERp__PS)
/* Data Register */
#define ADDERp_DR                     (* (reg8 *) ADDERp__DR)
/* Port Number */
#define ADDERp_PRT_NUM                (* (reg8 *) ADDERp__PRT) 
/* Connect to Analog Globals */                                                  
#define ADDERp_AG                     (* (reg8 *) ADDERp__AG)                       
/* Analog MUX bux enable */
#define ADDERp_AMUX                   (* (reg8 *) ADDERp__AMUX) 
/* Bidirectional Enable */                                                        
#define ADDERp_BIE                    (* (reg8 *) ADDERp__BIE)
/* Bit-mask for Aliased Register Access */
#define ADDERp_BIT_MASK               (* (reg8 *) ADDERp__BIT_MASK)
/* Bypass Enable */
#define ADDERp_BYP                    (* (reg8 *) ADDERp__BYP)
/* Port wide control signals */                                                   
#define ADDERp_CTL                    (* (reg8 *) ADDERp__CTL)
/* Drive Modes */
#define ADDERp_DM0                    (* (reg8 *) ADDERp__DM0) 
#define ADDERp_DM1                    (* (reg8 *) ADDERp__DM1)
#define ADDERp_DM2                    (* (reg8 *) ADDERp__DM2) 
/* Input Buffer Disable Override */
#define ADDERp_INP_DIS                (* (reg8 *) ADDERp__INP_DIS)
/* LCD Common or Segment Drive */
#define ADDERp_LCD_COM_SEG            (* (reg8 *) ADDERp__LCD_COM_SEG)
/* Enable Segment LCD */
#define ADDERp_LCD_EN                 (* (reg8 *) ADDERp__LCD_EN)
/* Slew Rate Control */
#define ADDERp_SLW                    (* (reg8 *) ADDERp__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define ADDERp_PRTDSI__CAPS_SEL       (* (reg8 *) ADDERp__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define ADDERp_PRTDSI__DBL_SYNC_IN    (* (reg8 *) ADDERp__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define ADDERp_PRTDSI__OE_SEL0        (* (reg8 *) ADDERp__PRTDSI__OE_SEL0) 
#define ADDERp_PRTDSI__OE_SEL1        (* (reg8 *) ADDERp__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define ADDERp_PRTDSI__OUT_SEL0       (* (reg8 *) ADDERp__PRTDSI__OUT_SEL0) 
#define ADDERp_PRTDSI__OUT_SEL1       (* (reg8 *) ADDERp__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define ADDERp_PRTDSI__SYNC_OUT       (* (reg8 *) ADDERp__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(ADDERp__SIO_CFG)
    #define ADDERp_SIO_HYST_EN        (* (reg8 *) ADDERp__SIO_HYST_EN)
    #define ADDERp_SIO_REG_HIFREQ     (* (reg8 *) ADDERp__SIO_REG_HIFREQ)
    #define ADDERp_SIO_CFG            (* (reg8 *) ADDERp__SIO_CFG)
    #define ADDERp_SIO_DIFF           (* (reg8 *) ADDERp__SIO_DIFF)
#endif /* (ADDERp__SIO_CFG) */

/* Interrupt Registers */
#if defined(ADDERp__INTSTAT)
    #define ADDERp_INTSTAT            (* (reg8 *) ADDERp__INTSTAT)
    #define ADDERp_SNAP               (* (reg8 *) ADDERp__SNAP)
    
	#define ADDERp_0_INTTYPE_REG 		(* (reg8 *) ADDERp__0__INTTYPE)
#endif /* (ADDERp__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_ADDERp_H */


/* [] END OF FILE */
