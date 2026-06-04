/*******************************************************************************
* File Name: LPout.h  
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

#if !defined(CY_PINS_LPout_H) /* Pins LPout_H */
#define CY_PINS_LPout_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "LPout_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 LPout__PORT == 15 && ((LPout__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    LPout_Write(uint8 value);
void    LPout_SetDriveMode(uint8 mode);
uint8   LPout_ReadDataReg(void);
uint8   LPout_Read(void);
void    LPout_SetInterruptMode(uint16 position, uint16 mode);
uint8   LPout_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the LPout_SetDriveMode() function.
     *  @{
     */
        #define LPout_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define LPout_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define LPout_DM_RES_UP          PIN_DM_RES_UP
        #define LPout_DM_RES_DWN         PIN_DM_RES_DWN
        #define LPout_DM_OD_LO           PIN_DM_OD_LO
        #define LPout_DM_OD_HI           PIN_DM_OD_HI
        #define LPout_DM_STRONG          PIN_DM_STRONG
        #define LPout_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define LPout_MASK               LPout__MASK
#define LPout_SHIFT              LPout__SHIFT
#define LPout_WIDTH              1u

/* Interrupt constants */
#if defined(LPout__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in LPout_SetInterruptMode() function.
     *  @{
     */
        #define LPout_INTR_NONE      (uint16)(0x0000u)
        #define LPout_INTR_RISING    (uint16)(0x0001u)
        #define LPout_INTR_FALLING   (uint16)(0x0002u)
        #define LPout_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define LPout_INTR_MASK      (0x01u) 
#endif /* (LPout__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define LPout_PS                     (* (reg8 *) LPout__PS)
/* Data Register */
#define LPout_DR                     (* (reg8 *) LPout__DR)
/* Port Number */
#define LPout_PRT_NUM                (* (reg8 *) LPout__PRT) 
/* Connect to Analog Globals */                                                  
#define LPout_AG                     (* (reg8 *) LPout__AG)                       
/* Analog MUX bux enable */
#define LPout_AMUX                   (* (reg8 *) LPout__AMUX) 
/* Bidirectional Enable */                                                        
#define LPout_BIE                    (* (reg8 *) LPout__BIE)
/* Bit-mask for Aliased Register Access */
#define LPout_BIT_MASK               (* (reg8 *) LPout__BIT_MASK)
/* Bypass Enable */
#define LPout_BYP                    (* (reg8 *) LPout__BYP)
/* Port wide control signals */                                                   
#define LPout_CTL                    (* (reg8 *) LPout__CTL)
/* Drive Modes */
#define LPout_DM0                    (* (reg8 *) LPout__DM0) 
#define LPout_DM1                    (* (reg8 *) LPout__DM1)
#define LPout_DM2                    (* (reg8 *) LPout__DM2) 
/* Input Buffer Disable Override */
#define LPout_INP_DIS                (* (reg8 *) LPout__INP_DIS)
/* LCD Common or Segment Drive */
#define LPout_LCD_COM_SEG            (* (reg8 *) LPout__LCD_COM_SEG)
/* Enable Segment LCD */
#define LPout_LCD_EN                 (* (reg8 *) LPout__LCD_EN)
/* Slew Rate Control */
#define LPout_SLW                    (* (reg8 *) LPout__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define LPout_PRTDSI__CAPS_SEL       (* (reg8 *) LPout__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define LPout_PRTDSI__DBL_SYNC_IN    (* (reg8 *) LPout__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define LPout_PRTDSI__OE_SEL0        (* (reg8 *) LPout__PRTDSI__OE_SEL0) 
#define LPout_PRTDSI__OE_SEL1        (* (reg8 *) LPout__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define LPout_PRTDSI__OUT_SEL0       (* (reg8 *) LPout__PRTDSI__OUT_SEL0) 
#define LPout_PRTDSI__OUT_SEL1       (* (reg8 *) LPout__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define LPout_PRTDSI__SYNC_OUT       (* (reg8 *) LPout__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(LPout__SIO_CFG)
    #define LPout_SIO_HYST_EN        (* (reg8 *) LPout__SIO_HYST_EN)
    #define LPout_SIO_REG_HIFREQ     (* (reg8 *) LPout__SIO_REG_HIFREQ)
    #define LPout_SIO_CFG            (* (reg8 *) LPout__SIO_CFG)
    #define LPout_SIO_DIFF           (* (reg8 *) LPout__SIO_DIFF)
#endif /* (LPout__SIO_CFG) */

/* Interrupt Registers */
#if defined(LPout__INTSTAT)
    #define LPout_INTSTAT            (* (reg8 *) LPout__INTSTAT)
    #define LPout_SNAP               (* (reg8 *) LPout__SNAP)
    
	#define LPout_0_INTTYPE_REG 		(* (reg8 *) LPout__0__INTTYPE)
#endif /* (LPout__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_LPout_H */


/* [] END OF FILE */
