/*******************************************************************************
* File Name: Vm.h  
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

#if !defined(CY_PINS_Vm_H) /* Pins Vm_H */
#define CY_PINS_Vm_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "Vm_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 Vm__PORT == 15 && ((Vm__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    Vm_Write(uint8 value);
void    Vm_SetDriveMode(uint8 mode);
uint8   Vm_ReadDataReg(void);
uint8   Vm_Read(void);
void    Vm_SetInterruptMode(uint16 position, uint16 mode);
uint8   Vm_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the Vm_SetDriveMode() function.
     *  @{
     */
        #define Vm_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define Vm_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define Vm_DM_RES_UP          PIN_DM_RES_UP
        #define Vm_DM_RES_DWN         PIN_DM_RES_DWN
        #define Vm_DM_OD_LO           PIN_DM_OD_LO
        #define Vm_DM_OD_HI           PIN_DM_OD_HI
        #define Vm_DM_STRONG          PIN_DM_STRONG
        #define Vm_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define Vm_MASK               Vm__MASK
#define Vm_SHIFT              Vm__SHIFT
#define Vm_WIDTH              1u

/* Interrupt constants */
#if defined(Vm__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in Vm_SetInterruptMode() function.
     *  @{
     */
        #define Vm_INTR_NONE      (uint16)(0x0000u)
        #define Vm_INTR_RISING    (uint16)(0x0001u)
        #define Vm_INTR_FALLING   (uint16)(0x0002u)
        #define Vm_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define Vm_INTR_MASK      (0x01u) 
#endif /* (Vm__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define Vm_PS                     (* (reg8 *) Vm__PS)
/* Data Register */
#define Vm_DR                     (* (reg8 *) Vm__DR)
/* Port Number */
#define Vm_PRT_NUM                (* (reg8 *) Vm__PRT) 
/* Connect to Analog Globals */                                                  
#define Vm_AG                     (* (reg8 *) Vm__AG)                       
/* Analog MUX bux enable */
#define Vm_AMUX                   (* (reg8 *) Vm__AMUX) 
/* Bidirectional Enable */                                                        
#define Vm_BIE                    (* (reg8 *) Vm__BIE)
/* Bit-mask for Aliased Register Access */
#define Vm_BIT_MASK               (* (reg8 *) Vm__BIT_MASK)
/* Bypass Enable */
#define Vm_BYP                    (* (reg8 *) Vm__BYP)
/* Port wide control signals */                                                   
#define Vm_CTL                    (* (reg8 *) Vm__CTL)
/* Drive Modes */
#define Vm_DM0                    (* (reg8 *) Vm__DM0) 
#define Vm_DM1                    (* (reg8 *) Vm__DM1)
#define Vm_DM2                    (* (reg8 *) Vm__DM2) 
/* Input Buffer Disable Override */
#define Vm_INP_DIS                (* (reg8 *) Vm__INP_DIS)
/* LCD Common or Segment Drive */
#define Vm_LCD_COM_SEG            (* (reg8 *) Vm__LCD_COM_SEG)
/* Enable Segment LCD */
#define Vm_LCD_EN                 (* (reg8 *) Vm__LCD_EN)
/* Slew Rate Control */
#define Vm_SLW                    (* (reg8 *) Vm__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define Vm_PRTDSI__CAPS_SEL       (* (reg8 *) Vm__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define Vm_PRTDSI__DBL_SYNC_IN    (* (reg8 *) Vm__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define Vm_PRTDSI__OE_SEL0        (* (reg8 *) Vm__PRTDSI__OE_SEL0) 
#define Vm_PRTDSI__OE_SEL1        (* (reg8 *) Vm__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define Vm_PRTDSI__OUT_SEL0       (* (reg8 *) Vm__PRTDSI__OUT_SEL0) 
#define Vm_PRTDSI__OUT_SEL1       (* (reg8 *) Vm__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define Vm_PRTDSI__SYNC_OUT       (* (reg8 *) Vm__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(Vm__SIO_CFG)
    #define Vm_SIO_HYST_EN        (* (reg8 *) Vm__SIO_HYST_EN)
    #define Vm_SIO_REG_HIFREQ     (* (reg8 *) Vm__SIO_REG_HIFREQ)
    #define Vm_SIO_CFG            (* (reg8 *) Vm__SIO_CFG)
    #define Vm_SIO_DIFF           (* (reg8 *) Vm__SIO_DIFF)
#endif /* (Vm__SIO_CFG) */

/* Interrupt Registers */
#if defined(Vm__INTSTAT)
    #define Vm_INTSTAT            (* (reg8 *) Vm__INTSTAT)
    #define Vm_SNAP               (* (reg8 *) Vm__SNAP)
    
	#define Vm_0_INTTYPE_REG 		(* (reg8 *) Vm__0__INTTYPE)
#endif /* (Vm__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_Vm_H */


/* [] END OF FILE */
