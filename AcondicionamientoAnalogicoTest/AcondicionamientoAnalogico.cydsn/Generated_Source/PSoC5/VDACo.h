/*******************************************************************************
* File Name: VDACo.h  
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

#if !defined(CY_PINS_VDACo_H) /* Pins VDACo_H */
#define CY_PINS_VDACo_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "VDACo_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 VDACo__PORT == 15 && ((VDACo__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    VDACo_Write(uint8 value);
void    VDACo_SetDriveMode(uint8 mode);
uint8   VDACo_ReadDataReg(void);
uint8   VDACo_Read(void);
void    VDACo_SetInterruptMode(uint16 position, uint16 mode);
uint8   VDACo_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the VDACo_SetDriveMode() function.
     *  @{
     */
        #define VDACo_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define VDACo_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define VDACo_DM_RES_UP          PIN_DM_RES_UP
        #define VDACo_DM_RES_DWN         PIN_DM_RES_DWN
        #define VDACo_DM_OD_LO           PIN_DM_OD_LO
        #define VDACo_DM_OD_HI           PIN_DM_OD_HI
        #define VDACo_DM_STRONG          PIN_DM_STRONG
        #define VDACo_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define VDACo_MASK               VDACo__MASK
#define VDACo_SHIFT              VDACo__SHIFT
#define VDACo_WIDTH              1u

/* Interrupt constants */
#if defined(VDACo__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in VDACo_SetInterruptMode() function.
     *  @{
     */
        #define VDACo_INTR_NONE      (uint16)(0x0000u)
        #define VDACo_INTR_RISING    (uint16)(0x0001u)
        #define VDACo_INTR_FALLING   (uint16)(0x0002u)
        #define VDACo_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define VDACo_INTR_MASK      (0x01u) 
#endif /* (VDACo__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define VDACo_PS                     (* (reg8 *) VDACo__PS)
/* Data Register */
#define VDACo_DR                     (* (reg8 *) VDACo__DR)
/* Port Number */
#define VDACo_PRT_NUM                (* (reg8 *) VDACo__PRT) 
/* Connect to Analog Globals */                                                  
#define VDACo_AG                     (* (reg8 *) VDACo__AG)                       
/* Analog MUX bux enable */
#define VDACo_AMUX                   (* (reg8 *) VDACo__AMUX) 
/* Bidirectional Enable */                                                        
#define VDACo_BIE                    (* (reg8 *) VDACo__BIE)
/* Bit-mask for Aliased Register Access */
#define VDACo_BIT_MASK               (* (reg8 *) VDACo__BIT_MASK)
/* Bypass Enable */
#define VDACo_BYP                    (* (reg8 *) VDACo__BYP)
/* Port wide control signals */                                                   
#define VDACo_CTL                    (* (reg8 *) VDACo__CTL)
/* Drive Modes */
#define VDACo_DM0                    (* (reg8 *) VDACo__DM0) 
#define VDACo_DM1                    (* (reg8 *) VDACo__DM1)
#define VDACo_DM2                    (* (reg8 *) VDACo__DM2) 
/* Input Buffer Disable Override */
#define VDACo_INP_DIS                (* (reg8 *) VDACo__INP_DIS)
/* LCD Common or Segment Drive */
#define VDACo_LCD_COM_SEG            (* (reg8 *) VDACo__LCD_COM_SEG)
/* Enable Segment LCD */
#define VDACo_LCD_EN                 (* (reg8 *) VDACo__LCD_EN)
/* Slew Rate Control */
#define VDACo_SLW                    (* (reg8 *) VDACo__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define VDACo_PRTDSI__CAPS_SEL       (* (reg8 *) VDACo__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define VDACo_PRTDSI__DBL_SYNC_IN    (* (reg8 *) VDACo__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define VDACo_PRTDSI__OE_SEL0        (* (reg8 *) VDACo__PRTDSI__OE_SEL0) 
#define VDACo_PRTDSI__OE_SEL1        (* (reg8 *) VDACo__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define VDACo_PRTDSI__OUT_SEL0       (* (reg8 *) VDACo__PRTDSI__OUT_SEL0) 
#define VDACo_PRTDSI__OUT_SEL1       (* (reg8 *) VDACo__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define VDACo_PRTDSI__SYNC_OUT       (* (reg8 *) VDACo__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(VDACo__SIO_CFG)
    #define VDACo_SIO_HYST_EN        (* (reg8 *) VDACo__SIO_HYST_EN)
    #define VDACo_SIO_REG_HIFREQ     (* (reg8 *) VDACo__SIO_REG_HIFREQ)
    #define VDACo_SIO_CFG            (* (reg8 *) VDACo__SIO_CFG)
    #define VDACo_SIO_DIFF           (* (reg8 *) VDACo__SIO_DIFF)
#endif /* (VDACo__SIO_CFG) */

/* Interrupt Registers */
#if defined(VDACo__INTSTAT)
    #define VDACo_INTSTAT            (* (reg8 *) VDACo__INTSTAT)
    #define VDACo_SNAP               (* (reg8 *) VDACo__SNAP)
    
	#define VDACo_0_INTTYPE_REG 		(* (reg8 *) VDACo__0__INTTYPE)
#endif /* (VDACo__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_VDACo_H */


/* [] END OF FILE */
