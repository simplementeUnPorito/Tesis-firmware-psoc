/*******************************************************************************
* File Name: INn.h  
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

#if !defined(CY_PINS_INn_H) /* Pins INn_H */
#define CY_PINS_INn_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "INn_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 INn__PORT == 15 && ((INn__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    INn_Write(uint8 value);
void    INn_SetDriveMode(uint8 mode);
uint8   INn_ReadDataReg(void);
uint8   INn_Read(void);
void    INn_SetInterruptMode(uint16 position, uint16 mode);
uint8   INn_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the INn_SetDriveMode() function.
     *  @{
     */
        #define INn_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define INn_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define INn_DM_RES_UP          PIN_DM_RES_UP
        #define INn_DM_RES_DWN         PIN_DM_RES_DWN
        #define INn_DM_OD_LO           PIN_DM_OD_LO
        #define INn_DM_OD_HI           PIN_DM_OD_HI
        #define INn_DM_STRONG          PIN_DM_STRONG
        #define INn_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define INn_MASK               INn__MASK
#define INn_SHIFT              INn__SHIFT
#define INn_WIDTH              1u

/* Interrupt constants */
#if defined(INn__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in INn_SetInterruptMode() function.
     *  @{
     */
        #define INn_INTR_NONE      (uint16)(0x0000u)
        #define INn_INTR_RISING    (uint16)(0x0001u)
        #define INn_INTR_FALLING   (uint16)(0x0002u)
        #define INn_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define INn_INTR_MASK      (0x01u) 
#endif /* (INn__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define INn_PS                     (* (reg8 *) INn__PS)
/* Data Register */
#define INn_DR                     (* (reg8 *) INn__DR)
/* Port Number */
#define INn_PRT_NUM                (* (reg8 *) INn__PRT) 
/* Connect to Analog Globals */                                                  
#define INn_AG                     (* (reg8 *) INn__AG)                       
/* Analog MUX bux enable */
#define INn_AMUX                   (* (reg8 *) INn__AMUX) 
/* Bidirectional Enable */                                                        
#define INn_BIE                    (* (reg8 *) INn__BIE)
/* Bit-mask for Aliased Register Access */
#define INn_BIT_MASK               (* (reg8 *) INn__BIT_MASK)
/* Bypass Enable */
#define INn_BYP                    (* (reg8 *) INn__BYP)
/* Port wide control signals */                                                   
#define INn_CTL                    (* (reg8 *) INn__CTL)
/* Drive Modes */
#define INn_DM0                    (* (reg8 *) INn__DM0) 
#define INn_DM1                    (* (reg8 *) INn__DM1)
#define INn_DM2                    (* (reg8 *) INn__DM2) 
/* Input Buffer Disable Override */
#define INn_INP_DIS                (* (reg8 *) INn__INP_DIS)
/* LCD Common or Segment Drive */
#define INn_LCD_COM_SEG            (* (reg8 *) INn__LCD_COM_SEG)
/* Enable Segment LCD */
#define INn_LCD_EN                 (* (reg8 *) INn__LCD_EN)
/* Slew Rate Control */
#define INn_SLW                    (* (reg8 *) INn__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define INn_PRTDSI__CAPS_SEL       (* (reg8 *) INn__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define INn_PRTDSI__DBL_SYNC_IN    (* (reg8 *) INn__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define INn_PRTDSI__OE_SEL0        (* (reg8 *) INn__PRTDSI__OE_SEL0) 
#define INn_PRTDSI__OE_SEL1        (* (reg8 *) INn__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define INn_PRTDSI__OUT_SEL0       (* (reg8 *) INn__PRTDSI__OUT_SEL0) 
#define INn_PRTDSI__OUT_SEL1       (* (reg8 *) INn__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define INn_PRTDSI__SYNC_OUT       (* (reg8 *) INn__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(INn__SIO_CFG)
    #define INn_SIO_HYST_EN        (* (reg8 *) INn__SIO_HYST_EN)
    #define INn_SIO_REG_HIFREQ     (* (reg8 *) INn__SIO_REG_HIFREQ)
    #define INn_SIO_CFG            (* (reg8 *) INn__SIO_CFG)
    #define INn_SIO_DIFF           (* (reg8 *) INn__SIO_DIFF)
#endif /* (INn__SIO_CFG) */

/* Interrupt Registers */
#if defined(INn__INTSTAT)
    #define INn_INTSTAT            (* (reg8 *) INn__INTSTAT)
    #define INn_SNAP               (* (reg8 *) INn__SNAP)
    
	#define INn_0_INTTYPE_REG 		(* (reg8 *) INn__0__INTTYPE)
#endif /* (INn__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_INn_H */


/* [] END OF FILE */
