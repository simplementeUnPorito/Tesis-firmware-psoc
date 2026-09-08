/*******************************************************************************
* File Name: ADDERo.h  
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

#if !defined(CY_PINS_ADDERo_H) /* Pins ADDERo_H */
#define CY_PINS_ADDERo_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "ADDERo_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 ADDERo__PORT == 15 && ((ADDERo__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    ADDERo_Write(uint8 value);
void    ADDERo_SetDriveMode(uint8 mode);
uint8   ADDERo_ReadDataReg(void);
uint8   ADDERo_Read(void);
void    ADDERo_SetInterruptMode(uint16 position, uint16 mode);
uint8   ADDERo_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the ADDERo_SetDriveMode() function.
     *  @{
     */
        #define ADDERo_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define ADDERo_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define ADDERo_DM_RES_UP          PIN_DM_RES_UP
        #define ADDERo_DM_RES_DWN         PIN_DM_RES_DWN
        #define ADDERo_DM_OD_LO           PIN_DM_OD_LO
        #define ADDERo_DM_OD_HI           PIN_DM_OD_HI
        #define ADDERo_DM_STRONG          PIN_DM_STRONG
        #define ADDERo_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define ADDERo_MASK               ADDERo__MASK
#define ADDERo_SHIFT              ADDERo__SHIFT
#define ADDERo_WIDTH              1u

/* Interrupt constants */
#if defined(ADDERo__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in ADDERo_SetInterruptMode() function.
     *  @{
     */
        #define ADDERo_INTR_NONE      (uint16)(0x0000u)
        #define ADDERo_INTR_RISING    (uint16)(0x0001u)
        #define ADDERo_INTR_FALLING   (uint16)(0x0002u)
        #define ADDERo_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define ADDERo_INTR_MASK      (0x01u) 
#endif /* (ADDERo__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define ADDERo_PS                     (* (reg8 *) ADDERo__PS)
/* Data Register */
#define ADDERo_DR                     (* (reg8 *) ADDERo__DR)
/* Port Number */
#define ADDERo_PRT_NUM                (* (reg8 *) ADDERo__PRT) 
/* Connect to Analog Globals */                                                  
#define ADDERo_AG                     (* (reg8 *) ADDERo__AG)                       
/* Analog MUX bux enable */
#define ADDERo_AMUX                   (* (reg8 *) ADDERo__AMUX) 
/* Bidirectional Enable */                                                        
#define ADDERo_BIE                    (* (reg8 *) ADDERo__BIE)
/* Bit-mask for Aliased Register Access */
#define ADDERo_BIT_MASK               (* (reg8 *) ADDERo__BIT_MASK)
/* Bypass Enable */
#define ADDERo_BYP                    (* (reg8 *) ADDERo__BYP)
/* Port wide control signals */                                                   
#define ADDERo_CTL                    (* (reg8 *) ADDERo__CTL)
/* Drive Modes */
#define ADDERo_DM0                    (* (reg8 *) ADDERo__DM0) 
#define ADDERo_DM1                    (* (reg8 *) ADDERo__DM1)
#define ADDERo_DM2                    (* (reg8 *) ADDERo__DM2) 
/* Input Buffer Disable Override */
#define ADDERo_INP_DIS                (* (reg8 *) ADDERo__INP_DIS)
/* LCD Common or Segment Drive */
#define ADDERo_LCD_COM_SEG            (* (reg8 *) ADDERo__LCD_COM_SEG)
/* Enable Segment LCD */
#define ADDERo_LCD_EN                 (* (reg8 *) ADDERo__LCD_EN)
/* Slew Rate Control */
#define ADDERo_SLW                    (* (reg8 *) ADDERo__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define ADDERo_PRTDSI__CAPS_SEL       (* (reg8 *) ADDERo__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define ADDERo_PRTDSI__DBL_SYNC_IN    (* (reg8 *) ADDERo__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define ADDERo_PRTDSI__OE_SEL0        (* (reg8 *) ADDERo__PRTDSI__OE_SEL0) 
#define ADDERo_PRTDSI__OE_SEL1        (* (reg8 *) ADDERo__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define ADDERo_PRTDSI__OUT_SEL0       (* (reg8 *) ADDERo__PRTDSI__OUT_SEL0) 
#define ADDERo_PRTDSI__OUT_SEL1       (* (reg8 *) ADDERo__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define ADDERo_PRTDSI__SYNC_OUT       (* (reg8 *) ADDERo__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(ADDERo__SIO_CFG)
    #define ADDERo_SIO_HYST_EN        (* (reg8 *) ADDERo__SIO_HYST_EN)
    #define ADDERo_SIO_REG_HIFREQ     (* (reg8 *) ADDERo__SIO_REG_HIFREQ)
    #define ADDERo_SIO_CFG            (* (reg8 *) ADDERo__SIO_CFG)
    #define ADDERo_SIO_DIFF           (* (reg8 *) ADDERo__SIO_DIFF)
#endif /* (ADDERo__SIO_CFG) */

/* Interrupt Registers */
#if defined(ADDERo__INTSTAT)
    #define ADDERo_INTSTAT            (* (reg8 *) ADDERo__INTSTAT)
    #define ADDERo_SNAP               (* (reg8 *) ADDERo__SNAP)
    
	#define ADDERo_0_INTTYPE_REG 		(* (reg8 *) ADDERo__0__INTTYPE)
#endif /* (ADDERo__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_ADDERo_H */


/* [] END OF FILE */
