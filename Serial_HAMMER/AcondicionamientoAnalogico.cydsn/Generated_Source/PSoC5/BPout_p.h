/*******************************************************************************
* File Name: BPout_p.h  
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

#if !defined(CY_PINS_BPout_p_H) /* Pins BPout_p_H */
#define CY_PINS_BPout_p_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "BPout_p_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 BPout_p__PORT == 15 && ((BPout_p__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    BPout_p_Write(uint8 value);
void    BPout_p_SetDriveMode(uint8 mode);
uint8   BPout_p_ReadDataReg(void);
uint8   BPout_p_Read(void);
void    BPout_p_SetInterruptMode(uint16 position, uint16 mode);
uint8   BPout_p_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the BPout_p_SetDriveMode() function.
     *  @{
     */
        #define BPout_p_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define BPout_p_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define BPout_p_DM_RES_UP          PIN_DM_RES_UP
        #define BPout_p_DM_RES_DWN         PIN_DM_RES_DWN
        #define BPout_p_DM_OD_LO           PIN_DM_OD_LO
        #define BPout_p_DM_OD_HI           PIN_DM_OD_HI
        #define BPout_p_DM_STRONG          PIN_DM_STRONG
        #define BPout_p_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define BPout_p_MASK               BPout_p__MASK
#define BPout_p_SHIFT              BPout_p__SHIFT
#define BPout_p_WIDTH              1u

/* Interrupt constants */
#if defined(BPout_p__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in BPout_p_SetInterruptMode() function.
     *  @{
     */
        #define BPout_p_INTR_NONE      (uint16)(0x0000u)
        #define BPout_p_INTR_RISING    (uint16)(0x0001u)
        #define BPout_p_INTR_FALLING   (uint16)(0x0002u)
        #define BPout_p_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define BPout_p_INTR_MASK      (0x01u) 
#endif /* (BPout_p__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define BPout_p_PS                     (* (reg8 *) BPout_p__PS)
/* Data Register */
#define BPout_p_DR                     (* (reg8 *) BPout_p__DR)
/* Port Number */
#define BPout_p_PRT_NUM                (* (reg8 *) BPout_p__PRT) 
/* Connect to Analog Globals */                                                  
#define BPout_p_AG                     (* (reg8 *) BPout_p__AG)                       
/* Analog MUX bux enable */
#define BPout_p_AMUX                   (* (reg8 *) BPout_p__AMUX) 
/* Bidirectional Enable */                                                        
#define BPout_p_BIE                    (* (reg8 *) BPout_p__BIE)
/* Bit-mask for Aliased Register Access */
#define BPout_p_BIT_MASK               (* (reg8 *) BPout_p__BIT_MASK)
/* Bypass Enable */
#define BPout_p_BYP                    (* (reg8 *) BPout_p__BYP)
/* Port wide control signals */                                                   
#define BPout_p_CTL                    (* (reg8 *) BPout_p__CTL)
/* Drive Modes */
#define BPout_p_DM0                    (* (reg8 *) BPout_p__DM0) 
#define BPout_p_DM1                    (* (reg8 *) BPout_p__DM1)
#define BPout_p_DM2                    (* (reg8 *) BPout_p__DM2) 
/* Input Buffer Disable Override */
#define BPout_p_INP_DIS                (* (reg8 *) BPout_p__INP_DIS)
/* LCD Common or Segment Drive */
#define BPout_p_LCD_COM_SEG            (* (reg8 *) BPout_p__LCD_COM_SEG)
/* Enable Segment LCD */
#define BPout_p_LCD_EN                 (* (reg8 *) BPout_p__LCD_EN)
/* Slew Rate Control */
#define BPout_p_SLW                    (* (reg8 *) BPout_p__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define BPout_p_PRTDSI__CAPS_SEL       (* (reg8 *) BPout_p__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define BPout_p_PRTDSI__DBL_SYNC_IN    (* (reg8 *) BPout_p__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define BPout_p_PRTDSI__OE_SEL0        (* (reg8 *) BPout_p__PRTDSI__OE_SEL0) 
#define BPout_p_PRTDSI__OE_SEL1        (* (reg8 *) BPout_p__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define BPout_p_PRTDSI__OUT_SEL0       (* (reg8 *) BPout_p__PRTDSI__OUT_SEL0) 
#define BPout_p_PRTDSI__OUT_SEL1       (* (reg8 *) BPout_p__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define BPout_p_PRTDSI__SYNC_OUT       (* (reg8 *) BPout_p__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(BPout_p__SIO_CFG)
    #define BPout_p_SIO_HYST_EN        (* (reg8 *) BPout_p__SIO_HYST_EN)
    #define BPout_p_SIO_REG_HIFREQ     (* (reg8 *) BPout_p__SIO_REG_HIFREQ)
    #define BPout_p_SIO_CFG            (* (reg8 *) BPout_p__SIO_CFG)
    #define BPout_p_SIO_DIFF           (* (reg8 *) BPout_p__SIO_DIFF)
#endif /* (BPout_p__SIO_CFG) */

/* Interrupt Registers */
#if defined(BPout_p__INTSTAT)
    #define BPout_p_INTSTAT            (* (reg8 *) BPout_p__INTSTAT)
    #define BPout_p_SNAP               (* (reg8 *) BPout_p__SNAP)
    
	#define BPout_p_0_INTTYPE_REG 		(* (reg8 *) BPout_p__0__INTTYPE)
#endif /* (BPout_p__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_BPout_p_H */


/* [] END OF FILE */
