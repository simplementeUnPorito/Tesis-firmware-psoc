/*******************************************************************************
* File Name: ADDERm.h  
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

#if !defined(CY_PINS_ADDERm_H) /* Pins ADDERm_H */
#define CY_PINS_ADDERm_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "ADDERm_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 ADDERm__PORT == 15 && ((ADDERm__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    ADDERm_Write(uint8 value);
void    ADDERm_SetDriveMode(uint8 mode);
uint8   ADDERm_ReadDataReg(void);
uint8   ADDERm_Read(void);
void    ADDERm_SetInterruptMode(uint16 position, uint16 mode);
uint8   ADDERm_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the ADDERm_SetDriveMode() function.
     *  @{
     */
        #define ADDERm_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define ADDERm_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define ADDERm_DM_RES_UP          PIN_DM_RES_UP
        #define ADDERm_DM_RES_DWN         PIN_DM_RES_DWN
        #define ADDERm_DM_OD_LO           PIN_DM_OD_LO
        #define ADDERm_DM_OD_HI           PIN_DM_OD_HI
        #define ADDERm_DM_STRONG          PIN_DM_STRONG
        #define ADDERm_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define ADDERm_MASK               ADDERm__MASK
#define ADDERm_SHIFT              ADDERm__SHIFT
#define ADDERm_WIDTH              1u

/* Interrupt constants */
#if defined(ADDERm__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in ADDERm_SetInterruptMode() function.
     *  @{
     */
        #define ADDERm_INTR_NONE      (uint16)(0x0000u)
        #define ADDERm_INTR_RISING    (uint16)(0x0001u)
        #define ADDERm_INTR_FALLING   (uint16)(0x0002u)
        #define ADDERm_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define ADDERm_INTR_MASK      (0x01u) 
#endif /* (ADDERm__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define ADDERm_PS                     (* (reg8 *) ADDERm__PS)
/* Data Register */
#define ADDERm_DR                     (* (reg8 *) ADDERm__DR)
/* Port Number */
#define ADDERm_PRT_NUM                (* (reg8 *) ADDERm__PRT) 
/* Connect to Analog Globals */                                                  
#define ADDERm_AG                     (* (reg8 *) ADDERm__AG)                       
/* Analog MUX bux enable */
#define ADDERm_AMUX                   (* (reg8 *) ADDERm__AMUX) 
/* Bidirectional Enable */                                                        
#define ADDERm_BIE                    (* (reg8 *) ADDERm__BIE)
/* Bit-mask for Aliased Register Access */
#define ADDERm_BIT_MASK               (* (reg8 *) ADDERm__BIT_MASK)
/* Bypass Enable */
#define ADDERm_BYP                    (* (reg8 *) ADDERm__BYP)
/* Port wide control signals */                                                   
#define ADDERm_CTL                    (* (reg8 *) ADDERm__CTL)
/* Drive Modes */
#define ADDERm_DM0                    (* (reg8 *) ADDERm__DM0) 
#define ADDERm_DM1                    (* (reg8 *) ADDERm__DM1)
#define ADDERm_DM2                    (* (reg8 *) ADDERm__DM2) 
/* Input Buffer Disable Override */
#define ADDERm_INP_DIS                (* (reg8 *) ADDERm__INP_DIS)
/* LCD Common or Segment Drive */
#define ADDERm_LCD_COM_SEG            (* (reg8 *) ADDERm__LCD_COM_SEG)
/* Enable Segment LCD */
#define ADDERm_LCD_EN                 (* (reg8 *) ADDERm__LCD_EN)
/* Slew Rate Control */
#define ADDERm_SLW                    (* (reg8 *) ADDERm__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define ADDERm_PRTDSI__CAPS_SEL       (* (reg8 *) ADDERm__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define ADDERm_PRTDSI__DBL_SYNC_IN    (* (reg8 *) ADDERm__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define ADDERm_PRTDSI__OE_SEL0        (* (reg8 *) ADDERm__PRTDSI__OE_SEL0) 
#define ADDERm_PRTDSI__OE_SEL1        (* (reg8 *) ADDERm__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define ADDERm_PRTDSI__OUT_SEL0       (* (reg8 *) ADDERm__PRTDSI__OUT_SEL0) 
#define ADDERm_PRTDSI__OUT_SEL1       (* (reg8 *) ADDERm__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define ADDERm_PRTDSI__SYNC_OUT       (* (reg8 *) ADDERm__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(ADDERm__SIO_CFG)
    #define ADDERm_SIO_HYST_EN        (* (reg8 *) ADDERm__SIO_HYST_EN)
    #define ADDERm_SIO_REG_HIFREQ     (* (reg8 *) ADDERm__SIO_REG_HIFREQ)
    #define ADDERm_SIO_CFG            (* (reg8 *) ADDERm__SIO_CFG)
    #define ADDERm_SIO_DIFF           (* (reg8 *) ADDERm__SIO_DIFF)
#endif /* (ADDERm__SIO_CFG) */

/* Interrupt Registers */
#if defined(ADDERm__INTSTAT)
    #define ADDERm_INTSTAT            (* (reg8 *) ADDERm__INTSTAT)
    #define ADDERm_SNAP               (* (reg8 *) ADDERm__SNAP)
    
	#define ADDERm_0_INTTYPE_REG 		(* (reg8 *) ADDERm__0__INTTYPE)
#endif /* (ADDERm__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_ADDERm_H */


/* [] END OF FILE */
