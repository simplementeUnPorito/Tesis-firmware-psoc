/*******************************************************************************
* File Name: ref_BP.h  
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

#if !defined(CY_PINS_ref_BP_H) /* Pins ref_BP_H */
#define CY_PINS_ref_BP_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "ref_BP_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 ref_BP__PORT == 15 && ((ref_BP__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    ref_BP_Write(uint8 value);
void    ref_BP_SetDriveMode(uint8 mode);
uint8   ref_BP_ReadDataReg(void);
uint8   ref_BP_Read(void);
void    ref_BP_SetInterruptMode(uint16 position, uint16 mode);
uint8   ref_BP_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the ref_BP_SetDriveMode() function.
     *  @{
     */
        #define ref_BP_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define ref_BP_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define ref_BP_DM_RES_UP          PIN_DM_RES_UP
        #define ref_BP_DM_RES_DWN         PIN_DM_RES_DWN
        #define ref_BP_DM_OD_LO           PIN_DM_OD_LO
        #define ref_BP_DM_OD_HI           PIN_DM_OD_HI
        #define ref_BP_DM_STRONG          PIN_DM_STRONG
        #define ref_BP_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define ref_BP_MASK               ref_BP__MASK
#define ref_BP_SHIFT              ref_BP__SHIFT
#define ref_BP_WIDTH              1u

/* Interrupt constants */
#if defined(ref_BP__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in ref_BP_SetInterruptMode() function.
     *  @{
     */
        #define ref_BP_INTR_NONE      (uint16)(0x0000u)
        #define ref_BP_INTR_RISING    (uint16)(0x0001u)
        #define ref_BP_INTR_FALLING   (uint16)(0x0002u)
        #define ref_BP_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define ref_BP_INTR_MASK      (0x01u) 
#endif /* (ref_BP__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define ref_BP_PS                     (* (reg8 *) ref_BP__PS)
/* Data Register */
#define ref_BP_DR                     (* (reg8 *) ref_BP__DR)
/* Port Number */
#define ref_BP_PRT_NUM                (* (reg8 *) ref_BP__PRT) 
/* Connect to Analog Globals */                                                  
#define ref_BP_AG                     (* (reg8 *) ref_BP__AG)                       
/* Analog MUX bux enable */
#define ref_BP_AMUX                   (* (reg8 *) ref_BP__AMUX) 
/* Bidirectional Enable */                                                        
#define ref_BP_BIE                    (* (reg8 *) ref_BP__BIE)
/* Bit-mask for Aliased Register Access */
#define ref_BP_BIT_MASK               (* (reg8 *) ref_BP__BIT_MASK)
/* Bypass Enable */
#define ref_BP_BYP                    (* (reg8 *) ref_BP__BYP)
/* Port wide control signals */                                                   
#define ref_BP_CTL                    (* (reg8 *) ref_BP__CTL)
/* Drive Modes */
#define ref_BP_DM0                    (* (reg8 *) ref_BP__DM0) 
#define ref_BP_DM1                    (* (reg8 *) ref_BP__DM1)
#define ref_BP_DM2                    (* (reg8 *) ref_BP__DM2) 
/* Input Buffer Disable Override */
#define ref_BP_INP_DIS                (* (reg8 *) ref_BP__INP_DIS)
/* LCD Common or Segment Drive */
#define ref_BP_LCD_COM_SEG            (* (reg8 *) ref_BP__LCD_COM_SEG)
/* Enable Segment LCD */
#define ref_BP_LCD_EN                 (* (reg8 *) ref_BP__LCD_EN)
/* Slew Rate Control */
#define ref_BP_SLW                    (* (reg8 *) ref_BP__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define ref_BP_PRTDSI__CAPS_SEL       (* (reg8 *) ref_BP__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define ref_BP_PRTDSI__DBL_SYNC_IN    (* (reg8 *) ref_BP__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define ref_BP_PRTDSI__OE_SEL0        (* (reg8 *) ref_BP__PRTDSI__OE_SEL0) 
#define ref_BP_PRTDSI__OE_SEL1        (* (reg8 *) ref_BP__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define ref_BP_PRTDSI__OUT_SEL0       (* (reg8 *) ref_BP__PRTDSI__OUT_SEL0) 
#define ref_BP_PRTDSI__OUT_SEL1       (* (reg8 *) ref_BP__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define ref_BP_PRTDSI__SYNC_OUT       (* (reg8 *) ref_BP__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(ref_BP__SIO_CFG)
    #define ref_BP_SIO_HYST_EN        (* (reg8 *) ref_BP__SIO_HYST_EN)
    #define ref_BP_SIO_REG_HIFREQ     (* (reg8 *) ref_BP__SIO_REG_HIFREQ)
    #define ref_BP_SIO_CFG            (* (reg8 *) ref_BP__SIO_CFG)
    #define ref_BP_SIO_DIFF           (* (reg8 *) ref_BP__SIO_DIFF)
#endif /* (ref_BP__SIO_CFG) */

/* Interrupt Registers */
#if defined(ref_BP__INTSTAT)
    #define ref_BP_INTSTAT            (* (reg8 *) ref_BP__INTSTAT)
    #define ref_BP_SNAP               (* (reg8 *) ref_BP__SNAP)
    
	#define ref_BP_0_INTTYPE_REG 		(* (reg8 *) ref_BP__0__INTTYPE)
#endif /* (ref_BP__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_ref_BP_H */


/* [] END OF FILE */
