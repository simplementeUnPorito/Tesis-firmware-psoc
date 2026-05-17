/*******************************************************************************
* File Name: OpaLP_vm.h  
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

#if !defined(CY_PINS_OpaLP_vm_H) /* Pins OpaLP_vm_H */
#define CY_PINS_OpaLP_vm_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "OpaLP_vm_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 OpaLP_vm__PORT == 15 && ((OpaLP_vm__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    OpaLP_vm_Write(uint8 value);
void    OpaLP_vm_SetDriveMode(uint8 mode);
uint8   OpaLP_vm_ReadDataReg(void);
uint8   OpaLP_vm_Read(void);
void    OpaLP_vm_SetInterruptMode(uint16 position, uint16 mode);
uint8   OpaLP_vm_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the OpaLP_vm_SetDriveMode() function.
     *  @{
     */
        #define OpaLP_vm_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define OpaLP_vm_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define OpaLP_vm_DM_RES_UP          PIN_DM_RES_UP
        #define OpaLP_vm_DM_RES_DWN         PIN_DM_RES_DWN
        #define OpaLP_vm_DM_OD_LO           PIN_DM_OD_LO
        #define OpaLP_vm_DM_OD_HI           PIN_DM_OD_HI
        #define OpaLP_vm_DM_STRONG          PIN_DM_STRONG
        #define OpaLP_vm_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define OpaLP_vm_MASK               OpaLP_vm__MASK
#define OpaLP_vm_SHIFT              OpaLP_vm__SHIFT
#define OpaLP_vm_WIDTH              1u

/* Interrupt constants */
#if defined(OpaLP_vm__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in OpaLP_vm_SetInterruptMode() function.
     *  @{
     */
        #define OpaLP_vm_INTR_NONE      (uint16)(0x0000u)
        #define OpaLP_vm_INTR_RISING    (uint16)(0x0001u)
        #define OpaLP_vm_INTR_FALLING   (uint16)(0x0002u)
        #define OpaLP_vm_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define OpaLP_vm_INTR_MASK      (0x01u) 
#endif /* (OpaLP_vm__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define OpaLP_vm_PS                     (* (reg8 *) OpaLP_vm__PS)
/* Data Register */
#define OpaLP_vm_DR                     (* (reg8 *) OpaLP_vm__DR)
/* Port Number */
#define OpaLP_vm_PRT_NUM                (* (reg8 *) OpaLP_vm__PRT) 
/* Connect to Analog Globals */                                                  
#define OpaLP_vm_AG                     (* (reg8 *) OpaLP_vm__AG)                       
/* Analog MUX bux enable */
#define OpaLP_vm_AMUX                   (* (reg8 *) OpaLP_vm__AMUX) 
/* Bidirectional Enable */                                                        
#define OpaLP_vm_BIE                    (* (reg8 *) OpaLP_vm__BIE)
/* Bit-mask for Aliased Register Access */
#define OpaLP_vm_BIT_MASK               (* (reg8 *) OpaLP_vm__BIT_MASK)
/* Bypass Enable */
#define OpaLP_vm_BYP                    (* (reg8 *) OpaLP_vm__BYP)
/* Port wide control signals */                                                   
#define OpaLP_vm_CTL                    (* (reg8 *) OpaLP_vm__CTL)
/* Drive Modes */
#define OpaLP_vm_DM0                    (* (reg8 *) OpaLP_vm__DM0) 
#define OpaLP_vm_DM1                    (* (reg8 *) OpaLP_vm__DM1)
#define OpaLP_vm_DM2                    (* (reg8 *) OpaLP_vm__DM2) 
/* Input Buffer Disable Override */
#define OpaLP_vm_INP_DIS                (* (reg8 *) OpaLP_vm__INP_DIS)
/* LCD Common or Segment Drive */
#define OpaLP_vm_LCD_COM_SEG            (* (reg8 *) OpaLP_vm__LCD_COM_SEG)
/* Enable Segment LCD */
#define OpaLP_vm_LCD_EN                 (* (reg8 *) OpaLP_vm__LCD_EN)
/* Slew Rate Control */
#define OpaLP_vm_SLW                    (* (reg8 *) OpaLP_vm__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define OpaLP_vm_PRTDSI__CAPS_SEL       (* (reg8 *) OpaLP_vm__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define OpaLP_vm_PRTDSI__DBL_SYNC_IN    (* (reg8 *) OpaLP_vm__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define OpaLP_vm_PRTDSI__OE_SEL0        (* (reg8 *) OpaLP_vm__PRTDSI__OE_SEL0) 
#define OpaLP_vm_PRTDSI__OE_SEL1        (* (reg8 *) OpaLP_vm__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define OpaLP_vm_PRTDSI__OUT_SEL0       (* (reg8 *) OpaLP_vm__PRTDSI__OUT_SEL0) 
#define OpaLP_vm_PRTDSI__OUT_SEL1       (* (reg8 *) OpaLP_vm__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define OpaLP_vm_PRTDSI__SYNC_OUT       (* (reg8 *) OpaLP_vm__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(OpaLP_vm__SIO_CFG)
    #define OpaLP_vm_SIO_HYST_EN        (* (reg8 *) OpaLP_vm__SIO_HYST_EN)
    #define OpaLP_vm_SIO_REG_HIFREQ     (* (reg8 *) OpaLP_vm__SIO_REG_HIFREQ)
    #define OpaLP_vm_SIO_CFG            (* (reg8 *) OpaLP_vm__SIO_CFG)
    #define OpaLP_vm_SIO_DIFF           (* (reg8 *) OpaLP_vm__SIO_DIFF)
#endif /* (OpaLP_vm__SIO_CFG) */

/* Interrupt Registers */
#if defined(OpaLP_vm__INTSTAT)
    #define OpaLP_vm_INTSTAT            (* (reg8 *) OpaLP_vm__INTSTAT)
    #define OpaLP_vm_SNAP               (* (reg8 *) OpaLP_vm__SNAP)
    
	#define OpaLP_vm_0_INTTYPE_REG 		(* (reg8 *) OpaLP_vm__0__INTTYPE)
#endif /* (OpaLP_vm__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_OpaLP_vm_H */


/* [] END OF FILE */
