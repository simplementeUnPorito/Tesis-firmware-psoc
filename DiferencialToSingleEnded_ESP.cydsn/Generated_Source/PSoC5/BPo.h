/*******************************************************************************
* File Name: BPo.h  
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

#if !defined(CY_PINS_BPo_H) /* Pins BPo_H */
#define CY_PINS_BPo_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "BPo_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 BPo__PORT == 15 && ((BPo__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    BPo_Write(uint8 value);
void    BPo_SetDriveMode(uint8 mode);
uint8   BPo_ReadDataReg(void);
uint8   BPo_Read(void);
void    BPo_SetInterruptMode(uint16 position, uint16 mode);
uint8   BPo_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the BPo_SetDriveMode() function.
     *  @{
     */
        #define BPo_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define BPo_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define BPo_DM_RES_UP          PIN_DM_RES_UP
        #define BPo_DM_RES_DWN         PIN_DM_RES_DWN
        #define BPo_DM_OD_LO           PIN_DM_OD_LO
        #define BPo_DM_OD_HI           PIN_DM_OD_HI
        #define BPo_DM_STRONG          PIN_DM_STRONG
        #define BPo_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define BPo_MASK               BPo__MASK
#define BPo_SHIFT              BPo__SHIFT
#define BPo_WIDTH              1u

/* Interrupt constants */
#if defined(BPo__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in BPo_SetInterruptMode() function.
     *  @{
     */
        #define BPo_INTR_NONE      (uint16)(0x0000u)
        #define BPo_INTR_RISING    (uint16)(0x0001u)
        #define BPo_INTR_FALLING   (uint16)(0x0002u)
        #define BPo_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define BPo_INTR_MASK      (0x01u) 
#endif /* (BPo__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define BPo_PS                     (* (reg8 *) BPo__PS)
/* Data Register */
#define BPo_DR                     (* (reg8 *) BPo__DR)
/* Port Number */
#define BPo_PRT_NUM                (* (reg8 *) BPo__PRT) 
/* Connect to Analog Globals */                                                  
#define BPo_AG                     (* (reg8 *) BPo__AG)                       
/* Analog MUX bux enable */
#define BPo_AMUX                   (* (reg8 *) BPo__AMUX) 
/* Bidirectional Enable */                                                        
#define BPo_BIE                    (* (reg8 *) BPo__BIE)
/* Bit-mask for Aliased Register Access */
#define BPo_BIT_MASK               (* (reg8 *) BPo__BIT_MASK)
/* Bypass Enable */
#define BPo_BYP                    (* (reg8 *) BPo__BYP)
/* Port wide control signals */                                                   
#define BPo_CTL                    (* (reg8 *) BPo__CTL)
/* Drive Modes */
#define BPo_DM0                    (* (reg8 *) BPo__DM0) 
#define BPo_DM1                    (* (reg8 *) BPo__DM1)
#define BPo_DM2                    (* (reg8 *) BPo__DM2) 
/* Input Buffer Disable Override */
#define BPo_INP_DIS                (* (reg8 *) BPo__INP_DIS)
/* LCD Common or Segment Drive */
#define BPo_LCD_COM_SEG            (* (reg8 *) BPo__LCD_COM_SEG)
/* Enable Segment LCD */
#define BPo_LCD_EN                 (* (reg8 *) BPo__LCD_EN)
/* Slew Rate Control */
#define BPo_SLW                    (* (reg8 *) BPo__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define BPo_PRTDSI__CAPS_SEL       (* (reg8 *) BPo__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define BPo_PRTDSI__DBL_SYNC_IN    (* (reg8 *) BPo__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define BPo_PRTDSI__OE_SEL0        (* (reg8 *) BPo__PRTDSI__OE_SEL0) 
#define BPo_PRTDSI__OE_SEL1        (* (reg8 *) BPo__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define BPo_PRTDSI__OUT_SEL0       (* (reg8 *) BPo__PRTDSI__OUT_SEL0) 
#define BPo_PRTDSI__OUT_SEL1       (* (reg8 *) BPo__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define BPo_PRTDSI__SYNC_OUT       (* (reg8 *) BPo__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(BPo__SIO_CFG)
    #define BPo_SIO_HYST_EN        (* (reg8 *) BPo__SIO_HYST_EN)
    #define BPo_SIO_REG_HIFREQ     (* (reg8 *) BPo__SIO_REG_HIFREQ)
    #define BPo_SIO_CFG            (* (reg8 *) BPo__SIO_CFG)
    #define BPo_SIO_DIFF           (* (reg8 *) BPo__SIO_DIFF)
#endif /* (BPo__SIO_CFG) */

/* Interrupt Registers */
#if defined(BPo__INTSTAT)
    #define BPo_INTSTAT            (* (reg8 *) BPo__INTSTAT)
    #define BPo_SNAP               (* (reg8 *) BPo__SNAP)
    
	#define BPo_0_INTTYPE_REG 		(* (reg8 *) BPo__0__INTTYPE)
#endif /* (BPo__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_BPo_H */


/* [] END OF FILE */
