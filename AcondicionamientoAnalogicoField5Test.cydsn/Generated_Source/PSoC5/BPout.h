/*******************************************************************************
* File Name: BPout.h  
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

#if !defined(CY_PINS_BPout_H) /* Pins BPout_H */
#define CY_PINS_BPout_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "BPout_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 BPout__PORT == 15 && ((BPout__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    BPout_Write(uint8 value);
void    BPout_SetDriveMode(uint8 mode);
uint8   BPout_ReadDataReg(void);
uint8   BPout_Read(void);
void    BPout_SetInterruptMode(uint16 position, uint16 mode);
uint8   BPout_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the BPout_SetDriveMode() function.
     *  @{
     */
        #define BPout_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define BPout_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define BPout_DM_RES_UP          PIN_DM_RES_UP
        #define BPout_DM_RES_DWN         PIN_DM_RES_DWN
        #define BPout_DM_OD_LO           PIN_DM_OD_LO
        #define BPout_DM_OD_HI           PIN_DM_OD_HI
        #define BPout_DM_STRONG          PIN_DM_STRONG
        #define BPout_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define BPout_MASK               BPout__MASK
#define BPout_SHIFT              BPout__SHIFT
#define BPout_WIDTH              1u

/* Interrupt constants */
#if defined(BPout__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in BPout_SetInterruptMode() function.
     *  @{
     */
        #define BPout_INTR_NONE      (uint16)(0x0000u)
        #define BPout_INTR_RISING    (uint16)(0x0001u)
        #define BPout_INTR_FALLING   (uint16)(0x0002u)
        #define BPout_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define BPout_INTR_MASK      (0x01u) 
#endif /* (BPout__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define BPout_PS                     (* (reg8 *) BPout__PS)
/* Data Register */
#define BPout_DR                     (* (reg8 *) BPout__DR)
/* Port Number */
#define BPout_PRT_NUM                (* (reg8 *) BPout__PRT) 
/* Connect to Analog Globals */                                                  
#define BPout_AG                     (* (reg8 *) BPout__AG)                       
/* Analog MUX bux enable */
#define BPout_AMUX                   (* (reg8 *) BPout__AMUX) 
/* Bidirectional Enable */                                                        
#define BPout_BIE                    (* (reg8 *) BPout__BIE)
/* Bit-mask for Aliased Register Access */
#define BPout_BIT_MASK               (* (reg8 *) BPout__BIT_MASK)
/* Bypass Enable */
#define BPout_BYP                    (* (reg8 *) BPout__BYP)
/* Port wide control signals */                                                   
#define BPout_CTL                    (* (reg8 *) BPout__CTL)
/* Drive Modes */
#define BPout_DM0                    (* (reg8 *) BPout__DM0) 
#define BPout_DM1                    (* (reg8 *) BPout__DM1)
#define BPout_DM2                    (* (reg8 *) BPout__DM2) 
/* Input Buffer Disable Override */
#define BPout_INP_DIS                (* (reg8 *) BPout__INP_DIS)
/* LCD Common or Segment Drive */
#define BPout_LCD_COM_SEG            (* (reg8 *) BPout__LCD_COM_SEG)
/* Enable Segment LCD */
#define BPout_LCD_EN                 (* (reg8 *) BPout__LCD_EN)
/* Slew Rate Control */
#define BPout_SLW                    (* (reg8 *) BPout__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define BPout_PRTDSI__CAPS_SEL       (* (reg8 *) BPout__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define BPout_PRTDSI__DBL_SYNC_IN    (* (reg8 *) BPout__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define BPout_PRTDSI__OE_SEL0        (* (reg8 *) BPout__PRTDSI__OE_SEL0) 
#define BPout_PRTDSI__OE_SEL1        (* (reg8 *) BPout__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define BPout_PRTDSI__OUT_SEL0       (* (reg8 *) BPout__PRTDSI__OUT_SEL0) 
#define BPout_PRTDSI__OUT_SEL1       (* (reg8 *) BPout__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define BPout_PRTDSI__SYNC_OUT       (* (reg8 *) BPout__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(BPout__SIO_CFG)
    #define BPout_SIO_HYST_EN        (* (reg8 *) BPout__SIO_HYST_EN)
    #define BPout_SIO_REG_HIFREQ     (* (reg8 *) BPout__SIO_REG_HIFREQ)
    #define BPout_SIO_CFG            (* (reg8 *) BPout__SIO_CFG)
    #define BPout_SIO_DIFF           (* (reg8 *) BPout__SIO_DIFF)
#endif /* (BPout__SIO_CFG) */

/* Interrupt Registers */
#if defined(BPout__INTSTAT)
    #define BPout_INTSTAT            (* (reg8 *) BPout__INTSTAT)
    #define BPout_SNAP               (* (reg8 *) BPout__SNAP)
    
	#define BPout_0_INTTYPE_REG 		(* (reg8 *) BPout__0__INTTYPE)
#endif /* (BPout__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_BPout_H */


/* [] END OF FILE */
