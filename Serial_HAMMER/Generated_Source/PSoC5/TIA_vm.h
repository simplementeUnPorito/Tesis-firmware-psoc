/*******************************************************************************
* File Name: TIA_vm.h  
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

#if !defined(CY_PINS_TIA_vm_H) /* Pins TIA_vm_H */
#define CY_PINS_TIA_vm_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "TIA_vm_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 TIA_vm__PORT == 15 && ((TIA_vm__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    TIA_vm_Write(uint8 value);
void    TIA_vm_SetDriveMode(uint8 mode);
uint8   TIA_vm_ReadDataReg(void);
uint8   TIA_vm_Read(void);
void    TIA_vm_SetInterruptMode(uint16 position, uint16 mode);
uint8   TIA_vm_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the TIA_vm_SetDriveMode() function.
     *  @{
     */
        #define TIA_vm_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define TIA_vm_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define TIA_vm_DM_RES_UP          PIN_DM_RES_UP
        #define TIA_vm_DM_RES_DWN         PIN_DM_RES_DWN
        #define TIA_vm_DM_OD_LO           PIN_DM_OD_LO
        #define TIA_vm_DM_OD_HI           PIN_DM_OD_HI
        #define TIA_vm_DM_STRONG          PIN_DM_STRONG
        #define TIA_vm_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define TIA_vm_MASK               TIA_vm__MASK
#define TIA_vm_SHIFT              TIA_vm__SHIFT
#define TIA_vm_WIDTH              1u

/* Interrupt constants */
#if defined(TIA_vm__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in TIA_vm_SetInterruptMode() function.
     *  @{
     */
        #define TIA_vm_INTR_NONE      (uint16)(0x0000u)
        #define TIA_vm_INTR_RISING    (uint16)(0x0001u)
        #define TIA_vm_INTR_FALLING   (uint16)(0x0002u)
        #define TIA_vm_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define TIA_vm_INTR_MASK      (0x01u) 
#endif /* (TIA_vm__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define TIA_vm_PS                     (* (reg8 *) TIA_vm__PS)
/* Data Register */
#define TIA_vm_DR                     (* (reg8 *) TIA_vm__DR)
/* Port Number */
#define TIA_vm_PRT_NUM                (* (reg8 *) TIA_vm__PRT) 
/* Connect to Analog Globals */                                                  
#define TIA_vm_AG                     (* (reg8 *) TIA_vm__AG)                       
/* Analog MUX bux enable */
#define TIA_vm_AMUX                   (* (reg8 *) TIA_vm__AMUX) 
/* Bidirectional Enable */                                                        
#define TIA_vm_BIE                    (* (reg8 *) TIA_vm__BIE)
/* Bit-mask for Aliased Register Access */
#define TIA_vm_BIT_MASK               (* (reg8 *) TIA_vm__BIT_MASK)
/* Bypass Enable */
#define TIA_vm_BYP                    (* (reg8 *) TIA_vm__BYP)
/* Port wide control signals */                                                   
#define TIA_vm_CTL                    (* (reg8 *) TIA_vm__CTL)
/* Drive Modes */
#define TIA_vm_DM0                    (* (reg8 *) TIA_vm__DM0) 
#define TIA_vm_DM1                    (* (reg8 *) TIA_vm__DM1)
#define TIA_vm_DM2                    (* (reg8 *) TIA_vm__DM2) 
/* Input Buffer Disable Override */
#define TIA_vm_INP_DIS                (* (reg8 *) TIA_vm__INP_DIS)
/* LCD Common or Segment Drive */
#define TIA_vm_LCD_COM_SEG            (* (reg8 *) TIA_vm__LCD_COM_SEG)
/* Enable Segment LCD */
#define TIA_vm_LCD_EN                 (* (reg8 *) TIA_vm__LCD_EN)
/* Slew Rate Control */
#define TIA_vm_SLW                    (* (reg8 *) TIA_vm__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define TIA_vm_PRTDSI__CAPS_SEL       (* (reg8 *) TIA_vm__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define TIA_vm_PRTDSI__DBL_SYNC_IN    (* (reg8 *) TIA_vm__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define TIA_vm_PRTDSI__OE_SEL0        (* (reg8 *) TIA_vm__PRTDSI__OE_SEL0) 
#define TIA_vm_PRTDSI__OE_SEL1        (* (reg8 *) TIA_vm__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define TIA_vm_PRTDSI__OUT_SEL0       (* (reg8 *) TIA_vm__PRTDSI__OUT_SEL0) 
#define TIA_vm_PRTDSI__OUT_SEL1       (* (reg8 *) TIA_vm__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define TIA_vm_PRTDSI__SYNC_OUT       (* (reg8 *) TIA_vm__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(TIA_vm__SIO_CFG)
    #define TIA_vm_SIO_HYST_EN        (* (reg8 *) TIA_vm__SIO_HYST_EN)
    #define TIA_vm_SIO_REG_HIFREQ     (* (reg8 *) TIA_vm__SIO_REG_HIFREQ)
    #define TIA_vm_SIO_CFG            (* (reg8 *) TIA_vm__SIO_CFG)
    #define TIA_vm_SIO_DIFF           (* (reg8 *) TIA_vm__SIO_DIFF)
#endif /* (TIA_vm__SIO_CFG) */

/* Interrupt Registers */
#if defined(TIA_vm__INTSTAT)
    #define TIA_vm_INTSTAT            (* (reg8 *) TIA_vm__INTSTAT)
    #define TIA_vm_SNAP               (* (reg8 *) TIA_vm__SNAP)
    
	#define TIA_vm_0_INTTYPE_REG 		(* (reg8 *) TIA_vm__0__INTTYPE)
#endif /* (TIA_vm__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_TIA_vm_H */


/* [] END OF FILE */
