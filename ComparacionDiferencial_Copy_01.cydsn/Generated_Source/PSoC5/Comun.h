/*******************************************************************************
* File Name: Comun.h  
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

#if !defined(CY_PINS_Comun_H) /* Pins Comun_H */
#define CY_PINS_Comun_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "Comun_aliases.h"

/* APIs are not generated for P15[7:6] */
#if !(CY_PSOC5A &&\
	 Comun__PORT == 15 && ((Comun__MASK & 0xC0) != 0))


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    Comun_Write(uint8 value);
void    Comun_SetDriveMode(uint8 mode);
uint8   Comun_ReadDataReg(void);
uint8   Comun_Read(void);
void    Comun_SetInterruptMode(uint16 position, uint16 mode);
uint8   Comun_ClearInterrupt(void);
/** @} general */

/***************************************
*           API Constants        
***************************************/
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the Comun_SetDriveMode() function.
     *  @{
     */
        #define Comun_DM_ALG_HIZ         PIN_DM_ALG_HIZ
        #define Comun_DM_DIG_HIZ         PIN_DM_DIG_HIZ
        #define Comun_DM_RES_UP          PIN_DM_RES_UP
        #define Comun_DM_RES_DWN         PIN_DM_RES_DWN
        #define Comun_DM_OD_LO           PIN_DM_OD_LO
        #define Comun_DM_OD_HI           PIN_DM_OD_HI
        #define Comun_DM_STRONG          PIN_DM_STRONG
        #define Comun_DM_RES_UPDWN       PIN_DM_RES_UPDWN
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define Comun_MASK               Comun__MASK
#define Comun_SHIFT              Comun__SHIFT
#define Comun_WIDTH              1u

/* Interrupt constants */
#if defined(Comun__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in Comun_SetInterruptMode() function.
     *  @{
     */
        #define Comun_INTR_NONE      (uint16)(0x0000u)
        #define Comun_INTR_RISING    (uint16)(0x0001u)
        #define Comun_INTR_FALLING   (uint16)(0x0002u)
        #define Comun_INTR_BOTH      (uint16)(0x0003u) 
    /** @} intrMode */
/** @} group_constants */

    #define Comun_INTR_MASK      (0x01u) 
#endif /* (Comun__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define Comun_PS                     (* (reg8 *) Comun__PS)
/* Data Register */
#define Comun_DR                     (* (reg8 *) Comun__DR)
/* Port Number */
#define Comun_PRT_NUM                (* (reg8 *) Comun__PRT) 
/* Connect to Analog Globals */                                                  
#define Comun_AG                     (* (reg8 *) Comun__AG)                       
/* Analog MUX bux enable */
#define Comun_AMUX                   (* (reg8 *) Comun__AMUX) 
/* Bidirectional Enable */                                                        
#define Comun_BIE                    (* (reg8 *) Comun__BIE)
/* Bit-mask for Aliased Register Access */
#define Comun_BIT_MASK               (* (reg8 *) Comun__BIT_MASK)
/* Bypass Enable */
#define Comun_BYP                    (* (reg8 *) Comun__BYP)
/* Port wide control signals */                                                   
#define Comun_CTL                    (* (reg8 *) Comun__CTL)
/* Drive Modes */
#define Comun_DM0                    (* (reg8 *) Comun__DM0) 
#define Comun_DM1                    (* (reg8 *) Comun__DM1)
#define Comun_DM2                    (* (reg8 *) Comun__DM2) 
/* Input Buffer Disable Override */
#define Comun_INP_DIS                (* (reg8 *) Comun__INP_DIS)
/* LCD Common or Segment Drive */
#define Comun_LCD_COM_SEG            (* (reg8 *) Comun__LCD_COM_SEG)
/* Enable Segment LCD */
#define Comun_LCD_EN                 (* (reg8 *) Comun__LCD_EN)
/* Slew Rate Control */
#define Comun_SLW                    (* (reg8 *) Comun__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define Comun_PRTDSI__CAPS_SEL       (* (reg8 *) Comun__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define Comun_PRTDSI__DBL_SYNC_IN    (* (reg8 *) Comun__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define Comun_PRTDSI__OE_SEL0        (* (reg8 *) Comun__PRTDSI__OE_SEL0) 
#define Comun_PRTDSI__OE_SEL1        (* (reg8 *) Comun__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define Comun_PRTDSI__OUT_SEL0       (* (reg8 *) Comun__PRTDSI__OUT_SEL0) 
#define Comun_PRTDSI__OUT_SEL1       (* (reg8 *) Comun__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define Comun_PRTDSI__SYNC_OUT       (* (reg8 *) Comun__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(Comun__SIO_CFG)
    #define Comun_SIO_HYST_EN        (* (reg8 *) Comun__SIO_HYST_EN)
    #define Comun_SIO_REG_HIFREQ     (* (reg8 *) Comun__SIO_REG_HIFREQ)
    #define Comun_SIO_CFG            (* (reg8 *) Comun__SIO_CFG)
    #define Comun_SIO_DIFF           (* (reg8 *) Comun__SIO_DIFF)
#endif /* (Comun__SIO_CFG) */

/* Interrupt Registers */
#if defined(Comun__INTSTAT)
    #define Comun_INTSTAT            (* (reg8 *) Comun__INTSTAT)
    #define Comun_SNAP               (* (reg8 *) Comun__SNAP)
    
	#define Comun_0_INTTYPE_REG 		(* (reg8 *) Comun__0__INTTYPE)
#endif /* (Comun__INTSTAT) */

#endif /* CY_PSOC5A... */

#endif /*  CY_PINS_Comun_H */


/* [] END OF FILE */
