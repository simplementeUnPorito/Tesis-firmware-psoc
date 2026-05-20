/*******************************************************************************
* File Name: BPin_vm.h  
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

#if !defined(CY_PINS_BPin_vm_H) /* Pins BPin_vm_H */
#define CY_PINS_BPin_vm_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "BPin_vm_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 BPin_vm__PORT == 15 && ((BPin_vm__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    BPin_vm_Write(uint8 value);
void    BPin_vm_SetDriveMode(uint8 mode);
uint8   BPin_vm_ReadDataReg(void);
uint8   BPin_vm_Read(void);
void    BPin_vm_SetInterruptMode(uint16 position, uint16 mode);
uint8   BPin_vm_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the BPin_vm_SetDriveMode() function.
     *  @{
     */
        #define BPin_vm_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define BPin_vm_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define BPin_vm_DM_RES_UP          PIN_DM_RES_UP
        #define BPin_vm_DM_RES_DWN         PIN_DM_RES_DWN
        #define BPin_vm_DM_OD_LO           PIN_DM_OD_LO
        #define BPin_vm_DM_OD_HI           PIN_DM_OD_HI
        #define BPin_vm_DM_STRONG          PIN_DM_STRONG
        #define BPin_vm_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define BPin_vm_MASK               BPin_vm__MASK
#define BPin_vm_SHIFT              BPin_vm__SHIFT
#define BPin_vm_WIDTH              1u

/* Interrupt constants */
#if defined(BPin_vm__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in BPin_vm_SetInterruptMode() function.
     *  @{
     */
        #define BPin_vm_INTR_NONE      (uint16)(0x0000u)
        #define BPin_vm_INTR_RISING    (uint16)(0x0001u)
        #define BPin_vm_INTR_FALLING   (uint16)(0x0002u)
        #define BPin_vm_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define BPin_vm_INTR_MASK      (0x01u) 
#endif /* (BPin_vm__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define BPin_vm_PS                     (* (reg8 *) BPin_vm__PS)
/* Data Register */
#define BPin_vm_DR                     (* (reg8 *) BPin_vm__DR)
/* Port Number */
#define BPin_vm_PRT_NUM                (* (reg8 *) BPin_vm__PRT) 
/* Connect to Analog Globals */                                                  
#define BPin_vm_AG                     (* (reg8 *) BPin_vm__AG)                       
/* Analog MUX bux enable */
#define BPin_vm_AMUX                   (* (reg8 *) BPin_vm__AMUX) 
/* Bidirectional Enable */                                                        
#define BPin_vm_BIE                    (* (reg8 *) BPin_vm__BIE)
/* Bit-mask for Aliased Register Access */
#define BPin_vm_BIT_MASK               (* (reg8 *) BPin_vm__BIT_MASK)
/* Bypass Enable */
#define BPin_vm_BYP                    (* (reg8 *) BPin_vm__BYP)
/* Port wide control signals */                                                   
#define BPin_vm_CTL                    (* (reg8 *) BPin_vm__CTL)
/* Drive Modes */
#define BPin_vm_DM0                    (* (reg8 *) BPin_vm__DM0) 
#define BPin_vm_DM1                    (* (reg8 *) BPin_vm__DM1)
#define BPin_vm_DM2                    (* (reg8 *) BPin_vm__DM2) 
/* Input Buffer Disable Override */
#define BPin_vm_INP_DIS                (* (reg8 *) BPin_vm__INP_DIS)
/* LCD Common or Segment Drive */
#define BPin_vm_LCD_COM_SEG            (* (reg8 *) BPin_vm__LCD_COM_SEG)
/* Enable Segment LCD */
#define BPin_vm_LCD_EN                 (* (reg8 *) BPin_vm__LCD_EN)
/* Slew Rate Control */
#define BPin_vm_SLW                    (* (reg8 *) BPin_vm__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define BPin_vm_PRTDSI__CAPS_SEL       (* (reg8 *) BPin_vm__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define BPin_vm_PRTDSI__DBL_SYNC_IN    (* (reg8 *) BPin_vm__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define BPin_vm_PRTDSI__OE_SEL0        (* (reg8 *) BPin_vm__PRTDSI__OE_SEL0) 
#define BPin_vm_PRTDSI__OE_SEL1        (* (reg8 *) BPin_vm__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define BPin_vm_PRTDSI__OUT_SEL0       (* (reg8 *) BPin_vm__PRTDSI__OUT_SEL0) 
#define BPin_vm_PRTDSI__OUT_SEL1       (* (reg8 *) BPin_vm__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define BPin_vm_PRTDSI__SYNC_OUT       (* (reg8 *) BPin_vm__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(BPin_vm__SIO_CFG)
    #define BPin_vm_SIO_HYST_EN        (* (reg8 *) BPin_vm__SIO_HYST_EN)
    #define BPin_vm_SIO_REG_HIFREQ     (* (reg8 *) BPin_vm__SIO_REG_HIFREQ)
    #define BPin_vm_SIO_CFG            (* (reg8 *) BPin_vm__SIO_CFG)
    #define BPin_vm_SIO_DIFF           (* (reg8 *) BPin_vm__SIO_DIFF)
#endif /* (BPin_vm__SIO_CFG) */

/* Interrupt Registers */
#if defined(BPin_vm__INTSTAT)
    #define BPin_vm_INTSTAT            (* (reg8 *) BPin_vm__INTSTAT)
    #define BPin_vm_SNAP               (* (reg8 *) BPin_vm__SNAP)
    
	#define BPin_vm_0_INTTYPE_REG 		(* (reg8 *) BPin_vm__0__INTTYPE)
#endif /* (BPin_vm__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_BPin_vm_H */


/* [] END OF FILE */
