/*******************************************************************************
* File Name: esp_reset.h  
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

#if !defined(CY_PINS_esp_reset_H) /* Pins esp_reset_H */
#define CY_PINS_esp_reset_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "esp_reset_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 esp_reset__PORT == 15 && ((esp_reset__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    esp_reset_Write(uint8 value);
void    esp_reset_SetDriveMode(uint8 mode);
uint8   esp_reset_ReadDataReg(void);
uint8   esp_reset_Read(void);
void    esp_reset_SetInterruptMode(uint16 position, uint16 mode);
uint8   esp_reset_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the esp_reset_SetDriveMode() function.
     *  @{
     */
        #define esp_reset_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define esp_reset_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define esp_reset_DM_RES_UP          PIN_DM_RES_UP
        #define esp_reset_DM_RES_DWN         PIN_DM_RES_DWN
        #define esp_reset_DM_OD_LO           PIN_DM_OD_LO
        #define esp_reset_DM_OD_HI           PIN_DM_OD_HI
        #define esp_reset_DM_STRONG          PIN_DM_STRONG
        #define esp_reset_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define esp_reset_MASK               esp_reset__MASK
#define esp_reset_SHIFT              esp_reset__SHIFT
#define esp_reset_WIDTH              1u

/* Interrupt constants */
#if defined(esp_reset__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in esp_reset_SetInterruptMode() function.
     *  @{
     */
        #define esp_reset_INTR_NONE      (uint16)(0x0000u)
        #define esp_reset_INTR_RISING    (uint16)(0x0001u)
        #define esp_reset_INTR_FALLING   (uint16)(0x0002u)
        #define esp_reset_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define esp_reset_INTR_MASK      (0x01u) 
#endif /* (esp_reset__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define esp_reset_PS                     (* (reg8 *) esp_reset__PS)
/* Data Register */
#define esp_reset_DR                     (* (reg8 *) esp_reset__DR)
/* Port Number */
#define esp_reset_PRT_NUM                (* (reg8 *) esp_reset__PRT) 
/* Connect to Analog Globals */                                                  
#define esp_reset_AG                     (* (reg8 *) esp_reset__AG)                       
/* Analog MUX bux enable */
#define esp_reset_AMUX                   (* (reg8 *) esp_reset__AMUX) 
/* Bidirectional Enable */                                                        
#define esp_reset_BIE                    (* (reg8 *) esp_reset__BIE)
/* Bit-mask for Aliased Register Access */
#define esp_reset_BIT_MASK               (* (reg8 *) esp_reset__BIT_MASK)
/* Bypass Enable */
#define esp_reset_BYP                    (* (reg8 *) esp_reset__BYP)
/* Port wide control signals */                                                   
#define esp_reset_CTL                    (* (reg8 *) esp_reset__CTL)
/* Drive Modes */
#define esp_reset_DM0                    (* (reg8 *) esp_reset__DM0) 
#define esp_reset_DM1                    (* (reg8 *) esp_reset__DM1)
#define esp_reset_DM2                    (* (reg8 *) esp_reset__DM2) 
/* Input Buffer Disable Override */
#define esp_reset_INP_DIS                (* (reg8 *) esp_reset__INP_DIS)
/* LCD Common or Segment Drive */
#define esp_reset_LCD_COM_SEG            (* (reg8 *) esp_reset__LCD_COM_SEG)
/* Enable Segment LCD */
#define esp_reset_LCD_EN                 (* (reg8 *) esp_reset__LCD_EN)
/* Slew Rate Control */
#define esp_reset_SLW                    (* (reg8 *) esp_reset__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define esp_reset_PRTDSI__CAPS_SEL       (* (reg8 *) esp_reset__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define esp_reset_PRTDSI__DBL_SYNC_IN    (* (reg8 *) esp_reset__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define esp_reset_PRTDSI__OE_SEL0        (* (reg8 *) esp_reset__PRTDSI__OE_SEL0) 
#define esp_reset_PRTDSI__OE_SEL1        (* (reg8 *) esp_reset__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define esp_reset_PRTDSI__OUT_SEL0       (* (reg8 *) esp_reset__PRTDSI__OUT_SEL0) 
#define esp_reset_PRTDSI__OUT_SEL1       (* (reg8 *) esp_reset__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define esp_reset_PRTDSI__SYNC_OUT       (* (reg8 *) esp_reset__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(esp_reset__SIO_CFG)
    #define esp_reset_SIO_HYST_EN        (* (reg8 *) esp_reset__SIO_HYST_EN)
    #define esp_reset_SIO_REG_HIFREQ     (* (reg8 *) esp_reset__SIO_REG_HIFREQ)
    #define esp_reset_SIO_CFG            (* (reg8 *) esp_reset__SIO_CFG)
    #define esp_reset_SIO_DIFF           (* (reg8 *) esp_reset__SIO_DIFF)
#endif /* (esp_reset__SIO_CFG) */

/* Interrupt Registers */
#if defined(esp_reset__INTSTAT)
    #define esp_reset_INTSTAT            (* (reg8 *) esp_reset__INTSTAT)
    #define esp_reset_SNAP               (* (reg8 *) esp_reset__SNAP)
    
	#define esp_reset_0_INTTYPE_REG 		(* (reg8 *) esp_reset__0__INTTYPE)
#endif /* (esp_reset__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_esp_reset_H */


/* [] END OF FILE */
