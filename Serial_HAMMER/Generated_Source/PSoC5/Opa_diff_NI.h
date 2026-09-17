/*******************************************************************************
* File Name: Opa_diff_NI.h  
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

#if !defined(CY_PINS_Opa_diff_NI_H) /* Pins Opa_diff_NI_H */
#define CY_PINS_Opa_diff_NI_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "Opa_diff_NI_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 Opa_diff_NI__PORT == 15 && ((Opa_diff_NI__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    Opa_diff_NI_Write(uint8 value);
void    Opa_diff_NI_SetDriveMode(uint8 mode);
uint8   Opa_diff_NI_ReadDataReg(void);
uint8   Opa_diff_NI_Read(void);
void    Opa_diff_NI_SetInterruptMode(uint16 position, uint16 mode);
uint8   Opa_diff_NI_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the Opa_diff_NI_SetDriveMode() function.
     *  @{
     */
        #define Opa_diff_NI_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define Opa_diff_NI_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define Opa_diff_NI_DM_RES_UP          PIN_DM_RES_UP
        #define Opa_diff_NI_DM_RES_DWN         PIN_DM_RES_DWN
        #define Opa_diff_NI_DM_OD_LO           PIN_DM_OD_LO
        #define Opa_diff_NI_DM_OD_HI           PIN_DM_OD_HI
        #define Opa_diff_NI_DM_STRONG          PIN_DM_STRONG
        #define Opa_diff_NI_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define Opa_diff_NI_MASK               Opa_diff_NI__MASK
#define Opa_diff_NI_SHIFT              Opa_diff_NI__SHIFT
#define Opa_diff_NI_WIDTH              1u

/* Interrupt constants */
#if defined(Opa_diff_NI__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in Opa_diff_NI_SetInterruptMode() function.
     *  @{
     */
        #define Opa_diff_NI_INTR_NONE      (uint16)(0x0000u)
        #define Opa_diff_NI_INTR_RISING    (uint16)(0x0001u)
        #define Opa_diff_NI_INTR_FALLING   (uint16)(0x0002u)
        #define Opa_diff_NI_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define Opa_diff_NI_INTR_MASK      (0x01u) 
#endif /* (Opa_diff_NI__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define Opa_diff_NI_PS                     (* (reg8 *) Opa_diff_NI__PS)
/* Data Register */
#define Opa_diff_NI_DR                     (* (reg8 *) Opa_diff_NI__DR)
/* Port Number */
#define Opa_diff_NI_PRT_NUM                (* (reg8 *) Opa_diff_NI__PRT) 
/* Connect to Analog Globals */                                                  
#define Opa_diff_NI_AG                     (* (reg8 *) Opa_diff_NI__AG)                       
/* Analog MUX bux enable */
#define Opa_diff_NI_AMUX                   (* (reg8 *) Opa_diff_NI__AMUX) 
/* Bidirectional Enable */                                                        
#define Opa_diff_NI_BIE                    (* (reg8 *) Opa_diff_NI__BIE)
/* Bit-mask for Aliased Register Access */
#define Opa_diff_NI_BIT_MASK               (* (reg8 *) Opa_diff_NI__BIT_MASK)
/* Bypass Enable */
#define Opa_diff_NI_BYP                    (* (reg8 *) Opa_diff_NI__BYP)
/* Port wide control signals */                                                   
#define Opa_diff_NI_CTL                    (* (reg8 *) Opa_diff_NI__CTL)
/* Drive Modes */
#define Opa_diff_NI_DM0                    (* (reg8 *) Opa_diff_NI__DM0) 
#define Opa_diff_NI_DM1                    (* (reg8 *) Opa_diff_NI__DM1)
#define Opa_diff_NI_DM2                    (* (reg8 *) Opa_diff_NI__DM2) 
/* Input Buffer Disable Override */
#define Opa_diff_NI_INP_DIS                (* (reg8 *) Opa_diff_NI__INP_DIS)
/* LCD Common or Segment Drive */
#define Opa_diff_NI_LCD_COM_SEG            (* (reg8 *) Opa_diff_NI__LCD_COM_SEG)
/* Enable Segment LCD */
#define Opa_diff_NI_LCD_EN                 (* (reg8 *) Opa_diff_NI__LCD_EN)
/* Slew Rate Control */
#define Opa_diff_NI_SLW                    (* (reg8 *) Opa_diff_NI__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define Opa_diff_NI_PRTDSI__CAPS_SEL       (* (reg8 *) Opa_diff_NI__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define Opa_diff_NI_PRTDSI__DBL_SYNC_IN    (* (reg8 *) Opa_diff_NI__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define Opa_diff_NI_PRTDSI__OE_SEL0        (* (reg8 *) Opa_diff_NI__PRTDSI__OE_SEL0) 
#define Opa_diff_NI_PRTDSI__OE_SEL1        (* (reg8 *) Opa_diff_NI__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define Opa_diff_NI_PRTDSI__OUT_SEL0       (* (reg8 *) Opa_diff_NI__PRTDSI__OUT_SEL0) 
#define Opa_diff_NI_PRTDSI__OUT_SEL1       (* (reg8 *) Opa_diff_NI__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define Opa_diff_NI_PRTDSI__SYNC_OUT       (* (reg8 *) Opa_diff_NI__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(Opa_diff_NI__SIO_CFG)
    #define Opa_diff_NI_SIO_HYST_EN        (* (reg8 *) Opa_diff_NI__SIO_HYST_EN)
    #define Opa_diff_NI_SIO_REG_HIFREQ     (* (reg8 *) Opa_diff_NI__SIO_REG_HIFREQ)
    #define Opa_diff_NI_SIO_CFG            (* (reg8 *) Opa_diff_NI__SIO_CFG)
    #define Opa_diff_NI_SIO_DIFF           (* (reg8 *) Opa_diff_NI__SIO_DIFF)
#endif /* (Opa_diff_NI__SIO_CFG) */

/* Interrupt Registers */
#if defined(Opa_diff_NI__INTSTAT)
    #define Opa_diff_NI_INTSTAT            (* (reg8 *) Opa_diff_NI__INTSTAT)
    #define Opa_diff_NI_SNAP               (* (reg8 *) Opa_diff_NI__SNAP)
    
	#define Opa_diff_NI_0_INTTYPE_REG 		(* (reg8 *) Opa_diff_NI__0__INTTYPE)
#endif /* (Opa_diff_NI__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_Opa_diff_NI_H */


/* [] END OF FILE */
